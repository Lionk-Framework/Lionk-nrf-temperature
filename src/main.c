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

static int64_t last_payload_ts = 0;
/* Get the ADC device node identifier */
#define ADC_NODE DT_NODELABEL(adc)

static const struct adc_dt_spec temperature_spec =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
//ADC_DT_SPEC_GET_BY_IDX(ADC_NODE, 0);

static const struct adc_dt_spec battery_spec =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

void update_data(void)
{
	uint16_t temp_read_mv = lionk_adc_do_read(&temperature_spec);
	uint16_t battery_read = lionk_adc_do_read(&battery_spec);

	sensor_data.temperature = temp_read_mv - 500;
	sensor_data.battery_mv = battery_read * 4;
}

void do_work(struct k_work *work)
{
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
			if (ret == 0) {
				last_payload_ts = k_uptime_get();
			} else {
				LOG_ERR("Couldn't send data (%d)", ret);
			}
		}
		break;
	}
}

void timer_handler(struct k_timer *timer)
{
	k_work_submit(&update_data_work);
}

int main(void)
{
	ble_setup();
	lionk_adc_setup(&temperature_spec);
	lionk_adc_setup(&battery_spec);
	k_timer_start(&work_timer, K_SECONDS(1), K_SECONDS(1));
	k_sleep(K_FOREVER);
}
