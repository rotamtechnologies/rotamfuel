#include <Arduino.h>

// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h> // the OBD2 library depends on the CAN library
#include <OBD2.h>

void printPID(int pid) {
  // read the PID value
  float pidValue = OBD2.pidRead(pid);

  if (isnan(pidValue)) {
    Serial.print("error"); 
  } else {
    // print value with units
    Serial.print(pidValue);
  }
}


// array of PID's to print values of
const int PIDS[] = {
  CALCULATED_ENGINE_LOAD,
  ENGINE_COOLANT_TEMPERATURE,
  ENGINE_RPM,
  VEHICLE_SPEED,
  AIR_INTAKE_TEMPERATURE,
  MAF_AIR_FLOW_RATE,
  THROTTLE_POSITION,
  RUN_TIME_SINCE_ENGINE_START,
  FUEL_TANK_LEVEL_INPUT,
  ABSOLULTE_BAROMETRIC_PRESSURE,
  ABSOLUTE_LOAD_VALUE,
  RELATIVE_THROTTLE_POSITION
};

const int NUM_PIDS = sizeof(PIDS) / sizeof(PIDS[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("OBD2 Key Stats"));

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
    for (int i = 0; i < NUM_PIDS; i++) {
      
      int pid = PIDS[i];
      Serial.print(OBD2.pidName(pid));
      Serial.print(F(" "));
      Serial.print(OBD2.pidUnits(pid));

      if( i != NUM_PIDS-1  ){
        Serial.print(",");
      }
  }
}

void loop() {
  // loop through all the PID's in the array
  // 
  for (int i = 0; i < NUM_PIDS; i++) {
    int pid = PIDS[i];

    printPID(pid);
    if( i != NUM_PIDS-1  ){
      Serial.print(",");
    }
  }
  Serial.println();
  delay(1000);
}