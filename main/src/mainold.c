
#include "esp_log.h"
#include "esp_hidd_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_gap_bt_api.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define BOOT_PROTO_MOUSE_RPT_ID 0x02
typedef struct
{
    esp_hidd_app_param_t app_param;
    esp_hidd_qos_param_t both_qos;
    uint8_t protocol_mode;
    SemaphoreHandle_t mouse_mutex;
    xTaskHandle mouse_task_hdl;
    uint8_t buffer[4];
    int8_t x_dir;
} local_param_t;

static local_param_t s_local_param = {0};

SemaphoreHandle_t auto_reconnect_mutex;
xTaskHandle auto_reconnect_task_handle;

// bool check_report_id_type(uint8_t report_id, uint8_t report_type)
// {
//     bool ret = false;
//     xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);
//     do {
//         if (report_type != ESP_HIDD_REPORT_TYPE_INPUT) {
//             break;
//         }
//         if (s_local_param.protocol_mode == ESP_HIDD_BOOT_MODE) {
//             if (report_id == BOOT_PROTO_MOUSE_RPT_ID) {
//                 ret = true;
//                 break;
//             }
//         } else {
//             if (report_id == 0) {
//                 ret = true;
//                 break;
//             }
//         }
//     } while (0);

//     if (!ret) {
//         if (s_local_param.protocol_mode == ESP_HIDD_BOOT_MODE) {
//             esp_bt_hid_device_report_error(ESP_HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
//         } else {
//             esp_bt_hid_device_report_error(ESP_HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
//         }
//     }
//     xSemaphoreGive(s_local_param.mouse_mutex);
//     return ret;
// }

// send the buttons, change in x, and change in y
// void send_mouse(uint8_t buttons, char dx, char dy, char wheel)
// {
//     xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);
//     if (s_local_param.protocol_mode ==  ESP_HIDD_REPORT_MODE) {
//         s_local_param.buffer[0] = buttons;
//         s_local_param.buffer[1] = dx;
//         s_local_param.buffer[2] = dy;
//         s_local_param.buffer[3] = wheel;
//         esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, 0x00, 4, s_local_param.buffer);
//     } else if (s_local_param.protocol_mode == ESP_HIDD_BOOT_MODE) {
//         s_local_param.buffer[0] = buttons;
//         s_local_param.buffer[1] = dx;
//         s_local_param.buffer[2] = dy;
//         esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, BOOT_PROTO_MOUSE_RPT_ID, 3, s_local_param.buffer);
//     }
//     xSemaphoreGive(s_local_param.mouse_mutex);
// }

// move the mouse left and right
// void mouse_move_task(void* pvParameters) {
//     const char* TAG = "mouse_move_task";

//     ESP_LOGI(TAG, "starting");
//     for(;;) {
//         s_local_param.x_dir = 1;
//         int8_t step = 10;
//         for (int i = 0; i < 2; i++) {
//             xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);
//             s_local_param.x_dir *= -1;
//             xSemaphoreGive(s_local_param.mouse_mutex);
//             for (int j = 0; j < 100; j++) {
//                 send_mouse(0, s_local_param.x_dir * step, 0, 0);
//                 vTaskDelay(50 / portTICK_PERIOD_MS);
//             }
//         }
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

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

static void print_bt_address(void) {
    const char* TAG = "bt_address";
    const uint8_t* bd_addr;

    bd_addr = esp_bt_dev_get_address();
    ESP_LOGI(TAG, "my bluetooth address is %02X:%02X:%02X:%02X:%02X:%02X",
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
}

//a generic mouse descriptor
uint8_t hid_descriptor_mouse_boot_mode[] = {
    0x0
};
int hid_descriptor_mouse_boot_mode_len = sizeof(hid_descriptor_mouse_boot_mode);

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    const char* TAG = "esp_bt_gap_cb";
    ESP_LOGI(TAG, "event: %d, param: %p", event, param);
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;
    default:
        ESP_LOGI(TAG, "event: %d", event);
        break;
    }
    return;
}

uint8_t last_connected_device[ESP_BD_ADDR_LEN] = {0};

