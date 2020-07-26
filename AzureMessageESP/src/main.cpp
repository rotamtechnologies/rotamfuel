#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"

#define RXD2 16
#define TXD2 17

#define INTERVAL 90
#define DEVICE_ID "Esp32Device"
#define MESSAGE_MAX_LEN 1024

// Please input the SSID and password of WiFi
const char* ssid = "Red_wifi";
const char* password = "Your_password"; 

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>";

// JsonDocument to hold the message
StaticJsonDocument<500> RotamJson;

int messageCount = 1;
static bool hasWifi = false;
static bool messageSending = true;
static uint64_t send_interval_ms;

static void InitWifi();
static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result);
static void MessageCallback(const char* payLoad, int size);
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size);
static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size);

void setup() {
  // put your setup code here, to run once:
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2 );
  Serial.begin(9600);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");

  // Initialize the WiFi module
  Serial.println(" > WiFi");
  hasWifi = false;
  InitWifi();
  if (!hasWifi)
  {
    return;
  }
  randomSeed(analogRead(0));

  Serial.println(" > IoT Hub");
  Esp32MQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "GetStarted");
  Esp32MQTTClient_Init((const uint8_t*)connectionString, true);

  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);

  send_interval_ms = millis();
}

int incomingByte = 0;

void loop() {
  // put your main code here, to run repeatedly:
  RotamJson.clear();
  if( Serial2.available() > 0 ){
    char MessageJson[ MESSAGE_MAX_LEN ];
    DeserializationError err = deserializeJson( RotamJson, Serial2);
    serializeJson( RotamJson, MessageJson);
    if (err == DeserializationError::Ok)
    {
      if (hasWifi)
        { 
          if (messageSending && 
              (int)(millis() - send_interval_ms) >= INTERVAL)
          {        
            Serial.println( MessageJson );
            EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate( MessageJson, MESSAGE);      
            Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
            Esp32MQTTClient_SendEventInstance(message);
            
            send_interval_ms = millis();
          }
          else
          {
            Esp32MQTTClient_Check();
          }
        }
    } 
    else{
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() returned ");
      Serial.println(err.c_str());
  
      // Flush all bytes in the "link" serial port buffer
      while (Serial2.available() > 0)
        Serial2.read();
    }
  }       
}

// Methods Definition
static void InitWifi(){
  Serial.println("Connecting...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  hasWifi = true;
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}