/* Project SAPHE */

#define ENABLE_SD
#define ENABLE_XB
//#define ENABLE_SERIAL
#define ENABLE_BMP
#define ENABLE_RTC
#define ENABLE_AXL
#define ENABLE_CMP

//#include<stdlib.h>

#if defined(ENABLE_SD)
  #include <SD.h>
  Sd2Card card;
  const int sdCS = 8;
  const int sdELED = 6;
  char filename[] = "LOG00.csv";
  File logFile;
#endif

#if defined(ENABLE_XB)
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(2, 3); // RX, TX
#endif

/* I2C */
#if defined(ENABLE_BMP) || defined(ENABLE_RTC) || defined(ENABLE_AXL) || defined(ENABLE_CMP)
  #include <Wire.h>
#endif

#if defined(ENABLE_BMP)
  #include <Adafruit_BMP085.h>
  Adafruit_BMP085 bmp;
  uint32_t curSeaPres = 101500;  //Current Sea Level Pressure
#endif

#if defined(ENABLE_RTC)
  #include "RTClib.h"
  RTC_DS1307 rtc;
#endif

#if defined(ENABLE_AXL)
  #include <ADXL345.h>
  ADXL345 accel;
  int areConnected = 0;
#endif

#if defined(ENABLE_CMP)
  #include <HMC5883L.h>
  HMC5883L compass;
#endif

void setup()
{
  #if defined(ENABLE_XB)
    mySerial.begin(57600);
  #endif
  
  #if defined(ENABLE_SERIAL)
    Serial.begin(57600);
  #endif
  
  #if defined(ENABLE_SD)
    pinMode(sdCS, OUTPUT);
    pinMode(sdELED, OUTPUT);
    card.init(SPI_HALF_SPEED, sdCS);
    if (!SD.begin(sdCS)) {
      #if defined(ENABLE_XB)
        mySerial.println("Card failed, or not present");
      #endif
      #if defined(ENABLE_SERIAL)
        Serial.println("Card failed, or not present");
      #endif
      digitalWrite(sdELED, HIGH);
    }
    else {
      #if defined(ENABLE_XB)
        mySerial.println("Card is initialized");
      #endif
      
      #if defined(ENABLE_SERIAL)
        Serial.println("Card is initialized");
      #endif
      
      for (uint8_t i = 0; i < 100; i++) 
      {
        filename[3] = i/10 + '0';
        filename[4] = i%10 + '0';
        if (! SD.exists(filename)) 
        {
          logFile = SD.open(filename, FILE_WRITE);
          break;
        }
      }
    }
  #endif
  
  #if defined(ENABLE_BMP)
    bmp.begin();
  #endif
  
  #if defined(ENABLE_RTC)
    rtc.begin();
  #endif
  
  #if defined(ENABLE_AXL)
    Wire.begin();
    accel.EnsureConnected();
    accel = ADXL345(); //is this necessary?
  #endif
  
  #if defined(ENABLE_CMP)
    Wire.begin();
    compass.EnsureConnected();
    compass = HMC5883L(); //is this necessary?
  #endif
  
  #if defined(ENABLE_CMP) && defined(ENABLE_AXL)
    if(compass.IsConnected && accel.IsConnected)
    {
      areConnected = true;
      compass.SetScale(1.3);
      compass.SetMeasurementMode(Measurement_Continuous);
      accel.SetRange(2, true);
      accel.EnableMeasurements();
    }
  #endif
  
  /* SD Card Log Header */
  #if defined(ENABLE_SD)
    String header;
    #if defined(ENABLE_RTC)
      header += "id,timestamp,";
    #endif
    #if defined(ENABLE_BMP)
      header += "degC,Pa,EstMeters,RealMeters,";
    #endif
    logFile.println(header);
    logFile.close();
    #if defined(ENABLE_XB)
      mySerial.println(header);
    #endif
    #if defined(ENABLE_SERIAL)
      Serial.println(header);
    #endif
  #endif
}


