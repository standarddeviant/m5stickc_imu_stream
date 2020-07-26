#include <Arduino.h>
#include <M5StickC.h>
#include <WiFi.h>
// #include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "WIFI_SECRETS.h"
// if you don't have WIFI_SECRETS.H, fill in and uncomment the following lines
// and copy it to WIFI_SECRETS.H in the same folder as main.cpp
// const char* ssid = "WIFI_SSID_FROM_WIFI_SECRETS.H";
// const char* password = "WIFI_PASSWORD_FROM_WIFI_SECRETS.H";

#define IMU_FETCH_PERIOD_MICROS 25000 // 1e6 micros / 40 Hz = 25e3 micros
#define WS_SERVER_PORT 42000
WebSocketsServer webSocket = WebSocketsServer(WS_SERVER_PORT);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            // USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                //IPAddress ip = webSocket.remoteIP(num);
                //USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
				// send message to client
                Serial.print("We got one!\n");
				// webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_ERROR:
        case WStype_TEXT:
        case WStype_BIN:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        case WStype_PING:
        case WStype_PONG:
            break;
    }
}

volatile int g_do_imu_fetch = 0;
static void imu_fetch_timer_callback(void* arg);
static void imu_fetch_timer_callback(void* arg) {
    // fixme - get timing data?
    g_do_imu_fetch = 1;
}

StaticJsonDocument<4000> g_imudoc;
#define IMU_MPK_MAX_SZ 4000
char g_imumpk[IMU_MPK_MAX_SZ];
size_t g_imumpk_sz;



// const char* PARAM_MESSAGE = "message";

void setup() {
    // put your setup code here, to run once:

    // Init Vars
    esp_timer_handle_t imu_fetch_timer;
    esp_timer_create_args_t imu_fetch_timer_args;
    imu_fetch_timer_args.callback = &imu_fetch_timer_callback;
    imu_fetch_timer_args.name = "IMU-Fetch"; /* optional, but helpful when debugging */

    esp_log_level_set("YO", ESP_LOG_DEBUG);        // set all components to ERROR level


    // Serial Init
    Serial.begin(115200);

    // Basic M5Stick-C Init
    M5.begin();
    M5.IMU.Init();
    M5.Axp.ScreenBreath(8); // turn the screen 0=off, 8=dim
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.printf("Connecting to WiFi...");

    // WiFi Init
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    IPAddress ip = WiFi.localIP();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    // M5.Lcd.printf("Connected to %s", WiFi.SSID());
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.printf("Connect to\n%d.%d.%d.%d:%d",
        ip[0], ip[1], ip[2], ip[3], WS_SERVER_PORT);

    // WebSocket Init
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.print("setup: YOYOYO!\n");


    // Create and Start Timer
    ESP_ERROR_CHECK(esp_timer_create(&imu_fetch_timer_args, &imu_fetch_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(imu_fetch_timer, IMU_FETCH_PERIOD_MICROS));
}

#define IMU_BUF_LEN 20
float g_ax[IMU_BUF_LEN] = { 0.0f };
float g_ay[IMU_BUF_LEN] = { 0.0f };
float g_az[IMU_BUF_LEN] = { 0.0f };
float g_gx[IMU_BUF_LEN] = { 0.0f };
float g_gy[IMU_BUF_LEN] = { 0.0f };
float g_gz[IMU_BUF_LEN] = { 0.0f };
uint32_t g_since[IMU_BUF_LEN] = { 0 };
float *g_p_ax;
float *g_p_ay;
float *g_p_az;
float *g_p_gx;
float *g_p_gy;
float *g_p_gz;
uint32_t *g_p_since;
void imu_reset_pointers(void) {
    g_p_ax = &(g_ax[0]);
    g_p_ay = &(g_ay[0]);
    g_p_az = &(g_az[0]);
    g_p_gx = &(g_gx[0]);
    g_p_gy = &(g_gy[0]);
    g_p_gz = &(g_gz[0]);
    g_p_since = &(g_since[0]);
}

int g_sample_count = 0;

void loop() {
    static int64_t tprev = 0;
    int64_t tnow;
    int ret;

    // put your main code here, to run repeatedly:
    webSocket.loop();
    if(g_do_imu_fetch) {
        // get imu fetch time
        tnow = esp_timer_get_time();

        g_do_imu_fetch = 0; // set flag to zero
        
        // initialize pointers if necessary
        if(0 == g_sample_count) {
            imu_reset_pointers();
        }

        // load accel/gyro data via pointers
        M5.IMU.getAccelData(g_p_ax++, g_p_ay++, g_p_az++);
        M5.IMU.getGyroData( g_p_gx++, g_p_gy++, g_p_gz++);

        // load timing data via pointer
        *(g_p_since++) = (uint32_t)(tnow - tprev);
        tprev = tnow;



        // increase sample count
        g_sample_count++;

        // send packet if necessary
        if(IMU_BUF_LEN == g_sample_count) {
            // package up data
            int ix;

            // init pointers and json doc
            imu_reset_pointers();
            g_imudoc.clear();
            
            // pack accel
            JsonArray ax = g_imudoc.createNestedArray("ax");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { ax.add(*(g_p_ax++)); }
            JsonArray ay = g_imudoc.createNestedArray("ay");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { ay.add(*(g_p_ay++)); }
            JsonArray az = g_imudoc.createNestedArray("az");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { az.add(*(g_p_az++)); }

            // pack gyro
            JsonArray gx = g_imudoc.createNestedArray("gx");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { gx.add(*(g_p_gx++)); }
            JsonArray gy = g_imudoc.createNestedArray("gy");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { gy.add(*(g_p_gy++)); }
            JsonArray gz = g_imudoc.createNestedArray("gz");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { gz.add(*(g_p_gz++)); }

            // pack timing data
            JsonArray _micros = g_imudoc.createNestedArray("micros");
            for(ix = 0; ix < IMU_BUF_LEN; ix++) { _micros.add(*(g_p_since++)); }

            // serialize json object
            g_imumpk_sz = serializeMsgPack(g_imudoc, &(g_imumpk[0]), sizeof(g_imumpk));

            // send packet to all clients
            ret = webSocket.broadcastBIN((uint8_t *) &(g_imumpk[0]), g_imumpk_sz);
            Serial.print("ret="); Serial.print(ret); 
            Serial.print(", N="); Serial.print(g_imumpk_sz);
            Serial.print(", C="); Serial.print(webSocket.connectedClients());
            Serial.print(", t="); Serial.print(millis());
            Serial.print("\n");
            
            // reset sample count and timing vars
            g_sample_count = 0;

        } // end send packet 

    } // end fetch sample

} // end loop