#include "painlessMesh.h"
#include <esp_http_server.h>

#define   MESH_PREFIX     "MegaGigaSiecESP32"
#define   MESH_PASSWORD   "papiez2137"
#define   MESH_PORT       5555

Scheduler userScheduler; 
painlessMesh  mesh;

String msgToSend = "0";
SimpleList<uint32_t> nodes;
std::list<String> data;

void receivedCallback(uint32_t from, String &msg);
void test(bool ask);

////////////////////////////INTERFACE////////////////////////////
typedef void (*StateUpdateCallback)(uint32_t id, uint8_t* data);
uint8_t currentState[256];
StateUpdateCallback callback = NULL;
xTaskHandle meshUpdateTask;

void setState(uint8_t *state){
  memcpy((void*) currentState, (void*) state, 256);
  mesh.sendBroadcast(String('1') + String((char *)currentState, 256));
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

void meshInit(){
  //Setup code here:
  Serial.print("\nSetup");
  mesh.setDebugMsgTypes( ERROR | STARTUP );  
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);


  // Loop replacement
  xTaskCreate([](void* unused){
    for(;;){
      Serial.print("\nLoop");
      mesh.update();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }, "Mesh Updates", 2 * 1024, NULL, configMAX_PRIORITIES-1, &meshUpdateTask);
}

////////////////////////////INTERFACE////////////////////////////

// __attribute__((noreturn)) void test(bool ask){
//   init();
//   Serial.print("\nTesting");
//   setStateUpdateCallback([](uint32_t id, uint8_t *data){
//     printf("Received data from %d: %s\n", id, (char*) data);
//     free((void*) data);
//   });
//   if(ask)
//     requestAllStates();
  
//   // ( ||+|| )\(;;)
//   for(;;);
// }


// Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

// void sendMessage() {
//   mesh.sendBroadcast( msgToSend );
//   nodes = mesh.getNodeList();
//   Serial.print("\nNr of nodes: ");
//   Serial.print(nodes.size());
//   Serial.print("\n");
//   taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
// }


void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

  switch(msg.c_str()[0]) {
  case '0':
    mesh.sendSingle(from, String('1') + String((char *)currentState, 256));
    break;
  case '1':
    if(callback)
      callback(from, (uint8_t*) msg.c_str() + 1);
    break;
  default:
    Serial.print("\n==== def ====\n");
}
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

// void setup() {
//   Serial.begin(115200);


//   mesh.setDebugMsgTypes( ERROR | STARTUP );  

//   mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
//   mesh.onReceive(&receivedCallback);
//   // mesh.onNewConnection(&newConnectionCallback);
//   // mesh.onChangedConnections(&changedConnectionCallback);
//   // mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

//   userScheduler.addTask( taskSendMessage );
//   taskSendMessage.enable();
// }

// void loop() {
  
//   mesh.update();
// }
