#include <stdbool.h>
#include <zephyr/bluetooth/uuid.h>

#ifndef BLE_H

#define BLE_H

#define BT_UUID_TEMPERATURE_SVC_VAL \
	BT_UUID_128_ENCODE(0x00000001, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_BATTERY_SVC_VAL \
	BT_UUID_128_ENCODE(0x00000003, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_BATTERY_VAL \
	BT_UUID_128_ENCODE(0x00000005, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_VERSION_SVC_VAL \
	BT_UUID_128_ENCODE(0x00000005, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_VERSION_VAL \
	BT_UUID_128_ENCODE(0x00000006, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_DATA_SVC_VAL \
	BT_UUID_128_ENCODE(0x00000007, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_DATA_VAL \
	BT_UUID_128_ENCODE(0x00000008, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_DATA_CCC_VAL \
	BT_UUID_128_ENCODE(0x00000009, 0x7669, 0x6163, 0x616d, 0x2d63616c6563)

#define BT_UUID_BATTERY_SVC	BT_UUID_DECLARE_128(BT_UUID_BATTERY_SVC_VAL)
#define BT_UUID_BATTERY		BT_UUID_DECLARE_128(BT_UUID_BATTERY_VAL)
#define BT_UUID_TEMPERATURE_SVC BT_UUID_DECLARE_128(BT_UUID_TEMPERATURE_SVC_VAL)
#define BT_UUID_DATA_SVC	BT_UUID_DECLARE_128(BT_UUID_DATA_SVC_VAL)
#define BT_UUID_DATA		BT_UUID_DECLARE_128(BT_UUID_DATA_VAL)
#define BT_UUID_DATA_CCC	BT_UUID_DECLARE_128(BT_UUID_DATA_CCC_VAL)
#define BT_UUID_VERSION_SVC	BT_UUID_DECLARE_128(BT_UUID_VERSION_SVC_VAL)
#define BT_UUID_VERSION		BT_UUID_DECLARE_128(BT_UUID_VERSION_VAL)

/**
 * @brief Initializes the BLE subsystem and configures device settings
 * 
 * This function sets up the BLE stack by:
 * - Retrieving unique device ID from hardware
 * - Generating device name with device ID suffix
 * - Enabling Bluetooth with default configuration
 * - Loading stored settings from flash
 * - Setting the device name for advertising
 * 
 * This must be called before any other BLE operations.
 */
void ble_setup(void);

/**
 * @brief Starts BLE advertising to make device discoverable
 * 
 * This function begins broadcasting advertising packets containing the device
 * name and service flags. Central devices can discover and connect to this
 * peripheral while advertising is active.
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_start_advertising(void);

/**
 * @brief Stops BLE advertising to make device non-discoverable
 * 
 * This function stops broadcasting advertising packets, making the device
 * no longer discoverable by central devices. Existing connections remain
 * active.
 * 
 * @return 0 on success, negative error code on failure
 */
int ble_stop_advertising(void);

/**
 * @brief Sends current sensor data via BLE notification
 * 
 * This function transmits the current temperature and battery readings
 * to a connected BLE client using GATT notifications. The data is only
 * sent if a client is connected and has subscribed to notifications.
 * 
 * @return 0 on success, -EACCES if not subscribed, or other negative error code
 */
int ble_send_data(void);

/**
 * @brief Checks if a BLE connection is currently active
 * 
 * This function returns the current BLE connection status by checking
 * if there is an active connection reference stored. This can be used
 * to determine if sensor data transmission is possible.
 * 
 * @return true if connected to a BLE central device, false otherwise
 */
bool ble_is_connected(void);

/**
 * @brief Checks if a BLE client has subscribed to data notifications
 * 
 * This function returns the current subscription status for the data
 * characteristic. Data notifications are only sent when a client has
 * enabled them by writing to the Client Characteristic Configuration.
 * 
 * @return true if notifications are enabled, false otherwise
 */
bool ble_is_subscribed(void);
#endif
