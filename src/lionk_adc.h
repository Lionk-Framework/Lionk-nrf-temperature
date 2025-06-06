#ifndef LIONK_ADC_H
#define LIONK_ADC_H

#include <zephyr/drivers/adc.h>

/**
 * @brief Set up an ADC channel for reading
 * 
 * Configures the specified ADC channel based on the device tree specification.
 * This function must be called before attempting to read from the channel.
 * If setup fails, the function will assert and halt execution.
 * 
 * @param spec Pointer to ADC device tree specification containing channel configuration
 */
void lionk_adc_setup(const struct adc_dt_spec *spec);

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
int lionk_adc_do_read(const struct adc_dt_spec *spec);

#endif
