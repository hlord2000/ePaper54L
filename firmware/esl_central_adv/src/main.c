/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define NUM_RSP_SLOTS 10
#define NUM_SUBEVENTS 5
#define PACKET_SIZE   32
#define NAME_LEN      30

static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);

static struct bt_uuid_128 pawr_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));
static uint16_t pawr_attr_handle;

/* Periodic advertising interval in 1.25ms units */
#define PER_ADV_INT_MIN 0xFF
/* Periodic advertising interval in 1.25ms units */
#define PER_ADV_INT_MAX 0xFF
/* Periodic advertising subevent interval in 1.25ms units */
#define SUBEVENT_INTERVAL 0x33
/* Periodic advertising subevent response delay in 1.25ms units */
#define RESPONSE_SLOT_DELAY 0x5
/* Periodic advertising response slot spacing in 0.125ms units */
#define RESPONSE_SLOT_SPACING 0x20

/* Validate interval range overlap with Controller */
BUILD_ASSERT(PER_ADV_INT_MAX >= BT_GAP_PER_ADV_MIN_INTERVAL,
            "Periodic advertising maximum interval must overlap with Controller's minimum.");
BUILD_ASSERT(PER_ADV_INT_MIN <= BT_GAP_PER_ADV_MAX_INTERVAL,
            "Periodic advertising minimum interval must overlap with Controller's maximum.");

/* Validate that interval min <= max */
BUILD_ASSERT(PER_ADV_INT_MIN <= PER_ADV_INT_MAX,
            "Periodic advertising minimum interval must not exceed maximum interval");

/* If num_subevents is not 0, validate subevent interval */
#if (NUM_SUBEVENTS > 0)
BUILD_ASSERT(SUBEVENT_INTERVAL <= (PER_ADV_INT_MIN / NUM_SUBEVENTS),
            "Subevent interval must be <= periodic advertising interval min divided by num subevents");
#endif

/* Validate response slot delay against subevent interval */
#if (NUM_SUBEVENTS > 0)
BUILD_ASSERT(RESPONSE_SLOT_DELAY < SUBEVENT_INTERVAL,
            "Response slot delay must be less than subevent interval");
#endif

/* Validate response slot spacing when multiple response slots are used */
#if (NUM_RSP_SLOTS > 1)
BUILD_ASSERT(RESPONSE_SLOT_SPACING <= (10 * (SUBEVENT_INTERVAL - RESPONSE_SLOT_DELAY) / NUM_RSP_SLOTS),
            "Response slot spacing exceeds maximum allowed value");
#endif

/* Range checks for individual parameters */
#if (NUM_SUBEVENTS > 0)
BUILD_ASSERT(SUBEVENT_INTERVAL >= 0x6 && SUBEVENT_INTERVAL <= 0xFF,
            "Subevent interval not in valid range (0x6 to 0xFF)");
#endif

#if (NUM_RSP_SLOTS > 0)
BUILD_ASSERT(RESPONSE_SLOT_DELAY >= 0x1 && RESPONSE_SLOT_DELAY <= 0xFE,
            "Response slot delay not in valid range (0x1 to 0xFE)");

BUILD_ASSERT(RESPONSE_SLOT_SPACING >= 0x2 && RESPONSE_SLOT_SPACING <= 0xFF,
            "Response slot spacing not in valid range (0x2 to 0xFF)");
#endif

static const struct bt_le_per_adv_param per_adv_params = {
	.interval_min = PER_ADV_INT_MIN,
	.interval_max = PER_ADV_INT_MAX,
	.options = 0,
	.num_subevents = NUM_SUBEVENTS,
	.subevent_interval = SUBEVENT_INTERVAL,
	.response_slot_delay = RESPONSE_SLOT_DELAY,
	.response_slot_spacing = RESPONSE_SLOT_SPACING,
	.num_response_slots = NUM_RSP_SLOTS,
};

static struct bt_le_per_adv_subevent_data_params subevent_data_params[NUM_SUBEVENTS];
static struct net_buf_simple bufs[NUM_SUBEVENTS];
static uint8_t backing_store[NUM_SUBEVENTS][PACKET_SIZE];

BUILD_ASSERT(ARRAY_SIZE(bufs) == ARRAY_SIZE(subevent_data_params));
BUILD_ASSERT(ARRAY_SIZE(backing_store) == ARRAY_SIZE(subevent_data_params));

static uint8_t counter;

static void request_cb(struct bt_le_ext_adv *adv, const struct bt_le_per_adv_data_request *request)
{
	int err;
	uint8_t to_send;
	struct net_buf_simple *buf;

	to_send = MIN(request->count, ARRAY_SIZE(subevent_data_params));

	for (size_t i = 0; i < to_send; i++) {
		buf = &bufs[i];
		buf->data[buf->len - 1] = counter++;

		subevent_data_params[i].subevent =
			(request->start + i) % per_adv_params.num_subevents;
		subevent_data_params[i].response_slot_start = 0;
		subevent_data_params[i].response_slot_count = NUM_RSP_SLOTS;
		subevent_data_params[i].data = buf;
	}

	err = bt_le_per_adv_set_subevent_data(adv, to_send, subevent_data_params);
	if (err) {
		LOG_ERR("Failed to set subevent data (err %d)", err);
	} else {
		LOG_INF("Subevent data set %d", counter);
	}
}

static bool print_ad_field(struct bt_data *data, void *user_data)
{
	ARG_UNUSED(user_data);

	char data_type[6];
	snprintf(data_type, 6, "0x%02X:", data->type);
	LOG_HEXDUMP_DBG(data->data, data->data_len, data_type);

	return true;
}

static struct bt_conn *default_conn;

