/* Project SAPHE */

#define ENABLE_SD
#define ENABLE_XB
#define ENABLE_BMP

#if defined(ENABLE_SD)
  #include <SD.h>
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
#ifdef ENABLE_BMP
  #include <Wire.h>
#endif

#ifdef ENABLE_BMP
  #include <Adafruit_BMP085.h>
  Adafruit_BMP085 bmp;
  uint32_t curSeaPres = 101500;  //Current Sea Level Pressure
#endif

void setup()
{
  mySerial.begin(57600);
  
  #if defined(ENABLE_SD)
    pinMode(sdCS, OUTPUT);
    pinMode(sdELED, OUTPUT);
    if (!SD.begin(sdCS)) {
      mySerial.println("Card failed, or not present");
      digitalWrite(sdELED, HIGH);
    }
    else {
      mySerial.println("Card is initialized");
      
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
  
  
  #ifdef ENABLE_BMP
    bmp.begin();
  #endif
  
  /* SD Card Log Header */
  #if defined(ENABLE_SD)
    #if defined(ENABLE_BMP)
      String header = "degC,Pa,EstMeters,RealMeters,";
    #endif
    logFile.println(header);
    logFile.close();
    #if defined(ENABLE_XB)
      mySerial.println(header);
    #endif
  #endif
}


void loop()
{
  #if defined(ENABLE_SD) && defined(ENABLE_BMP)
    String record;
    char temp[6]; dtostrf(bmp.readTemperature(), 4, 2, temp); 
    record = String(temp); record += ",";
    record += bmp.readPressure(); record += ",";
    char metr[6]; dtostrf(bmp.readAltitude(), 4, 2, metr);
    record += metr; record += ",";
    char cmetr[6]; dtostrf(bmp.readAltitude(curSeaPres), 4, 2, cmetr);
    record += cmetr; record += ",";
    if (SD.exists(filename)) {
      logFile = SD.open(filename, FILE_WRITE);
      logFile.println(record);
      logFile.close();
      #if defined (ENABLE_XB)
        mySerial.println(record);
      #endif
    }
  #endif
    
  #if defined(ENABLE_XB)
    delay(1000);
  #endif
}
