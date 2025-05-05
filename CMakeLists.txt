cmake_minimum_required(VERSION 3.20.0)

# Add the app overlay file before including the zephyr package
list(APPEND DTC_OVERLAY_FILE app.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lionk-nrf-temperature)

target_sources(app PRIVATE src/main.c src/ble.c src/lionk_adc.c)
