#include "main.h"

static ws_state_update_cb_t callback = NULL;
void setStateUpdateCallback(ws_state_update_cb_t callbacka){
    callback = callbacka;
}

static std::map<int, WSConnection> connections;

httpd_handle_t server = NULL;
esp_err_t main_page(httpd_req_t *req){
    httpd_resp_send(req, "<!DOCTYPE html><html lang=\"pl\"><head> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Dashboard</title> <style>*{margin: 0; padding: 0; box-sizing: border-box;}body{background-color: black; font-family: \"Trebuchet MS\", sans-serif; z-index: 1;}.device{margin-left: 50px; width: 20rem; background-color: rgb(255, 255, 255); border-radius: 1rem; padding: .5rem; margin: 1rem; position: relative; color: black; transition: .3s ease-in-out;}.flex-container{display: flex; flex-direction: column; align-items: center; justify-content: flex-start;}.time{font-weight: 800; font-size: 3rem; flex-grow: 2;}.dot{padding: 10px; border-radius: 50%; position: absolute; top: 8px; left: 8px;}.hidden{visibility: hidden;}#bar{width: 100%; color: #f84018; text-align: center; font-weight: bold; font-size: 1.5rem; padding-top: 1rem; transition: .3s ease-in-out; cursor: pointer;}.bar:hover{color: white;}fieldset{color: white; border: none; border-top: 1px solid white; margin: 1rem 1rem 4rem 1rem;}legend{margin: 1rem 1rem; padding: 0 1rem;}#connected{display: flex; flex-direction: row; flex-wrap: wrap; padding: 0 2rem; justify-content: center;}#configurable{display: flex; flex-direction: row; flex-wrap: wrap;padding: 0 2rem; justify-content: center;}#configurable .device:hover{background-color: rgb(223, 223, 223); cursor: pointer;}#popup-window{position: fixed; top: 0; left: 0; height: 20rem; width: 30rem; margin: calc((100vh - 20rem)/2) calc((100vw - 30rem)/2); background-color: white; z-index: 2137; text-align: center; font-size: 17rem; padding: 3rem; display: flex; flex-direction: column; justify-content: space-between; align-items: center; border-radius: 1rem; transition: .3s ease-in-out;}#popup-window input{width: 78%; margin: 0 16%; height: 3rem; border: none; border: 2px solid #f84018; text-align: center; border-radius: 1.5rem; transition: .3s ease-in-out; font-size: 1.15rem;}input:focus{border-color: black !important; outline: none;}#ligma{visibility: hidden; position: fixed; z-index: 2136; height: 100%; width: 100%; background-color: black; opacity: .7;}h2{font-size: 2rem;}button{border: none; outline: none; border: 2px solid white; background-color: #f84018; color: white; height: 3rem; width: 78%; margin: 0 auto; border-radius: 1.5rem; font-size: 1.15rem; transition: .3s ease-in-out;}button:hover{border: 2px solid #f84018; background-color: white; cursor: pointer; color: #f84018;}.waldissimo{position: fixed; right: 0; bottom: 0;}.trueHidden{display: none;}#lupka{width: 2rem; height: 2rem;}#search{display: flex; justify-content: center; align-items: flex-start;}#searchbar{color: white; background-color: black; border: none; border: 3px solid white; border-radius: 1rem; height: 2rem; text-align: center; margin-left: 15px; transition: all .3s ease-in-out;}#searchbar:focus{border-color: #f84018 !important; outline: none;}.greenBackground{background-color: green;}.redBackground{background-color: #f84018;}.yellowBackground{background-color: orange;}/* #f84018 */ </style></head><body> <div id=\"ligma\"></div><div id=\"bar\"> <h1 id=\"walde\">Dashboard</h1> <div id=\"search\"><span>🔎</span> <input id=\"searchbar\" class=\"trueHidden\" placeholder=\"e.g. John Doe\" type=\"text\"> </div></div><fieldset id=\"configurable\"> <legend>Cofigurable Devices</legend> </fieldset> <fieldset id=\"connected\"> <legend>Connected Devices</legend> </fieldset> <script>let ws; function isObject(value){return typeof (value)==='object' && value !=null;}class AmogElement{mount(element){}render(){throw new Error(\"Unimplemented render() method\");}}function applyProp(target, prop, value){console.log(\"applyProp:\", prop, value, \"on\", target); if (isObject(value)){applyProps(target[prop], value);}else{target[prop]=value;}}function applyProps(target, props){console.log(\"applyProps:\", props, \"on\", target); if (isObject(props)){for (const prop in props){const value=props[prop]; applyProp(target, prop, value);}}else{throw new Error(\"`attributes` must be an object!\");}}function d(target){if (target){target.remove();}}function childToNode(child){if (child==null){return document.createComment(\"child placeholder\");}else if (child instanceof AmogElement){const element=child.render(); element.$amog=child; return element;}else if (child instanceof HTMLElement){return child;}else if (typeof (child)==='string'){return document.createTextNode(child);}else{return document.createTextNode(JSON.stringify(child));}}function c(tag, attributes, ...children){const element=document.createElement(tag); applyProps(element, attributes); for (const child of children){element.appendChild(childToNode(child));}return element;}function notifyMount(element){for (const child of element.children){child.$amog?.mount(child); notifyMount(child);}}function mount(element, target){target.appendChild(childToNode(element)); notifyMount(target);}let configurables=document.querySelector(\"#configurable\"); let connected=document.querySelector(\"#connected\"); let bg=document.querySelector(\"#ligma\"); let bar=document.querySelector(\"#bar\"); let body=document.body; let devicesFound={}; displayDevices(devicesFound); function displayDevices(obj){document.querySelectorAll(\".device\").forEach(d); Object.values(obj).forEach((element, i)=>{let gamer=c(\"div\",{className: 'device', id: `config-${i}`}, c(\"div\",{className: \"flex-container\"}, (!element.configurable ? c(\"div\",{className: `dot ${({1: \"greenBackground\", 2: \"yellowBackground\", 3: \"redBackground\",})[element.state]}`}) : \"\"), c(\"p\",{className: \"name\"}, element.firstName + \" \" + element.lastName), c(\"p\",{className: \"time\"}, element.configurable ? \"Configurable\" : element.time), c(\"div\",{className: \"hidden\"}, 'a') ) ); if (element.configurable){gamer.addEventListener('click', ()=>{bg.style.visibility='visible'; let input, input2, time; let popup=c(\"div\",{id: 'popup-window'}, c(\"h2\",{}, \"Device Configuration\"), (input=c(\"input\",{type: 'text', placeholder: element.firstName},)), (input2=c(\"input\",{type: 'text', placeholder: element.lastName},)), (time=c(\"input\",{type: 'number', value: element.outOfOfficeTime})), c(\"button\",{onclick: ()=>{handleConfig({time: time.value, firstName: input.value || input.placeholder, lastName: input2.value || input2.placeholder, id: element.id,})}}, \"OK\") ); body.appendChild(popup);}); configurables.appendChild(gamer);}else{connected.appendChild(gamer);}});}function closePopup(){bg.style.visibility='hidden'; if (document.querySelector(\"#popup-window\")){d(document.querySelector(\"#popup-window\"))}}function handleConfig({id, firstName, lastName, time}){closePopup(); const object={firstName, lastName, oofTime: time, id,}; console.log(object); ws.send(JSON.stringify(object));}bg.addEventListener('click', ()=>{closePopup();}); let waldek=document.querySelector(\"#walde\"); waldek_count=0; waldek.addEventListener('click', ()=>{waldek_count++; if (waldek_count % 7==0){let waled=c(\"img\",{className: \"waldissimo\", size: 'contain', src: \"\"}); body.append(waled); waled.addEventListener('click', ()=>{waled.style.width=Math.floor(Math.random() * 100 +2) + \"vw\"; waled.style.height=Math.floor(Math.random() * 100 +2) + \"vh\";});}}); let searchBar=document.querySelector(\"#search > input\"); searchBar.addEventListener(\"input\", ()=>{displayDevices(Object.fromEntries(Object.entries(devicesFound).filter(([id, object])=> searchBar.value.split(\" \").some(e=> object.firstName.includes(e) || object.lastName.includes(e)))));}); let searchIcon=document.querySelector(\"#search > span\"); searchIcon.addEventListener(\"click\", ()=>{searchBar.classList.toggle(\"trueHidden\");}); let wsInterval=null; function connectWebsocket(){ws=new WebSocket(`ws://${window.location.host}/ws`); ws.onmessage=message=>{const{firstName, lastName, minutes, id, outOfOfficeTime, state, configurable}=JSON.parse(message.data); const formattedTime=`${Math.floor(minutes / 60).toString().padStart(2, '0')}:${(minutes % 60).toString().padStart(2, '0')}`;if(id===0) return; devicesFound[id]={firstName, lastName, time: formattedTime, configurable, id, state, outOfOfficeTime,}; displayDevices(devicesFound);}; if(wsInterval) clearInterval(wsInterval); wsInterval=setInterval(()=> ws.send(\"A\"), 1000); ws.onclose=()=> setTimeout(connectWebsocket, 10000);}connectWebsocket(); </script></body></html>", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static xQueueHandle queue;
static xTaskHandle task;
static ws_new_connection_cb_t newConnectionCallback = NULL;
void setNewConnectionCallback(ws_new_connection_cb_t cb){
    newConnectionCallback = cb;
}

void broadcastStringToWS(String &string){
    if(queue == NULL) return;

    int length = string.length();
    char* data = (char*) malloc(length + 1);
    memcpy(data, string.c_str(), length);
    data[length] = 0;
    xQueueSend(queue, &data, 0);
}

void wsTask(void *unused){
    char *data;
    for(;;){
        if(xQueueReceive(queue, &data, portMAX_DELAY)){
            httpd_ws_frame_t frame;
            frame.payload = (uint8_t*)data;
            frame.len = strlen(data);
            frame.type = HTTPD_WS_TYPE_TEXT;


            httpd_ws_frame_t die;
            die.payload = NULL;
            die.len = 0;
            die.type = HTTPD_WS_TYPE_CLOSE;
            die.final = true;
            std::vector<int> killList;
            for(auto connection : connections){
                if(time(NULL) - connection.second.lastPingTime > 30){
                    httpd_ws_send_frame_async(connection.second.handle, connection.second.fd, &die);
                    killList.push_back(connection.first);
                    continue;
                }
                httpd_ws_send_frame_async(connection.second.handle, connection.second.fd, &frame);
            }

            for(int fd : killList){
                // DT_LOG doesn't work
                // We tried
                printf("Client with fd %d has been yeeted.\n", fd);
                connections.erase(fd);
            }
            free((void*) data);
        }
    }
}

static esp_err_t websocket_handler(httpd_req_t *req) {
    DT_INFO("req method: %d", req->method);
    if (req->method == HTTP_GET) {
        WSConnection connection;
        connection.fd = httpd_req_to_sockfd(req);
        connection.handle = req->handle;
        connection.lastPingTime = time(NULL);
        connections[connection.fd] = connection;
        DT_INFO("Websocket handshake done, connection opened");
        if(newConnectionCallback){
            newConnectionCallback();
        }
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        DT_ERROR("failed to read frame length, error code: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
        return ret;
    }

    DT_INFO("Received frame with length %d", ws_pkt.len);

    uint8_t *data = NULL;

    if (ws_pkt.len) {
        data = (uint8_t*)malloc(ws_pkt.len + 1);
        if (data == NULL) {
            DT_ERROR("Failed to allocate buffer for websocket message");
            return ESP_ERR_NO_MEM;
        }

        ws_pkt.payload = data;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        data[ws_pkt.len] = 0;

        if (ret != ESP_OK) {
            DT_ERROR("Failed to receive websocket message: " DT_ESP_ERR_FORMAT, DT_ESP_ERR(ret));
            free(data);
            return ret;
        }

        if(callback){
            callback(data, ws_pkt.len);
        }
        
        DT_INFO("Got packet with message: %s", data);

        free(data);
        connections[httpd_req_to_sockfd(req)].lastPingTime = time(NULL);
    }
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = main_page,
    .user_ctx = NULL
};

httpd_uri_t uri_get_index = {
    .uri      = "/index.html",
    .method   = HTTP_GET,
    .handler  = main_page,
    .user_ctx = NULL
};

httpd_uri_t uri_websocket = {
    .uri      = "/ws",
    .method   = HTTP_GET,
    .handler  = websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true
};

esp_err_t startHttpServer(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Start the httpd server */
    const esp_err_t state = httpd_start(&server, &config);
    if (state == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_get_index);
        httpd_register_uri_handler(server, &uri_websocket);
    }
    /* If server failed to start, handle will be NULL */

    queue = xQueueCreate(20, sizeof(void*));
    xTaskCreate(wsTask, "WebSocket Manager", 16 * 1024, NULL, configMAX_PRIORITIES - 6, &task);

    return state;
}
