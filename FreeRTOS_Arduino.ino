  //Author: Maitland Kelly
#include <AWS_IOT.h>
#include <WiFi.h>
#include <string.h>

//#include <ArduinoJson.h>
//#include <HardwareSerial.h>

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}

#define WIFI_SSID "example"        // put a string in quotes, example: "Wifi Name goes here"
#define WIFI_PASSWORD "example"    //same here, example "password goes here"
#define WIFI_TIMEOUT_MS 10000      // 10 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 15000 // Wait 15 seconds after a failed connection attempt
#define HOST_ADDRESS "example.com" // put string in quotes..  this is MQTT server IP "exampleserverip.com"

AWS_IOT mqtt_client;    //MQTT function

TaskHandle_t WIFI_Task;          //RTOS task handles, add as needed
TaskHandle_t heap_task;

uint32_t mem_size = 0;
uint32_t free_mem = 0;
String   free_str;
String   mem_str;

//WiFi and MQTT Broker setup, Keys are located in aws_iot_certificate.c
int status = 0;
int mph = 0;
char CLIENT_ID[]= "";            //Client ID should be changed if you have multiple devices connect to one server. Otherwise whatever
char TEMP_TOPIC[] ="";           // put the topic name, ie: "/topic2" in the quotes 
char HUMD_TOPIC[] ="";
char MEM_TOPIC[]  ="";
char payload[128];
char rcvdPayload[128];

char mem_string[8];
char memsize_string[8];


int msgReceived = 0;

//MQTT Publishing callback
void mysubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
   strncpy(rcvdPayload,payLoad,payloadLen);
   rcvdPayload[payloadLen] = 0;
   msgReceived = 1;
}
/*======================================================================================================
****************************FreeRtos Tasks/Functions****************************************************
=======================================================================================================*/

void keepWiFiAlive(void * parameter){
    for(;;){
        if(WiFi.status() == WL_CONNECTED){
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }
        Serial.println("[WIFI] Connecting");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        unsigned long startAttemptTime = millis();
        // Keep looping while we're not connected and haven't reached the timeout
        while (WiFi.status() != WL_CONNECTED && 
                millis() - startAttemptTime < WIFI_TIMEOUT_MS){}

        // When we couldn't make a WiFi connection (or the timeout expired)
      // sleep for a while and then retry.
        if(WiFi.status() != WL_CONNECTED){
            Serial.println("[WIFI] FAILED");
            vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
        continue;
        }
        Serial.println("[WIFI] Connected: " + WiFi.localIP());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(mqtt_client.connect(HOST_ADDRESS,CLIENT_ID)== 0)
        {
        Serial.println("Connected to Broker");
        }
        else
        {
        Serial.println("MQTT Broker unreachable, check address");
        while(1);
    }
  }
    vTaskDelete( &WIFI_Task );
}


void heap_check(void * parameter)
{
  for(;;){
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      free_mem = ESP.getFreeHeap();
      mem_size = ESP.getHeapSize();
      free_str =String(free_mem);
      free_str.toCharArray(mem_string,7);
      vTaskDelay(200 / portTICK_PERIOD_MS);
      mqtt_client.publish(MEM_TOPIC,mem_string);
  }vTaskDelete( &heap_task );
}

/*==================================================================================================
*******************************Main Code Goes Below***********************************************
================================================================================================== */

void setup() {  
   Serial.begin(115200);
   xTaskCreate(
     keepWiFiAlive,
     "keepWiFiAlive",                    // Task name
     20000,                              // Stack size (bytes) 
     NULL,                               // Parameter
     10,                                 // Task priority
     &WIFI_Task                          // Task handle
   );
   xTaskCreatePinnedToCore(
     heap_check,
     "heap_check",                       // Task name
     20000,                              // Stack size (bytes)
     NULL,                               // Parameter
     6,                                  // Task priority
     &heap_task,                         // Task handle
     1                                   // Task Core
   );
}
void loop() {
}
