#include <ADXL345.h>
#include <HMC58X3.h>
#include <ITG3200.h>
#include <bma180.h>
#include <MS561101BA.h>

//#define DEBUG
#include "DebugUtils.h"

#include "FreeIMU.h"
#include "CommunicationUtils.h"
#include <Wire.h>

int raw_values[9];
//char str[512];
float ypr[3]; // yaw pitch roll
float val[9];

// Set the FreeIMU object
FreeIMU my3IMU = FreeIMU();

void setup() { 
  Serial.begin(115200);
  Wire.begin();
  
  delay(5);
  my3IMU.init(); // the parameter enable or disable fast mode
  delay(5);
}

void loop() { 
  
  my3IMU.getYawPitchRoll(ypr);
  Serial.print("Yaw: ");
  Serial.print(ypr[0]);
  Serial.print(" Pitch: ");
  Serial.print(ypr[1]);
  Serial.print(" Roll: ");
  Serial.print(ypr[2]);
  Serial.println("");
  
  delay(10);
}



