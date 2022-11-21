#include <stdlib.h>
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_hidd_prf_api.h"

#include "main.h"
#include "../debug.h"

static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,       //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x30,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static bool initialized = false;
static char* device_name = NULL;
static bluetooth_device_state_cb_t device_state_cb = NULL;

static void bluetooth_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        DT_INFO("Starting advertising");
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        DT_INFO("BLE security request from " DT_BT_ADDR_FORMAT, DT_BT_ADDR(param->ble_security.ble_req.bd_addr));
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        DT_INFO("BLE authentication completed with " DT_BT_ADDR_FORMAT, DT_BT_ADDR(param->ble_security.auth_cmpl.bd_addr));
        DT_INFO("Pair status: %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
            DT_INFO("Error code: %d", param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

static void bluetooth_hid_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param) {
    switch (event) {
    case ESP_HIDD_EVENT_REG_FINISH:
        if (param->init_finish.state == ESP_HIDD_INIT_OK) {
            DT_INFO("HID registration success");
            esp_ble_gap_set_device_name(device_name);
            esp_ble_gap_config_adv_data(&adv_data);
        } else {
            DT_ERROR("HID registration failed");
        }
        break;
    case ESP_HIDD_EVENT_BLE_CONNECT:
        DT_INFO("BLE device connected");
        if (device_state_cb) {
            device_state_cb(BT_DEVICE_CONNECTED);
        }
        break;
    case ESP_HIDD_EVENT_BLE_DISCONNECT:
        DT_INFO("BLE device disconnected");
        if (device_state_cb) {
            device_state_cb(BT_DEVICE_DISCONNECTED);
        }
        DT_INFO("Restarting advertising");
        esp_ble_gap_start_advertising(&adv_params);
        break;
    default:
        break;
    }
}

void bluetooth_set_device_state_cb(bluetooth_device_state_cb_t cb) {
    device_state_cb = cb;
}

int bluetooth_init(void) {
    esp_err_t ret;

    if ((ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        DT_ERROR("Failed to release unused BT classic memory: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        DT_ERROR("Failed to initialize controller: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK) {
        DT_ERROR("Failed to enable controller: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        DT_ERROR("Failed to initialize Bluedroid: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        DT_ERROR("Failed to enable Bluedroid: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_ble_gap_register_callback(bluetooth_gap_cb)) != ESP_OK) {
        DT_ERROR("Failed to register GAP callback: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_hidd_profile_init()) != ESP_OK) {
        DT_ERROR("Failed to initialize HID profile: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    initialized = true;
    return 0;
}

void bluetooth_set_device_name(const char *name) {
    if (device_name != NULL) {
        free(device_name);
    }
    size_t name_len = strlen(name) + 1;
    device_name = malloc(name_len);
    strcpy(device_name, name);
}

int bluetooth_start(void) {
    if (!initialized) {
        return 1;
    }

    if (device_name == NULL) {
        const int id_len = 4;
        char template[] = "DT-";
        char name[sizeof(template) + id_len];
        strcpy(name, template);
        int i;
        for (i = sizeof(template) - 1; i < sizeof(name) - 1; i++) {
            name[i] = esp_random() % 10 + '0';
        }
        name[i] = '\0';

        DT_WARNING("No device name set, defaulting to %s", name);
        bluetooth_set_device_name(name);
    }

    esp_hidd_register_callbacks(bluetooth_hid_cb);

    return 0;
}