void auto_reconnect_task(void *pvParameters) {
    const char *TAG = "auto_reconnect_task";
    ESP_LOGI(TAG, "task started");
    while (1) {
        ESP_LOGI(TAG, "taking semaphore");
        xSemaphoreTake(auto_reconnect_mutex, portMAX_DELAY);
        ESP_LOGI(TAG, "took semaphore");
        if (hd_cb.device.conn.conn_state == 0) {
            ESP_LOGI(TAG, "trying to reconnect to last connected device %02x:%02x:%02x:%02x:%02x:%02x", last_connected_device[0],
                            last_connected_device[1], last_connected_device[2], last_connected_device[3], last_connected_device[4],
                            last_connected_device[5]);
            esp_bt_hid_device_connect(last_connected_device);
            ESP_LOGI(TAG, "esp_bt_hid_device_connect returned");
        }
        xSemaphoreGive(auto_reconnect_mutex);
        ESP_LOGI(TAG, "gave semaphore");

        vTaskDelay(5000 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "taking semaphore 2");
        xSemaphoreTake(auto_reconnect_mutex, portMAX_DELAY);
        ESP_LOGI(TAG, "took semaphore 2");
        bool was_still_connecting = hd_cb.device.conn.conn_state == 1;
        if (was_still_connecting) {
            ESP_LOGI(TAG, "committed bad");
            hd_cb.device.in_use = 0;
            hd_cb.device.conn.conn_state = 0;
        }
        xSemaphoreGive(auto_reconnect_mutex);
        ESP_LOGI(TAG, "gave semaphore 2");

        if (was_still_connecting) {
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }

        ESP_LOGI(TAG, "waited, retrying connection");
    }
}

void aaa(void *pvParameters) {
    const char *TAG = "aaa";
    while (1) {
        ESP_LOGI(TAG, "mood");
        ESP_LOGI(TAG, "idk device addr %02x:%02x:%02x:%02x:%02x:%02x", hd_cb.device.addr[0],
                            hd_cb.device.addr[1], hd_cb.device.addr[2], hd_cb.device.addr[3], hd_cb.device.addr[4],
                            hd_cb.device.addr[5]);
        ESP_LOGI(TAG, "connection state: %d", hd_cb.device.conn.conn_state);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void bt_app_task_start_up(void)
{
    auto_reconnect_mutex = xSemaphoreCreateMutex();
    xTaskCreate(auto_reconnect_task, "auto_reconnect_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, &auto_reconnect_task_handle);
    xTaskHandle surely;
    xTaskCreate(aaa, "aaa", 2 * 1024, NULL, configMAX_PRIORITIES - 3, &surely);
    return;
}

// void bt_app_task_shut_down(void)
// {
//     if (s_local_param.mouse_task_hdl) {
//         vTaskDelete(s_local_param.mouse_task_hdl);
//         s_local_param.mouse_task_hdl = NULL;
//     }

//     if (s_local_param.mouse_mutex) {
//         vSemaphoreDelete(s_local_param.mouse_mutex);
//         s_local_param.mouse_mutex = NULL;
//     }
//     return;
// }


void esp_bt_hidd_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    static const char* TAG = "esp_bt_hidd_cb";
    ESP_LOGI(TAG, "event: %d, param: %p", event, param);
    switch (event) {
    case ESP_HIDD_INIT_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_INIT_EVT");
        if (param->init.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "setting hid parameters");
            esp_bt_hid_device_register_app(&s_local_param.app_param, &s_local_param.both_qos, &s_local_param.both_qos);
        } else {
            ESP_LOGE(TAG, "init hidd failed!");
        }
        ESP_LOGI(TAG, "making ourselves connectable and discoverable IIIIIIIIII");
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
    case ESP_HIDD_DEINIT_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_DEINIT_EVT");
        break;
    case ESP_HIDD_REGISTER_APP_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_REGISTER_APP_EVT");
        if (param->register_app.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "setting hid parameters success!");
            // ESP_LOGI(TAG, "setting to connectable, discoverable");
            // esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            // if (param->register_app.in_use && param->register_app.bd_addr != NULL) {
            //     ESP_LOGI(TAG, "start virtual cable plug!");
            //     esp_log_buffer_hex(TAG, param->register_app.bd_addr, ESP_BD_ADDR_LEN);
            //     esp_bt_hid_device_connect(param->register_app.bd_addr);
            // }
        } else {
            ESP_LOGE(TAG, "setting hid parameters failed!");
        }
        break;
    case ESP_HIDD_UNREGISTER_APP_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_UNREGISTER_APP_EVT");
        if (param->unregister_app.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "unregister app success!");
        } else {
            ESP_LOGE(TAG, "unregister app failed!");
        }
        break;
    case ESP_HIDD_OPEN_EVT:
        if (param->open.status == ESP_HIDD_SUCCESS) {
            if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTING) {
                ESP_LOGI(TAG, "connecting...");
            } else if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED) {
                ESP_LOGI(TAG, "connected to %02x:%02x:%02x:%02x:%02x:%02x", param->open.bd_addr[0],
                         param->open.bd_addr[1], param->open.bd_addr[2], param->open.bd_addr[3], param->open.bd_addr[4],
                         param->open.bd_addr[5]);
                xSemaphoreTake(auto_reconnect_mutex, portMAX_DELAY);
                memcpy(last_connected_device, param->open.bd_addr, ESP_BD_ADDR_LEN);
                // bt_app_task_start_up();
                ESP_LOGI(TAG, "making self non-discoverable and non-connectable.");
                esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "open failed!");
        }
        break;
    case ESP_HIDD_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_CLOSE_EVT");
        if (param->close.status == ESP_HIDD_SUCCESS) {
            if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTING) {
                ESP_LOGI(TAG, "disconnecting...");
            } else if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "disconnected!");
                xSemaphoreGive(auto_reconnect_mutex);
                // bt_app_task_shut_down();
                ESP_LOGI(TAG, "making self discoverable and connectable again.");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "close failed!");
        }
        break;
    case ESP_HIDD_SEND_REPORT_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_SEND_REPORT_EVT id:0x%02x, type:%d", param->send_report.report_id,
                 param->send_report.report_type);
        break;
    case ESP_HIDD_REPORT_ERR_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_REPORT_ERR_EVT");
        break;
    case ESP_HIDD_GET_REPORT_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_GET_REPORT_EVT id:0x%02x, type:%d, size:%d", param->get_report.report_id,
                 param->get_report.report_type, param->get_report.buffer_size);
        // if (check_report_id_type(param->get_report.report_id, param->get_report.report_type)) {
        //     xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);
        //     if (s_local_param.protocol_mode == ESP_HIDD_REPORT_MODE) {
        //         esp_bt_hid_device_send_report(param->get_report.report_type, 0x00, 4, s_local_param.buffer);
        //     } else if (s_local_param.protocol_mode == ESP_HIDD_BOOT_MODE) {
        //         esp_bt_hid_device_send_report(param->get_report.report_type, 0x02, 3, s_local_param.buffer);
        //     }
        //     xSemaphoreGive(s_local_param.mouse_mutex);
        // } else {
        //     ESP_LOGE(TAG, "check_report_id failed!");
        // }
        break;
    case ESP_HIDD_SET_REPORT_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_SET_REPORT_EVT");
        break;
    case ESP_HIDD_SET_PROTOCOL_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_SET_PROTOCOL_EVT");
        // ESP_LOGI(TAG, "ESP_HIDD_SET_PROTOCOL_EVT");
        // if (param->set_protocol.protocol_mode == ESP_HIDD_BOOT_MODE) {
        //     ESP_LOGI(TAG, "  - boot protocol");
        //     xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);
        //     s_local_param.x_dir = -1;
        //     xSemaphoreGive(s_local_param.mouse_mutex);
        // } else if (param->set_protocol.protocol_mode == ESP_HIDD_REPORT_MODE) {
        //     ESP_LOGI(TAG, "  - report protocol");
        // }
        // xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);
        // s_local_param.protocol_mode = param->set_protocol.protocol_mode;
        // xSemaphoreGive(s_local_param.mouse_mutex);
        break;
    case ESP_HIDD_INTR_DATA_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_INTR_DATA_EVT");
        // ESP_LOGI(TAG, "ESP_HIDD_INTR_DATA_EVT");
        break;
    case ESP_HIDD_VC_UNPLUG_EVT:
        ESP_LOGI(TAG, "hid callback: ESP_HIDD_VC_UNPLUG_EVT");
        if (param->vc_unplug.status == ESP_HIDD_SUCCESS) {
            if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "disconnected!");
                // bt_app_task_shut_down();
                ESP_LOGI(TAG, "making self discoverable and connectable again.");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "close failed!");
        }
        break;
    default:
        break;
    }
}

