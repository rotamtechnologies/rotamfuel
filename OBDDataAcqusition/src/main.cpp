// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//Prueba Rotam Technologies
#include <Arduino.h>
#include <TinyGPS++.h>
#include <OBD2.h>
#include <ArduinoJson.h>
#include <TimeLib.h> // Library to update UTC timezone 
#include <Wire.h>
#include <MPU6050_tockn.h>
//#include <SD.h> // Library to backup Json message

void InitOBD();
void OBDInfo();
// void InitSD();
static void GpsEncode( unsigned long ms );
void GpsTimeZone();
void GpsInfo();
void InitMPU6050();
void InfoMPU6050();

// Variables for the SD
/* #define FilePath "/DataOBD/"
  const int chipSelect = 9; // SS pin para la tarjeta SD.
  bool EnableSD;  // Variable para saber si está un SD disponible.
  File dataFile;  // Variable para guardar el archivo
  char FileName[24];
 */

// JsonDocument to hold the message
StaticJsonDocument<300> RotamJson; 

const int BasicPIDS[] = {
  ENGINE_RPM,
  VEHICLE_SPEED,
  CALCULATED_ENGINE_LOAD,
  THROTTLE_POSITION,
  MAF_AIR_FLOW_RATE,
  INTAKE_MANIFOLD_ABSOLUTE_PRESSURE
};

const int NUM_BasicPIDS = sizeof(BasicPIDS) / sizeof(BasicPIDS[0]);

// The TinyGPS++ object
TinyGPSPlus gps;
const int UTC_offset = -5; // offset hour in hours
time_t prevDisplay = 0;

// MPU6050 object
MPU6050 mpu6050(Wire);

// Definition of the serial ports for ESP32 and GPS
#define SerialESP Serial2
#define SerialGPS Serial1

void setup() {
  Serial.begin(9600);
  SerialESP.begin(9600);
  SerialGPS.begin(9600);
  while( !Serial && !SerialESP && !SerialGPS );
  Serial.println(F("Serial ports initialized"));

  InitOBD();    
  InitMPU6050();
//  InitSD();
}

void loop() {
  // loop through all the PID's in the array
  RotamJson.clear();
  GpsEncode(100);
  GpsTimeZone();
  
  OBDInfo();
  InfoMPU6050();
  serializeJson( RotamJson, SerialESP);
  SerialESP.println();
  serializeJsonPretty( RotamJson, Serial);
  Serial.println();
  delay(100);
}

// Cuerpo de las funciones.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InitOBD(){
  while (true) {
    Serial.print(F("Attempting to connect to OBD2 CAN bus ... "));

    if (!OBD2.begin()) {
      Serial.println(F("failed!"));
    
      delay(1000);
    } else {
      Serial.println(F("success"));
      break;
    }
  }  
}

void OBDInfo(){
  for (int i = 0; i < NUM_BasicPIDS; i++) {
    int pid = BasicPIDS[i];
    float pidValue = OBD2.pidRead(pid);
    if (isnan(pidValue)) {
      RotamJson[ OBD2.pidName(pid) ] = "error";
    } else {
      // print value with units
      RotamJson[ OBD2.pidName(pid) ] = pidValue;
    }    
  }
} 

/* void InitSD(){
  // Inicialización de la tarjeta SD
  pinMode(chipSelect, OUTPUT);

  // see if the card is present and can be initialized:
  EnableSD = SD.begin(chipSelect); 
  
  if (!EnableSD) {
    Serial.println("No inició SD.");
    // don't do anything more:
  }else{   
    FileName[24] = FilePath;
        
    if( SD.exists(FilePath) ){  
      sprintf(FileName + sizeof(FilePath) -1,"DateMeasure" );    
    }
    else{
      SD.mkdir(FilePath);
      sprintf(FileName + sizeof(FilePath) -1,"DateMeasure" );                
    }
    Serial.println("Tarjeta SD lista."); 
  }
  if(!SD.exists(FileName)){ // Si el archivo ya existe no es necesario imprimir las cabeceras de la tabla
    dataFile = SD.open(FileName, FILE_WRITE);
      for (int i = 0; i < NUM_PIDS; i++) {  
        int pid = PIDS[i];
        dataFile.print(OBD2.pidName(pid));
        dataFile.print(F(" "));
        dataFile.print(OBD2.pidUnits(pid));          
        if( i != NUM_PIDS-1  ){
          dataFile.print(",");
        }
      }
    dataFile.close();   
  } 
  // Final de la inicialización de la tarjeta SD
} */

static void GpsEncode(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while ( SerialGPS.available() )
      gps.encode( SerialGPS.read() );
  } while (millis() - start < ms);
}

void GpsTimeZone(){
  int Year = gps.date.year();
  byte Month = gps.date.month();
  byte Day = gps.date.day();
  byte Hour = gps.time.hour();
  byte Minute = gps.time.minute();
  byte Second = gps.time.second();  
  
  setTime( Hour, Minute, Second, Day, Month, Year);
  adjustTime( UTC_offset * SECS_PER_HOUR );
  GpsInfo();
}

void GpsInfo() {
#define GPS_MAX_LEN 256  
  const char *DateFormat = "%u/%u/%u"; // DD/MM/YYYY
  const char *HourFormat  = "%02u:%02u:%02u.%02u" ; // HH:MM:SS.cc
  char Date[256], Hour[256];
  
  if (gps.date.isValid())
  {
    snprintf( Date, GPS_MAX_LEN, DateFormat, day(), month(), year());
    RotamJson["Date"] = Date;
  }
  else
  {
    RotamJson["Date"] = "error";
  }
  if (gps.time.isValid())
  {
    snprintf( Hour, GPS_MAX_LEN, HourFormat, hour(), minute(), second(), gps.time.centisecond());
    RotamJson["Hour"] = Hour;
  }
  else
  {
    RotamJson["Hour"] = "error";
  }

  if (gps.location.isValid())
  {
    RotamJson["Latitude"] = gps.location.lat();
    RotamJson["Longitude"] = gps.location.lng();
  }
  else
  {
    RotamJson["Latitude"] = 91;
    RotamJson["Longitude"] = 181;
  }
}

void InitMPU6050(){
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);  
}

void InfoMPU6050(){
  RotamJson["AccXYZ"][0] = mpu6050.getAccX();
  RotamJson["AccXYZ"][1] = mpu6050.getAccY();
  RotamJson["AccXYZ"][2] = mpu6050.getAccZ();

  RotamJson["GyroXYZ"][0] = mpu6050.getGyroX();
  RotamJson["GyroXYZ"][1] = mpu6050.getGyroY();
  RotamJson["GyroXYZ"][2] = mpu6050.getGyroZ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* {
  "sensor": "gps",
  "time": 1351824120,
  "data": [
    48.75608,
    2.302038
  ]
}
 */

/* {
  "Date": "20/7/2020",
  "Hour": "14:26:42.47",
  "Lat": 7.1,
  "Lng": 74.6,
  "RPM": 3450,
  "SPEED": 46,
  "Engine Load": 76,
  "TPS": 40,
  "MAF": 2.1,
  "MAP": 2.1,
  "AccXYZ": [
    48.75608,
    2.302038,
    7.21
  ],
  "GyroXYZ": [
    48.75608,
    2.302038,
    7.21
  ]
} */