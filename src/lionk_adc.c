#include "lionk_adc.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lionk_adc, LOG_LEVEL_INF);

/**
 * @brief Set up an ADC channel for reading
 * 
 * Configures the specified ADC channel based on the device tree specification.
 * This function must be called before attempting to read from the channel.
 * If setup fails, the function will assert and halt execution.
 * 
 * @param spec Pointer to ADC device tree specification containing channel configuration
 */
void lionk_adc_setup(const struct adc_dt_spec *spec)
{
	int err = adc_channel_setup_dt(spec);
	if (err < 0) {
		__ASSERT(0, "Couldn't setup ADC channel (%d)", err);
	}
	LOG_INF("ADC channel %d setup on %s", spec->channel_id,
		spec->dev->name);
}

/**
 * @brief Perform a single ADC reading and convert to millivolts
 * 
 * Reads a raw value from the specified ADC channel and converts it to millivolts.
 * The function handles both differential and single-ended channel configurations.
 * If the read operation fails, the function returns 0 and logs a warning.
 * 
 * @param spec Pointer to ADC device tree specification for the channel to read
 * @return int Voltage reading in millivolts, or 0 if read failed
 */
int lionk_adc_do_read(const struct adc_dt_spec *spec)
{
	uint16_t buf;
	int val_mv;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
	int err = adc_sequence_init_dt(spec, &sequence);
	if (err < 0) {
		LOG_ERR("Couldn't initialise read sequence from ADC %s\n",
			spec->dev->name);
		return 0;
	}

	err = adc_read_dt(spec, &sequence);
	if (err < 0) {
		LOG_WRN("Couldn't read from ADC %s\n", spec->dev->name);
		return 0;
	}

	if (spec->channel_cfg.differential) {
		val_mv = (int32_t)((int16_t)buf);
	} else {
		val_mv = (int32_t)buf;
	}
	err = adc_raw_to_millivolts_dt(spec, &val_mv);
	/* conversion to mV may not be supported, skip if not */
	if (err < 0) {
		LOG_INF("Can't Get value in mV\n");
	}
	return val_mv;
}
