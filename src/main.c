#include "lionk_adc.h"
#include "sensor.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>

#include "ble.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void do_work(struct k_work *work);
static void timer_handler(struct k_timer *timer);
static void update_data(void);

K_WORK_DEFINE(update_data_work, do_work);
K_TIMER_DEFINE(work_timer, timer_handler, NULL);

sensor_data_t sensor_data;
static sensor_state_t state = DISCONNECTED;

static const struct gpio_dt_spec temp_resistor_div_en =
	GPIO_DT_SPEC_GET(DT_ALIAS(resistordiven0), gpios);
static const struct gpio_dt_spec battery_resistor_div_en =
	GPIO_DT_SPEC_GET(DT_ALIAS(resistordiven1), gpios);

static const struct adc_dt_spec temperature_spec =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static const struct adc_dt_spec battery_spec =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

/**
 * @brief Updates sensor data by reading temperature and battery values
 * 
 * This function enables the resistor dividers, waits for voltage stabilization,
 * reads ADC values for temperature and battery, then disables the resistor
 * dividers to save power. The raw ADC values are converted to meaningful units.
 */
void update_data(void)
{
	gpio_pin_set_dt(&temp_resistor_div_en, 1);
	gpio_pin_set_dt(&battery_resistor_div_en, 1);

	k_msleep(10);

	uint16_t temp_read_mv = lionk_adc_do_read(&temperature_spec);
	uint16_t battery_read = lionk_adc_do_read(&battery_spec);

	gpio_pin_set_dt(&temp_resistor_div_en, 0);
	gpio_pin_set_dt(&battery_resistor_div_en, 0);

	sensor_data.temperature = temp_read_mv - 500;
	sensor_data.battery_mv = battery_read * 4;
}

/**
 * @brief Main work function that handles sensor updates and BLE state management
 * 
 * This function is called periodically by the timer. It updates sensor data,
 * logs the values, and manages the BLE connection state machine (disconnected,
 * advertising, connected). When connected and subscribed, it sends sensor data.
 * 
 * @param work Pointer to the work structure (unused)
 */
void do_work(struct k_work *work)
{
	(void)work;
	update_data();
	LOG_INF("Temperature: %d, battery %d", sensor_data.temperature,
		sensor_data.battery_mv);
	switch (state) {
	case DISCONNECTED:
		ble_start_advertising();
		state = ADVERTISING;
		LOG_INF("Advertising");
		break;

	case ADVERTISING:
		if (ble_is_connected()) {
			state = CONNECTED;
			LOG_INF("Connected!");
		}
		break;
	case CONNECTED:
		if (!ble_is_connected()) {
			LOG_INF("Disconnected");
			state = DISCONNECTED;
		}

		if (ble_is_subscribed()) {
			const int ret = ble_send_data();
			if (ret) {
				LOG_ERR("Couldn't send data (%d)", ret);
			}
		}
		break;
	}
}

/**
 * @brief Timer callback function that submits work to the system work queue
 * 
 * This function is called every second by the timer and submits the sensor
 * update work to be processed by the system work queue.
 * 
 * @param timer Pointer to the timer structure (unused)
 */
void timer_handler(struct k_timer *timer)
{
	k_work_submit(&update_data_work);
}

/**
 * @brief Configures flash protection settings for the nRF device
 * 
 * This function enables or disables flash protection (APPROTECT) based on the
 * ENABLE_APPROTECT configuration. It modifies the UICR (User Information 
 * Configuration Registers) and performs a system reset if changes are made.
 * This protects the flash memory from unauthorized access.
 */
static void nrf_bootloader_debug_port_disable(void)
{
#ifdef ENABLE_APPROTECT
	if ((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) !=
	    (UICR_APPROTECT_PALL_Enabled << UICR_APPROTECT_PALL_Pos)) {
		LOG_INF("Flash Protection not enabled. Enabling and resetting the device");
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
			;
		NRF_UICR->APPROTECT = ((NRF_UICR->APPROTECT &
					~((uint32_t)UICR_APPROTECT_PALL_Msk)) |
				       (UICR_APPROTECT_PALL_Enabled
					<< UICR_APPROTECT_PALL_Pos));

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
			;
		NVIC_SystemReset();
	}
#else
	if ((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) !=
	    (UICR_APPROTECT_PALL_HwDisabled << UICR_APPROTECT_PALL_Pos)) {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
			;

		NRF_UICR->APPROTECT = ((NRF_UICR->APPROTECT &
					~((uint32_t)UICR_APPROTECT_PALL_Msk)) |
				       (UICR_APPROTECT_PALL_HwDisabled
					<< UICR_APPROTECT_PALL_Pos));

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
			;
	}
#endif
}

/**
 * @brief Main application entry point
 * 
 * This function initializes the system by:
 * - Configuring flash protection settings
 * - Setting up GPIO pins for resistor divider control
 * - Initializing BLE functionality
 * - Configuring ADC channels for temperature and battery monitoring
 * - Starting a periodic timer for sensor updates
 * - Entering an infinite sleep state (work is handled by interrupts)
 * 
 * @return Should never return; exits with error code if initialization fails
 */
int main(void)
{
	int ret;

	nrf_bootloader_debug_port_disable();

	if (!gpio_is_ready_dt(&temp_resistor_div_en)) {
		LOG_ERR("GPIO device %s is not ready",
			temp_resistor_div_en.port->name);
		return -1;
	}
	if (!gpio_is_ready_dt(&battery_resistor_div_en)) {
		LOG_ERR("GPIO device %s is not ready",
			battery_resistor_div_en.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&temp_resistor_div_en,
				    GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Cannot configure GPIO pin for temperature resistor divider");
		return -1;
	}

	ret = gpio_pin_configure_dt(&battery_resistor_div_en,
				    GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Cannot configure GPIO pin for battery resistor divider");
		return -1;
	}

	LOG_INF("GPIO pins configured for resistor divider control");

	ble_setup();
	lionk_adc_setup(&temperature_spec);
	lionk_adc_setup(&battery_spec);
	k_timer_start(&work_timer, K_SECONDS(1), K_SECONDS(1));
	k_sleep(K_FOREVER);
}