void loop()
{
  #if defined(ENABLE_SD)
    String record;
    #if defined(ENABLE_RTC)
      DateTime now = rtc.now();
      String rtcmonth = "0"; String rtcday = "0";
      String rtcmin = "0"; String rtcsec = "0";
      String utime = String(now.unixtime());  record += utime; record += ",";
      char buffer[20]; dtostrf(now.year(), 4, 0, buffer);  record += String(buffer); record += "/";
      if(now.month() <= 9) {
        dtostrf(now.month(), 1, 0, buffer);
        rtcmonth += buffer;
      } else {
        dtostrf(now.month(), 2, 0, buffer);
        rtcmonth = String(buffer);
      }
      record += rtcmonth; record += "/";
      if(now.day() <= 9) {
        dtostrf(now.day(), 1, 0, buffer);
        rtcday += buffer;
      } else {
        dtostrf(now.day(), 2, 0, buffer);
        rtcday = String(buffer);
      }
      record += rtcday; record += " ";
      dtostrf(now.hour(), 2, 0, buffer);
      record += String(buffer); record += ":";
      if(now.minute() <= 9) {
        dtostrf(now.minute(), 1, 0, buffer);
        rtcmin += buffer;
      } else {
        dtostrf(now.minute(), 2, 0, buffer);
        rtcmin = String(buffer);
      }
      record += rtcmin; record += ":";
      if(now.second() <= 9) {
        dtostrf(now.second(), 1, 0, buffer);
        rtcsec += buffer;
      } else {
        dtostrf(now.second(), 2, 0, buffer);
        rtcsec = String(buffer);
      }
      record += rtcsec; record += ",";
    #endif
    
    #if defined(ENABLE_BMP)
      int temp = bmp.readTemperature() * 100;
      long pressure = bmp.readPressure();
      long meters = bmp.readAltitude() * 100;
      long cmeters = bmp.readAltitude(curSeaPres) * 100;
      record += temp; record += ",";
      record += pressure; record += ",";
      record += meters; record += ",";
      record += cmeters; record += ",";
    #endif
    
    #if defined(ENABLE_AXL) && defined(ENABLE_CMP)
      if(areConnected)
      {
        MagnetometerScaled magnetometerReadings = compass.ReadScaledAxis();
        AccelerometerScaled accelerometerReadings = accel.ReadScaledAxis();
        
        float headingNTC = CalculateHeadingNotTiltCompensated(magnetometerReadings);
        float headingTC = CalculateHeadingTiltCompensated(magnetometerReadings, accelerometerReadings);
        
        int heading = headingTC * 100;
        record += heading; record += ",";
      }
    #endif
      
    
    if (SD.exists(filename)) {
        logFile = SD.open(filename, FILE_WRITE);
      logFile.println(record);
      logFile.close();
      #if defined(ENABLE_XB)
        mySerial.println(record);
      #endif
      #if defined(ENABLE_SERIAL)
        Serial.println(record);
      #endif
    }
  #endif
  
 delay(5000);
 
}

float CalculateHeadingTiltCompensated(MagnetometerScaled mag, AccelerometerScaled acc)
{
  // We are swapping the accelerometers axis as they are opposite to the compass the way we have them mounted.
  // We are swapping the signs axis as they are opposite.
  // Configure this for your setup.
  float accX = -acc.YAxis;
  float accY = -acc.XAxis;
  
  float rollRadians = asin(accY);
  float pitchRadians = asin(accX);
  
  // We cannot correct for tilt over 40 degrees with this algorthem, if the board is tilted as such, return 0.
  if(rollRadians > 0.78 || rollRadians < -0.78 || pitchRadians > 0.78 || pitchRadians < -0.78)
  {
    return 0;
  }
  
  // Some of these are used twice, so rather than computing them twice in the algorithem we precompute them before hand.
  float cosRoll = cos(rollRadians);
  float sinRoll = sin(rollRadians);  
  float cosPitch = cos(pitchRadians);
  float sinPitch = sin(pitchRadians);
  
  float Xh = mag.XAxis * cosPitch + mag.ZAxis * sinPitch;
  float Yh = mag.XAxis * sinRoll * sinPitch + mag.YAxis * cosRoll - mag.ZAxis * sinRoll * cosPitch;
  
  float heading = atan2(Yh, Xh);
    
  return heading;
}

float CalculateHeadingNotTiltCompensated(MagnetometerScaled mag)
{
   // Calculate heading when the magnetometer is level, then correct for signs of axis.
   float heading = atan2(mag.YAxis, mag.XAxis);
   return heading;
}

float RadiansToDegrees(float rads)
{
  // Correct for when signs are reversed.
  if(rads < 0)
    rads += 2*PI;
      
  // Check for wrap due to addition of declination.
  if(rads > 2*PI)
    rads -= 2*PI;
   
  // Convert radians to degrees for readability.
  float heading = rads * 180/PI;
       
  return heading;
}
#endif

