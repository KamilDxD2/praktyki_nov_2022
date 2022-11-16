#include <stdlib.h>
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_gap_bt_api.h"

#include "main.h"
#include "hid.h"
#include "../debug.h"

static const char *default_pin = "1234123412341234";

static bool initialized = false;
static char* device_name = NULL;

static void bluetooth_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            DT_INFO("Authentication completed with device %s", param->auth_cmpl.device_name);
        } else {
            DT_WARNING("Authentication failed with device %s with error code %d", param->auth_cmpl.device_name, param->auth_cmpl.stat);
        }
        break;
    case ESP_BT_GAP_PIN_REQ_EVT: {
        const int pin_length = param->pin_req.min_16_digit ? 16 : 4;
        esp_bt_pin_code_t pin_code;
        memcpy(pin_code, default_pin, pin_length);
        DT_INFO("Use code: %.*s for pairing with " DT_BT_ADDR_FORMAT, pin_length, pin_code, DT_BT_ADDR(param->pin_req.bda));
        esp_bt_gap_pin_reply(param->pin_req.bda, true, pin_length, pin_code);
        break;
    }
#ifdef CONFIG_BT_SSP_ENABLED
    case ESP_BT_GAP_CFM_REQ_EVT:
        DT_INFO("Please compare the PIN code: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        DT_INFO("Use this passkey to connect: %d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        DT_INFO("Please enter the passkey");
        break;
#endif
    case ESP_BT_GAP_MODE_CHG_EVT:
        DT_DEBUG("GAP mode changed: %d", param->mode_chg.mode);
        break;
    default:
        break;
    }
}

int bluetooth_init(void) {
    esp_err_t ret;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        DT_ERROR("Failed to initialize controller: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK) {
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

    if ((ret = esp_bt_gap_register_callback(bluetooth_gap_cb)) != ESP_OK) {
        DT_ERROR("Failed to register GAP callback: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_bt_hid_device_register_callback(bluetooth_hid_cb)) != ESP_OK) {
        DT_ERROR("Failed to register HID device callback: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    if ((ret = esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_VARIABLE, 0, NULL)) != ESP_OK) {
        DT_ERROR("Failed to set GAP PIN parameters: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

#ifdef CONFIG_BT_SSP_ENABLED
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    if ((ret = esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap))) != ESP_OK) {
        DT_ERROR("Failed to set SSP parameters: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }
#endif

    esp_bt_cod_t cod;
    cod.major = ESP_BT_COD_MAJOR_DEV_PERIPHERAL;
    if ((ret = esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR)) != ESP_OK) {
        DT_WARNING("Failed to set Class of Device: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
    }

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
        const int id_len = 8;
        char template[] = "DeskThing-Unknown-";
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

    esp_err_t ret;

    if ((ret = esp_bt_dev_set_device_name(device_name)) != ESP_OK) {
        DT_WARNING("Failed to set device name: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
    }

    if ((ret = esp_bt_hid_device_init()) != ESP_OK) {
        DT_ERROR("Failed to initialize HID device: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return 1;
    }

    BaseType_t err;
    if ((err = bluetooth_start_autoreconnect_task()) != pdPASS) {
        DT_ERROR("Failed to start bluetooth autoreconnect task, error code: %d", err);
        return 1;
    }

    return 0;
}
