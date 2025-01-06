/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/logging/log.h>
#include "esl_packets.h"

LOG_MODULE_REGISTER(peripheral_sync, LOG_LEVEL_DBG);

#define NAME_LEN 30

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);

static struct bt_conn *default_conn;
static struct bt_le_per_adv_sync *default_sync;
static struct __packed {
	uint8_t subevent;
	uint8_t response_slot;
} pawr_timing;

static void sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	struct bt_le_per_adv_sync_subevent_params params;
	uint8_t subevents[1];
	char le_addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_INF("Synced to %s with %d subevents", le_addr, info->num_subevents);

	default_sync = sync;

	params.properties = 0;
	params.num_subevents = 1;
	params.subevents = subevents;
	subevents[0] = pawr_timing.subevent;

	err = bt_le_per_adv_sync_subevent(sync, &params);
	if (err) {
		LOG_ERR("Failed to set subevents to sync to (err %d)", err);
	} else {
		LOG_INF("Changed sync to subevent %d", subevents[0]);
	}

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_INF("Sync terminated (reason %d)", info->reason);

	default_sync = NULL;

	k_sem_give(&sem_per_sync_lost);
}

static bool print_ad_field(struct bt_data *data, void *user_data)
{
	ARG_UNUSED(user_data);

	char data_type[6];
	snprintf(data_type, 6, "0x%02X:", data->type);
//	LOG_HEXDUMP_DBG(data->data, data->data_len, data_type);

	return true;
}

/*
int bt_le_per_adv_set_response_data(struct bt_le_per_adv_sync *per_adv_sync,
				    const struct bt_le_per_adv_response_params *params,
				    const struct net_buf_simple *data);
*/

static struct bt_le_per_adv_response_params rsp_params;

NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, 512);

extern struct zbus_channel sensor_chan;

static void recv_cb(struct bt_le_per_adv_sync *sync,
            const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
    int err = 0;
    struct esl_sensor_reading sensor_reading = {0};

    err = zbus_chan_read(&sensor_chan, &sensor_reading, K_NO_WAIT);
    if (err == 0) {
        LOG_INF("Zbus chan read: %.2f %.2f", sensor_reading.temperature, sensor_reading.humidity);
        
        /* Prepare response buffer with properly formatted advertising data */
        net_buf_simple_reset(&rsp_buf);
        
        /* Add sensor data as manufacturer specific data */
        net_buf_simple_add_u8(&rsp_buf, sizeof(struct esl_sensor_reading) + 1); // Length
        net_buf_simple_add_u8(&rsp_buf, BT_DATA_MANUFACTURER_DATA);  // AD type
        net_buf_simple_add_mem(&rsp_buf, &sensor_reading, sizeof(struct esl_sensor_reading));

        /* Set up response parameters */
        rsp_params.request_event = info->periodic_event_counter;
        rsp_params.request_subevent = info->subevent;
        rsp_params.response_subevent = info->subevent;
        rsp_params.response_slot = pawr_timing.response_slot;

        LOG_INF("Sending sensor data: temp=%.2f, humidity=%.2f in subevent %d, slot %d", 
                sensor_reading.temperature, 
                sensor_reading.humidity,
                info->subevent,
                pawr_timing.response_slot);

        err = bt_le_per_adv_set_response_data(sync, &rsp_params, &rsp_buf);
        if (err) {
            LOG_ERR("Failed to send response (err %d)", err);
        }
    } else if (buf) {
        LOG_INF("Received empty indication: subevent %d", info->subevent);
    } else {
        LOG_ERR("Failed to receive indication: subevent %d", info->subevent);
    }
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
};

static const struct bt_uuid_128 pawr_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));
static const struct bt_uuid_128 pawr_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static ssize_t write_timing(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(pawr_timing)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&pawr_timing, buf, len);

	LOG_INF("New timing: subevent %d, response slot %d", pawr_timing.subevent,
	       pawr_timing.response_slot);

	struct bt_le_per_adv_sync_subevent_params params;
	uint8_t subevents[1];
	int err;

	params.properties = 0;
	params.num_subevents = 1;
	params.subevents = subevents;
	subevents[0] = pawr_timing.subevent;

	if (default_sync) {
		err = bt_le_per_adv_sync_subevent(default_sync, &params);
		if (err) {
			LOG_ERR("Failed to set subevents to sync to (err %d)", err);
		} else {
			LOG_INF("Changed sync to subevent %d", subevents[0]);
		}
	} else {
		LOG_INF("Not synced yet");
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(pawr_svc, BT_GATT_PRIMARY_SERVICE(&pawr_svc_uuid.uuid),
		       BT_GATT_CHARACTERISTIC(&pawr_char_uuid.uuid, BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL, write_timing,
					      &pawr_timing),
);

void connected(struct bt_conn *conn, uint8_t err)
{
	LOG_INF("Connected, err 0x%02X %s", err, bt_hci_err_to_str(err));

	if (err) {
		default_conn = NULL;

		return;
	}

	default_conn = bt_conn_ref(conn);
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(default_conn);
	default_conn = NULL;

	LOG_INF("Disconnected, reason 0x%02X %s", reason, bt_hci_err_to_str(reason));
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void)
{
	struct bt_le_per_adv_sync_transfer_param past_param;
	int err;

	LOG_INF("Starting Periodic Advertising with Responses Synchronization Demo");

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);

		return 0;
	}

	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	past_param.skip = 1;
	past_param.timeout = 1000; /* 10 seconds */
	past_param.options = BT_LE_PER_ADV_SYNC_TRANSFER_OPT_NONE;
	err = bt_le_per_adv_sync_transfer_subscribe(NULL, &past_param);
	if (err) {
		LOG_ERR("PAST subscribe failed (err %d)", err);

		return 0;
	}

	do {
		err = bt_le_adv_start(
			BT_LE_ADV_PARAM(BT_LE_ADV_OPT_ONE_TIME | BT_LE_ADV_OPT_CONNECTABLE,
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL),
			ad, ARRAY_SIZE(ad), NULL, 0);
		if (err && err != -EALREADY) {
			LOG_ERR("Advertising failed to start (err %d)", err);
			bt_le_adv_stop();
			k_msleep(1000);
		}

		LOG_INF("Waiting for periodic sync...");
		err = k_sem_take(&sem_per_sync, K_FOREVER);
		if (err) {
			LOG_ERR("Timed out while synchronizing");

			continue;
		}

		LOG_INF("Periodic sync established.");

		err = k_sem_take(&sem_per_sync_lost, K_FOREVER);
		if (err) {
			LOG_ERR("failed (err %d)", err);

			return 0;
		}

		LOG_INF("Periodic sync lost.");
	} while (true);

	return 0;
}
