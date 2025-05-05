#ifndef SENSOR_H
#define SENSOR_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint16_t battery_mv; // Battery level in mv
	uint16_t temperature; // Divide by 100 to get the temperature in °C
} sensor_data_t;
typedef enum {
	DISCONNECTED,
	ADVERTISING,
	CONNECTED,
} sensor_state_t;

extern sensor_data_t sensor_data;

#endif
