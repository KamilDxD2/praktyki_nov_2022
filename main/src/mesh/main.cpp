#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "main.h"

#define   MESH_PREFIX     "MegaGigaSiecESP32"
#define   MESH_PASSWORD   "papiez2137"
#define   MESH_PORT       5555

static Scheduler userScheduler; 
static painlessMesh  mesh;

static String currentState;
static StateUpdateCallback callback = NULL;
static xTaskHandle meshUpdateTask;

void meshBroadcastState(String &state){
  currentState = state;
  mesh.sendBroadcast(String('1') + state);
}

void sendBoardConfigurationChange(int id, String &state){
  mesh.sendSingle(id, String('2') + state);
}

void setStateUpdateCallback(StateUpdateCallback newCb){
  callback = newCb;
}

void requestAllStates(){
  mesh.sendBroadcast("0");
}

uint32_t getCurrentID(){
  return mesh.getNodeId();
}

void receivedCallback( uint32_t from, String &msg ) {
  DT_INFO("Received from %u msg=%s\n", from, msg.c_str());

  switch(msg.c_str()[0]) {
  case '0':
    mesh.sendSingle(from, String('1') + currentState);
    break;
  case '1':
    if(callback)
      callback(from, String(msg.c_str() + 1), false);
    break;
  case '2':
    if(callback)
      callback(from, String(msg.c_str() + 1), true);
    break;
  }
}

static void meshLoop(void *pvParameters) {
  while (1) {
    mesh.update();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void meshInit(){
  //Setup code here:
  mesh.setDebugMsgTypes( ERROR | STARTUP );  
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);

  xTaskCreate(meshLoop, "Mesh Loop", 40*1024, NULL, configMAX_PRIORITIES - 1, &meshUpdateTask);
}
