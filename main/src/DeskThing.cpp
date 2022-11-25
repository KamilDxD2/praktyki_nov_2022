#include "display/display_main.h"
#include "config/persistconfig.h"
#include "bluetooth/main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "debug.h"
#include "mesh/main.h"
#include "arduino_init.h"
#include "http/main.h"


enum class EmployeeState : uint8_t{
	AT_DESK = 1, AWAY, OUT_OF_OFFICE
};

uint32_t minutesSinceLastUpdate;
char* userFirstName;
char* userLastName;
uint32_t outOfOfficeTime = 60;
EmployeeState employeeState;
uint8_t canAcceptConfigChange;


xTimerHandle timerHandle;
xTaskHandle wifiTaskHandle;

String createStateJSON(){
	String buff;
	DynamicJsonDocument jsonDocument(1024);
	jsonDocument["firstName"] = userFirstName;
	jsonDocument["lastName"] = userLastName;
	jsonDocument["state"] = (uint8_t) employeeState;
	jsonDocument["minutes"] = minutesSinceLastUpdate;
	jsonDocument["id"] = getCurrentID();
	jsonDocument["configurable"] = canAcceptConfigChange;
	jsonDocument["outOfOfficeTime"] = outOfOfficeTime;
	serializeJson(jsonDocument, buff);
	return buff;
}

void handleStateChange(){
	const char* screenData = "This shouldn't happen";
	char buffer[20] = { 0 };

	switch(employeeState){
		case EmployeeState::AT_DESK:
			screenData = "At Desk";
			break;
		case EmployeeState::AWAY:
			sprintf(buffer, "Away %02d:%02d", minutesSinceLastUpdate / 60, minutesSinceLastUpdate % 60);
			screenData = buffer;
			break;
		case EmployeeState::OUT_OF_OFFICE:
			screenData = "OOF";
			break;
	}

	Display::DisplayUpdate update{
		.top = userFirstName,
		.main = userLastName,
		.bottom = (char*) screenData,
		.wifiIconVisible = canAcceptConfigChange
	};

	Display::queue_display_update(update);
	String stateString = createStateJSON();
	meshBroadcastState(stateString);
	broadcastStringToWS(stateString);
}

void timerCallback(void* pvParameters){
	++minutesSinceLastUpdate;
	if(minutesSinceLastUpdate > outOfOfficeTime){
		employeeState = EmployeeState::OUT_OF_OFFICE;
		handleStateChange();
		xTimerStop(timerHandle, portMAX_DELAY);
		return;
	}
	handleStateChange();
}

void loadConfig(void){
	if(Config::hasString("firstName") && Config::hasString("lastName")){
		userFirstName = Config::getString("firstName");
		userLastName = Config::getString("lastName");
	}else{
		userFirstName = (char*) "John";
		userLastName =  (char*) "Doe";
	}

	if(Config::hasUint32("oofTime")){
		outOfOfficeTime = Config::getUint32("oofTime");
	}
}

void wifiTimerTask(void* pvParameters){
	for(;;){
		// clear the notification if it was sent when the WiFi was active
        xTaskNotifyWait(0, UINT_MAX, NULL, 0);
        xTaskNotifyWait(0, UINT_MAX, NULL, portMAX_DELAY);
		canAcceptConfigChange = 1;
		handleStateChange();

		vTaskDelay(3 * 60 * 1000 / portTICK_PERIOD_MS);
		canAcceptConfigChange = 0;
		handleStateChange();
	}
}

void button_pressed(void *arg){
	BaseType_t wake_higher_priority = pdFALSE;
    xTaskNotifyFromISR(wifiTaskHandle, 0, eNoAction, &wake_higher_priority);
    portYIELD_FROM_ISR(wake_higher_priority);
}

void handleConfigurationChange(DynamicJsonDocument const& jsonDocument){
	if(!canAcceptConfigChange) return;
	if(jsonDocument.containsKey("firstName")){
		const char* firstName = jsonDocument["firstName"];
		Config::setString("firstName", firstName);
	}
	if(jsonDocument.containsKey("lastName")){
		const char* firstName = jsonDocument["lastName"];
		Config::setString("lastName", firstName);
	}
	if(jsonDocument.containsKey("oofTime")){
		uint32_t oofTime = jsonDocument["oofTime"];
		Config::setUint32("oofTime", oofTime);
	}
	loadConfig();
	handleStateChange();
}

extern "C"
void app_main(void)
{
	initArduinoWithBT();
	Config::init();
	Display::init_display();
	loadConfig();
	employeeState = EmployeeState::AWAY;
	
	// Create timers
	timerHandle = xTimerCreate("Timer", 60 * 1000 / portTICK_PERIOD_MS, pdTRUE, NULL, &timerCallback);
	xTaskCreate(&wifiTimerTask, "WifiTimer", 5*1024, NULL, configMAX_PRIORITIES - 1, &wifiTaskHandle);
	handleStateChange();
	xTimerStart(timerHandle, portMAX_DELAY);

	// Button init code:
	gpio_reset_pin(GPIO_NUM_12);
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_INPUT);
    gpio_install_isr_service(0);
    gpio_intr_enable(GPIO_NUM_12);
    gpio_set_intr_type(GPIO_NUM_12, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(GPIO_NUM_12, button_pressed, NULL);

	// BT init code:
	if(bluetooth_init()){
		DT_ERROR("Failed to initialize bluetooth");
	}
	bluetooth_set_device_state_cb([](bluetooth_device_state state){
		switch(state){
			case BT_DEVICE_CONNECTED: 
				xTimerStop(timerHandle, portMAX_DELAY);
				employeeState = EmployeeState::AT_DESK;
				break;
			case BT_DEVICE_DISCONNECTED:
				minutesSinceLastUpdate = 0;
				employeeState = EmployeeState::AWAY;
				xTimerStart(timerHandle, portMAX_DELAY);
				break;
		}
		handleStateChange();
	});
	if(bluetooth_start()){
		DT_ERROR("Failed to start bluetooth");
	}
	
	// Mesh init code:
	meshInit();
	setStateUpdateCallback([](uint32_t from, String recv, bool configurationChangeRequest){
		if(configurationChangeRequest){
			DynamicJsonDocument jsonDocument(1024);
			deserializeJson(jsonDocument, recv);
			handleConfigurationChange(jsonDocument);
		}else{
			broadcastStringToWS(recv);
		}
	});

	// HTTP / WS init code
	setStateUpdateCallback([](uint8_t* data, int length){
		if(!length) return;
		if(data[0] != '{') return;

		String recv((const char*) data, length);
		DynamicJsonDocument jsonDocument(1024);
		deserializeJson(jsonDocument, recv);
		if(!jsonDocument.containsKey("id")){
			return;
		}
		int id = jsonDocument["id"];
		if(id == getCurrentID()){
			handleConfigurationChange(jsonDocument);
		}else{
			sendBoardConfigurationChange(id, recv);
		}
	});
	setNewConnectionCallback([](){
		printf("Received new connection in main\n");
		String state = createStateJSON();
		broadcastStringToWS(state);
		requestAllStates();
	});
	ESP_ERROR_CHECK(startHttpServer());
}