void app_main(void) {
    const char* TAG = "app_main";
	esp_err_t ret;

    s_local_param.app_param.name = "Bomba";
    s_local_param.app_param.description = "Relówa";
    s_local_param.app_param.provider = "REL-DEVICE-69";
    s_local_param.app_param.subclass = ESP_HID_CLASS_UNKNOWN;
    s_local_param.app_param.desc_list = hid_descriptor_mouse_boot_mode;
    s_local_param.app_param.desc_list_len = hid_descriptor_mouse_boot_mode_len;
    memset(&s_local_param.both_qos, 0, sizeof(esp_hidd_qos_param_t)); // don't set the qos parameters
    s_local_param.protocol_mode = ESP_HIDD_REPORT_MODE;

	ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "initialize controller failed: %s\n",  esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG, "enable controller failed: %s\n",  esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "initialize bluedroid failed: %s\n",  esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "enable bluedroid failed: %s\n",  esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "gap register failed: %s\n", esp_err_to_name(ret));
        return;
    }


    ESP_LOGI(TAG, "setting device name");
    esp_bt_dev_set_device_name("ES-69420621-x88gdu");

    ESP_LOGI(TAG, "setting cod major, peripheral");
    esp_bt_cod_t cod;
    cod.major = ESP_BT_COD_MAJOR_DEV_TOY;
    esp_bt_gap_set_cod(cod ,ESP_BT_SET_COD_MAJOR_MINOR);

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "register hid device callback");
    esp_bt_hid_device_register_callback(esp_bt_hidd_cb);

    ESP_LOGI(TAG, "starting hid device");
	esp_bt_hid_device_init();

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    bt_app_task_start_up();
    print_bt_address();
	ESP_LOGI(TAG, "exiting");
}