static void response_cb(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
		     struct net_buf_simple *buf)
{
	if (buf) {
		LOG_INF("Response: subevent %d, slot %d", info->subevent, info->response_slot);
		bt_data_parse(buf, print_ad_field, NULL);
	}
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.pawr_data_request = request_cb,
	.pawr_response = response_cb,
};

void connected_cb(struct bt_conn *conn, uint8_t err)
{
	LOG_INF("Connected (err 0x%02X)", err);

	__ASSERT(conn == default_conn, "Unexpected connected callback");

	if (err) {
		LOG_ERR("Connection error 0x%02X", err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02X %s", reason, bt_hci_err_to_str(reason));

	bt_conn_unref(default_conn);
	default_conn = NULL;

	k_sem_give(&sem_disconnected);
}

void remote_info_available_cb(struct bt_conn *conn, struct bt_conn_remote_info *remote_info)
{
	/* Need to wait for remote info before initiating PAST */
	k_sem_give(&sem_connected);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.remote_info_available = remote_info_available_cb,
};

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	int err;

	if (default_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	(void)memset(name, 0, sizeof(name));
	bt_data_parse(ad, data_cb, name);

	if (strcmp(name, "PAwR sync sample")) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		LOG_ERR("Create conn to %s failed (%u)", addr_str, err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	LOG_INF("Discovery: attr %p", attr);

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	LOG_INF("UUID %s", str);

	if (!bt_uuid_cmp(chrc->uuid, &pawr_char_uuid.uuid)) {
		pawr_attr_handle = chrc->value_handle;

		LOG_INF("Characteristic handle: %d", pawr_attr_handle);

		k_sem_give(&sem_discovered);
	}

	return BT_GATT_ITER_STOP;
}

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err) {
		LOG_ERR("Write failed (err %d)", err);

		return;
	}

	k_sem_give(&sem_written);
}

void init_bufs(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(backing_store); i++) {
		backing_store[i][0] = ARRAY_SIZE(backing_store[i]) - 1;
		backing_store[i][1] = BT_DATA_MANUFACTURER_DATA;
		backing_store[i][2] = 0x59; /* Nordic */
		backing_store[i][3] = 0x00;

		net_buf_simple_init_with_data(&bufs[i], &backing_store[i],
					      ARRAY_SIZE(backing_store[i]));
	}
}

#define MAX_SYNCS (NUM_SUBEVENTS * NUM_RSP_SLOTS)
struct pawr_timing {
	uint8_t subevent;
	uint8_t response_slot;
} __packed;

static uint8_t num_synced;

int main(void)
{
	int err;
	struct bt_le_ext_adv *pawr_adv;
	struct bt_gatt_discover_params discover_params;
	struct bt_gatt_write_params write_params;
	struct pawr_timing sync_config;

	init_bufs();

	LOG_INF("Starting Periodic Advertising Demo");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &pawr_adv);
	if (err) {
		LOG_ERR("Failed to create advertising set (err %d)", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(pawr_adv, &per_adv_params);
	if (err) {
		LOG_ERR("Failed to set periodic advertising parameters (err %d)", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	LOG_INF("Start Periodic Advertising");
	err = bt_le_per_adv_start(pawr_adv);
	if (err) {
		LOG_ERR("Failed to enable periodic advertising (err %d)", err);
		return 0;
	}

	LOG_INF("Start Extended Advertising");
	err = bt_le_ext_adv_start(pawr_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_ERR("Failed to start extended advertising (err %d)", err);
		return 0;
	}

	while (num_synced < MAX_SYNCS) {
		/* Enable continuous scanning */
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, device_found);
		if (err) {
			LOG_ERR("Scanning failed to start (err %d)", err);
			return 0;
		}

		LOG_INF("Scanning successfully started");

		k_sem_take(&sem_connected, K_FOREVER);

		err = bt_le_per_adv_set_info_transfer(pawr_adv, default_conn, 0);
		if (err) {
			LOG_ERR("Failed to send PAST (err %d)", err);

			goto disconnect;
		}

		LOG_INF("PAST sent");

		discover_params.uuid = &pawr_char_uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			LOG_ERR("Discovery failed (err %d)", err);

			goto disconnect;
		}

		LOG_INF("Discovery started");

		err = k_sem_take(&sem_discovered, K_SECONDS(10));
		if (err) {
			LOG_ERR("Timed out during GATT discovery");

			goto disconnect;
		}

		sync_config.subevent = num_synced % NUM_SUBEVENTS;
		sync_config.response_slot = num_synced / NUM_RSP_SLOTS;
		num_synced++;

		write_params.func = write_func;
		write_params.handle = pawr_attr_handle;
		write_params.offset = 0;
		write_params.data = &sync_config;
		write_params.length = sizeof(sync_config);

		err = bt_gatt_write(default_conn, &write_params);
		if (err) {
			LOG_ERR("Write failed (err %d)", err);
			num_synced--;

			goto disconnect;
		}

		LOG_INF("Write started");

		err = k_sem_take(&sem_written, K_SECONDS(10));
		if (err) {
			LOG_ERR("Timed out during GATT write");
			num_synced--;

			goto disconnect;
		}

		LOG_INF("PAwR config written to sync %d, disconnecting", num_synced - 1);

disconnect:
		/* Adding delay (2ms * interval value, using 2ms intead of the 1.25ms
		 * used by controller) to ensure sync is established before
		 * disconnection.
		 */
		k_sleep(K_MSEC(per_adv_params.interval_max * 2));

		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			return 0;
		}

		k_sem_take(&sem_disconnected, K_FOREVER);
	}

	LOG_INF("Maximum numnber of syncs onboarded");

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
