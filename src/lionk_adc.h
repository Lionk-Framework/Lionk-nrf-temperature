#ifndef LIONK_ADC_H
#define LIONK_ADC_H

#include <zephyr/drivers/adc.h>

int lionk_adc_do_read(const struct adc_dt_spec *spec);
void lionk_adc_setup(const struct adc_dt_spec *spec);
#endif
