/***
Project SAPHE
Flight 2 Telemetry Software

MicroSD reader
SPI Pins
D13 SCK
D12 MISO
D11 MOSI
D8 CS

BMP085
3.3V to VCC
GND to GND
A4 to SDA
A5 to SCL
***/

/*** Configuration ***/

//#define ENABLE_SD
#define ENABLE_BMP
//#define ENABLE_CMP
//#define ENABLE_ADXL

/*** End Configuration ***/

/* SD Card */
#if defined(ENABLE_SD)
  #include <SD.h>
#endif

/* I2C */
#if defined(ENABLE_BMP) || defined(ENABLE_ADXL) || defined(ENABLE_CMP)
  #include <Wire.h>
#endif

/* Barometer */
#if defined(ENABLE_BMP)
  #include <Adafruit_BMP085.h>
  Adafruit_BMP085 bmp;
#endif

/* Accelerometer */
#if defined(ENABLE_ADXL)
  #include <ADXL345.h>
  ADXL345 accel;
#endif

/* Compass */
#if defined(ENABLE_CMP)
  #include <HMC5883L.h>
  HMC5883L cmp;
#endif

void setup() {
  Serial.begin(115200);
  
  /* Start I2C */
  #if defined(ENABLE_BMP) || defined(ENABLE_ADXL) || defined(ENABLE_CMP)
    Wire.begin();
  #endif
  
  /* Barometer */
  #if defined(ENABLE_BMP)
    bmp.begin();
  #endif
}

void loop() {
  
}
