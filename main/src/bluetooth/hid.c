#include <string.h>
#include <time.h>

#include "esp_gap_bt_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hid.h"
#include "../debug.h"

static uint8_t hid_device_descriptor[] = { 0x0 };

static esp_hidd_app_param_t app_param = {
    .name = "Dummy",
    .description = "Dummy device",
    .provider = "Dummy",
    .subclass = ESP_HID_CLASS_UNKNOWN,
    .desc_list = hid_device_descriptor,
    .desc_list_len = sizeof(hid_device_descriptor)
};
static esp_hidd_qos_param_t qos_param = {0};

static bluetooth_device_state_cb_t device_state_cb = NULL;

static esp_bd_addr_t last_connected_device;
static xTaskHandle autoreconnect_task = NULL;

typedef struct hid_conn {
#define HID_CONN_STATE_UNUSED               (0)
#define HID_CONN_STATE_CONNECTING_CTRL      (1)
#define HID_CONN_STATE_CONNECTING_INTR      (2)
#define HID_CONN_STATE_CONFIG               (3)
#define HID_CONN_STATE_CONNECTED            (4)
#define HID_CONN_STATE_DISCONNECTING        (5)
#define HID_CONN_STATE_SECURITY             (6)
#define HID_CONN_STATE_DISCONNECTING_CTRL   (7)
#define HID_CONN_STATE_DISCONNECTING_INTR   (8)

    uint8_t             conn_state;

#define HID_CONN_FLAGS_IS_ORIG              (0x01)
#define HID_CONN_FLAGS_HIS_CTRL_CFG_DONE    (0x02)
#define HID_CONN_FLAGS_MY_CTRL_CFG_DONE     (0x04)
#define HID_CONN_FLAGS_HIS_INTR_CFG_DONE    (0x08)
#define HID_CONN_FLAGS_MY_INTR_CFG_DONE     (0x10)
#define HID_CONN_FLAGS_ALL_CONFIGURED       (0x1E)        /* All the config done */
#define HID_CONN_FLAGS_CONGESTED            (0x20)
#define HID_CONN_FLAGS_INACTIVE             (0x40)

    uint8_t             conn_flags;

    uint8_t             ctrl_id;
    uint16_t            ctrl_cid;
    uint16_t            intr_cid;
    uint16_t            rem_mtu_size;
    uint16_t            disc_reason;                       /* Reason for disconnecting (for HID_HDEV_EVT_CLOSE) */
} tHID_CONN;

typedef struct device_ctb {
    bool in_use;
    uint8_t addr[ESP_BD_ADDR_LEN];
    uint8_t state;
    tHID_CONN conn;
    bool boot_mode;
    uint8_t idle_time;
} tHID_DEV_DEV_CTB;

typedef struct dev_ctb {
    tHID_DEV_DEV_CTB device;
} tHID_DEV_CTB;

tHID_DEV_CTB hd_cb;

static void bluetooth_autoreconnect_task(void *pvParameters) {
    while (true) {
        DT_INFO("waiting for notification");
        xTaskNotifyWait(0, UINT_MAX, NULL, portMAX_DELAY);
        DT_INFO("notification received");

        while (hd_cb.device.conn.conn_state != HID_CONN_STATE_CONNECTED) {
            DT_INFO("Reconnecting to last connected device " DT_BT_ADDR_FORMAT, DT_BT_ADDR(last_connected_device));
            esp_bt_hid_device_connect(last_connected_device);

            vTaskDelay(5000 / portTICK_PERIOD_MS);

            if (hd_cb.device.conn.conn_state != HID_CONN_STATE_CONNECTED) {
                hd_cb.device.in_use = false;
                hd_cb.device.conn.conn_state = HID_CONN_STATE_UNUSED;
                DT_INFO("Commited bad");

                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
        }
    }
}

BaseType_t bluetooth_start_autoreconnect_task(void) {
    return xTaskCreate(bluetooth_autoreconnect_task, "bt_autoreconnect", 2 * 1024, NULL, configMAX_PRIORITIES - 3, &autoreconnect_task);
}

void bluetooth_set_device_state_cb(bluetooth_device_state_cb_t cb) {
    device_state_cb = cb;
}

void bluetooth_set_last_connected_device(esp_bd_addr_t addr) {
    memcpy(last_connected_device, addr, ESP_BD_ADDR_LEN);
    if (autoreconnect_task != NULL) {
        xTaskNotifyGive(autoreconnect_task);
    }
}

void bluetooth_hid_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param) {
    switch (event) {
    case ESP_HIDD_INIT_EVT:
        if (param->init.status == ESP_HIDD_SUCCESS) {
            DT_INFO("Registering HID application");
            esp_bt_hid_device_register_app(&app_param, &qos_param, &qos_param);
        } else {
            DT_ERROR("HID init failed, error code: %d", param->init.status);
        }
        break;
    case ESP_HIDD_REGISTER_APP_EVT:
        if (param->register_app.status == ESP_HIDD_SUCCESS) {
            DT_INFO("HID application registered successfully, becoming discoverable");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        } else {
            DT_ERROR("Failed to register HID application, error code: %d", param->register_app.status);
        }
        break;
    case ESP_HIDD_OPEN_EVT:
        if (param->open.status == ESP_HIDD_SUCCESS) {
            if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTING) {
                DT_INFO("HID device connecting...");
            } else if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED) {
                DT_INFO("HID device connected to " DT_BT_ADDR_FORMAT, DT_BT_ADDR(param->open.bd_addr));
                memcpy(last_connected_device, param->open.bd_addr, ESP_BD_ADDR_LEN);
                esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
                if (device_state_cb != NULL) {
                    device_state_cb(BT_DEVICE_CONNECTED, param->open.bd_addr);
                }
            } else {
                DT_WARNING("HID device in unknown connection status: %d", param->open.conn_status);
            }
        } else {
            DT_ERROR("HID device open failed, error code: %d", param->open.status);
        }
        break;
    case ESP_HIDD_CLOSE_EVT:
        if (param->close.status == ESP_HIDD_SUCCESS) {
            if (param->open.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTING) {
                DT_INFO("HID device disconnecting...");
            } else if (param->open.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                DT_INFO("HID device disconnected");
                xTaskNotifyGive(autoreconnect_task);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
                if (device_state_cb != NULL) {
                    device_state_cb(BT_DEVICE_DISCONNECTED, last_connected_device);
                }
            } else {
                DT_WARNING("HID device in unknown connection status: %d", param->open.conn_status);
            }
        } else {
            DT_ERROR("HID device close failed, error code: %d", param->open.status);
        }
        break;
    case ESP_HIDD_VC_UNPLUG_EVT:
        DT_WARNING("Unimplemented ESP_HIDD_VC_UNPLUG_EVT handler");
        break;
    default:
        break;
    }
}