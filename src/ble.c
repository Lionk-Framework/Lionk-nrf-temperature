#include "ble.h"
#include "sensor.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/settings/settings.h>
#include "version.h"

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

static bool subscribed = false;

static ssize_t read_version(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset);

static ssize_t read_attribute(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset);

static void notification_ccc_changed(const struct bt_gatt_attr *attr,
				     uint16_t value);

static char device_name[CONFIG_BT_DEVICE_NAME_MAX];

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, device_name, sizeof(device_name))
};
static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY), 800, 16384,
	NULL);

static struct bt_conn *current_connection = NULL;
static struct bt_gatt_exchange_params exchange_params;

BT_GATT_SERVICE_DEFINE(battery_svc,
		       BT_GATT_PRIMARY_SERVICE(BT_UUID_BATTERY_SVC),
		       BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY,
					      BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_attribute,
					      NULL, &sensor_data.battery_mv));

BT_GATT_SERVICE_DEFINE(temperature_svc,
		       BT_GATT_PRIMARY_SERVICE(BT_UUID_TEMPERATURE_SVC),
		       BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
					      BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_attribute,
					      NULL, &sensor_data.temperature));

BT_GATT_SERVICE_DEFINE(data_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DATA_SVC),
		       BT_GATT_CHARACTERISTIC(BT_UUID_DATA, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL,
					      NULL),
		       BT_GATT_CCC(notification_ccc_changed,
				   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );

BT_GATT_SERVICE_DEFINE(
	version_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_VERSION_SVC),
	BT_GATT_CHARACTERISTIC(BT_UUID_VERSION, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_version, NULL, NULL));

typedef union {
	uint64_t id;
	uint8_t buffer[sizeof(uint64_t)];
} id_union_t;

static ssize_t read_version(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, VERSION,
				 strlen(VERSION));
}

static ssize_t read_attribute(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const uint16_t *value = (uint16_t *)attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

static void notification_ccc_changed(const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	subscribed = value == BT_GATT_CCC_NOTIFY;
	LOG_INF("Notifications %s", subscribed ? "enabled" : "disabled");
}
static void update_phy(struct bt_conn *conn)
{
	int err;
	const struct bt_conn_le_phy_param phy = {
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		.pref_tx_phy = BT_GAP_LE_PHY_CODED,
	};
	err = bt_conn_le_phy_update(conn, &phy);
	if (err) {
		LOG_ERR("Couldn't set phy settings (err %d)", err);
	}
}
static void update_data_length(struct bt_conn *conn)
{
	int err;
	struct bt_conn_le_data_len_param my_data_len = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};
	err = bt_conn_le_data_len_update(conn, &my_data_len);
	if (err) {
		LOG_ERR("data_len_update failed (err %d)", err);
	}
}
static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
	if (!att_err) {
		uint16_t payload_mtu = bt_gatt_get_mtu(conn) -
				       3; // 3 bytes used for Attribute headers.
		LOG_INF("New MTU: %d bytes", payload_mtu);
	}
}
static void update_mtu(struct bt_conn *conn)
{
	int err;
	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
		return;
	}
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;
	bt_conn_get_info(conn, &info);
	current_connection = bt_conn_ref(conn);
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	double connection_interval = info.le.interval * 1.25;
	uint16_t supervision_timeout = info.le.timeout * 10;

	LOG_INF("Connected to %s, interval %.2f ms, latency %d intervals, timeout %d ms",
		addr, connection_interval, info.le.latency,
		supervision_timeout);

	update_phy(conn);
	update_data_length(conn);
	update_mtu(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(conn);
	current_connection = NULL;
	LOG_INF("Disconnected (reason 0x%02x)", reason);
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	LOG_INF("Connection parameters updated: interval %.2f ms, latency %d, timeout %d ms",
		interval * 1.25, latency, timeout * 10);
}

static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	// PHY Updated
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
		LOG_INF("PHY updated. New PHY: 1M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
		LOG_INF("PHY updated. New PHY: 2M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
		LOG_INF("PHY updated. New PHY: Long Range");
	}
}
static void le_data_len_updated(struct bt_conn *conn,
				struct bt_conn_le_data_len_info *info)
{
	uint16_t tx_len = info->tx_max_len;
	uint16_t tx_time = info->tx_max_time;
	uint16_t rx_len = info->rx_max_len;
	uint16_t rx_time = info->rx_max_time;
	LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us",
		tx_len, rx_len, tx_time, rx_time);
}

void ble_setup(void)
{
	id_union_t device_id;
	ssize_t device_id_len = hwinfo_get_device_id(device_id.buffer,
						     sizeof(device_id.buffer));

	__ASSERT(device_id_len >= 0, "Couldn't get device id len (%d)",
		 device_id_len);

	sprintf(device_name, CONFIG_BT_DEVICE_NAME, device_id.id);
	int err = bt_enable(NULL);
	settings_load();
	__ASSERT(err == 0, "Couldn't enable bluetooth");

	bt_set_name(device_name);

	LOG_INF("Device ID: %llx", device_id.id);
	LOG_INF("Device Name: %s", device_name);
	LOG_INF("Bluetooth initialized");
}
int ble_start_advertising(void)
{
	return bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
}

int ble_stop_advertising(void)
{
	return bt_le_adv_stop();
}

static bool build_sensor_data_buffer(const sensor_data_t *data, uint8_t *buf,
				     uint16_t len)
{
	if (len < 5) {
		return false;
	}

	buf[0] = 0;
	buf[1] = (data->temperature & 0xFF00) >> 8;
	buf[2] = (data->temperature & 0xFF);
	buf[3] = (data->battery_mv & 0xFF00) >> 8;
	buf[4] = (data->battery_mv & 0xFF);
	return true;
}
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_ERR("Security failed: %s level %u err %d", addr, level,
			err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
	.le_data_len_updated = le_data_len_updated,
	.security_changed = security_changed,
};

bool ble_is_subscribed(void)
{
	return subscribed;
}
int ble_send_data(void)
{
	if (!subscribed) {
		return -EACCES;
	}
	uint8_t buffer[5];

	build_sensor_data_buffer(&sensor_data, buffer, sizeof(buffer));
	return bt_gatt_notify(current_connection,
			      &data_svc.attrs[data_svc.attr_count - 2], buffer,
			      sizeof(buffer));
}
bool ble_is_connected(void)
{
	return current_connection != NULL;
}
