/*** to use the SD Card datalogger shield with an Arduino Mega:
     change: #define MEGA_SOFT_SPI 0
         to: #define MEGA_SOFT_SPI 1
     in the file libraries\SD\utility\Sd2Card.h 
***/
 
/*** Configuration options ***/

/* Sensors */
#define ENABLE_BMP085
#define ENABLE_ADXL
#define ENABLE_SDLOG
#define ENABLE_TFT
#define ENABLE_GPS
//#define ENABLE_THERM

/* GPS */
#ifdef ENABLE_GPS
  #include <Adafruit_GPS.h>
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(50, 40);
  //RX40 TX50
  #define GPSECHO false
  Adafruit_GPS GPS(&mySerial);
  boolean usingInterrupt = false;
#endif

/* SD Shield */
#ifdef ENABLE_SDLOG
  #include <SD.h>
  #include "RTClib.h"
  int redLEDpin = 2;
  int greenLEDpin = 3;
  RTC_DS1307 RTC;
  const int sdCS = 10;
#endif

/* TFT Screen */
#ifdef ENABLE_TFT
  // 47 = red
  // 51 = yellow
  // 52 = green
  // 53 = orange
  #define tftcs 53
  #define tftdc 47
  #define rst 8
  #include <Adafruit_GFX.h>     //Core graphics library
  #include <Adafruit_ST7735.h>  //Hardware specific library
  #include <SPI.h>
  Adafruit_ST7735 tft = Adafruit_ST7735(tftcs, tftdc, rst);
#endif

/* I2C */
#if defined(ENABLE_BMP085) || defined(ENABLE_ADXL)
  #include <Wire.h>
  #include <Math.h>
#endif

/* Barometer */
#ifdef ENABLE_BMP085
  #include <Adafruit_BMP085.h>
  Adafruit_BMP085 dps;
  float Temperature = 0, Pressure = 0, Altitude = 0, calcAltitude = 0;
  uint32_t nextBmp085 = 0;
  //#define curPress 101964
#endif /* ENABLE_BMP085 */

/* Accelerometer */
#ifdef ENABLE_ADXL
  #include <ADXL345.h>
  ADXL345 accel;
#endif

/* Thermocouple */
#ifdef ENABLE_THERM
  #include <max6675.h>
  //change these pins
  int thermoDO = 36;
  int thermoCS = 37;
  int thermoCLK = 39;
  MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
  uint8_t degree[8]  = {140,146,146,140,128,128,128,128};
#endif

void setup() 
{
  Serial.begin(115200);
  Serial.println("SAPHE Telemetry Payload");
  
    /* Setup the SD Log */
  #ifdef ENABLE_SDLOG
    Wire.begin();
    RTC.begin();
    digitalWrite(greenLEDpin, HIGH);
    digitalWrite(redLEDpin, HIGH);

    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      RTC.adjust(DateTime(__DATE__, __TIME__));
    }

    Serial.print("Initializing SD Card...");
    pinMode(sdCS, OUTPUT);
    if (!SD.begin(sdCS)) {
      digitalWrite(greenLEDpin, LOW);
      Serial.println("Card failed, or not present");
      delay(500);
      digitalWrite(redLEDpin, LOW);
      return;
    }
    digitalWrite(redLEDpin, LOW);
    Serial.println("card initialized.");
    delay(500);
    digitalWrite(greenLEDpin, LOW);
    
    File logFile = SD.open("LOG001.csv", FILE_WRITE);
    if (logFile)
    {
      digitalWrite(greenLEDpin, HIGH);
      String header = "ID,RTCtimestamp";
      #if defined(ENABLE_ADXL)
        header += ",X,Y,Z";
      #endif
      #if defined(ENABLE_BMP085)
        header += ",TempC,PressurePA,AltitudeM,CAltitudeM";
      #endif
      #if defined(ENABLE_GPS)
        header += ",Latitude,Longitude,Knots,GPSAltitude,Satelites,Fix,Quality,Angle,GPStimestamp";
      #endif
      #if defined(ENABLE_THERM)
        header += ",ThermTempC";
      #endif
      logFile.println(header);
      logFile.close();
      digitalWrite(greenLEDpin, LOW);
      //Serial.println(header);
    }
    else
    {
      digitalWrite(redLEDpin, HIGH);
      Serial.println("Couldn't open log file");
    }
  #endif /* ENABLE_SDLOG */

  /* GPS Setup */
  #if defined(ENABLE_GPS)
    GPS.begin(9600);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    useInterrupt(true);
    delay(1000);
  #endif
  
  /* Setup the TFT */
  #if defined(ENABLE_TFT)
    tft.initR(INITR_REDTAB);
    tft.fillScreen(ST7735_BLACK);
    tft.setRotation(1);
    tft.drawString(0, 0, "Project SAPHE", ST7735_WHITE);
  #endif
  
  #if defined(ENABLE_TFT) && defined(ENABLE_GPS)
    tft.setRotation(1);
    tft.drawString(0, 50, "Lat", ST7735_WHITE);
    tft.drawString(80, 50, "Lon", ST7735_WHITE);
    tft.drawString(0, 80, "Speed", ST7735_WHITE);
    tft.drawString(50, 80, "Altitude", ST7735_WHITE);
    tft.drawString(120, 80, "Sats", ST7735_WHITE);
  #endif
  
  #if defined(ENABLE_TFT) && defined(ENABLE_BMP085)
    tft.setRotation(1);
    tft.drawString(0, 110, "Pa", ST7735_WHITE);
    tft.drawString(50, 110, "Int Temp", ST7735_WHITE);
  #endif
  
  #if defined(ENABLE_TFT) && defined(ENABLE_THERM)
    tft.drawString(110, 110, "Ext Temp", ST7735_WHITE);
  #endif
        
  /* I2C */
  #if defined(ENABLE_BMP085) || defined(ENABLE_ADXL)
    Wire.begin();
    delay(1);
  #endif
  
  #if defined(ENABLE_BMP085)
    dps.begin(BMP085_STANDARD); // Initialize for relative altitude    
    ;
    nextBmp085 = millis() + 1000;
    delay(500);
  #endif /* ENABLE_BMP085 */

  #if defined(ENABLE_ADXL)
    accel = ADXL345();
    if(accel.EnsureConnected()){
      Serial.println("Connected to ADXL345");
    }
    else {
      Serial.println("Could not connect to ADXL");
    }
    accel.EnableMeasurements();
  #endif
  delay(1000);
}

#ifdef ENABLE_GPS
  /* GPS Interupt */
  // Interrupt is called once a millisecond, looks for any new GPS data, and stores it
  SIGNAL(TIMER0_COMPA_vect) {
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) UDR0 = c;  
      // writing direct to UDR0 is much much faster than Serial.print 
      // but only one character can be written at a time. 
  }

  void useInterrupt(boolean v) {
    if (v) {
      // Timer0 is already used for millis() - we'll just interrupt somewhere
      // in the middle and call the "Compare A" function above
      OCR0A = 0xAF;
      TIMSK0 |= _BV(OCIE0A);
      usingInterrupt = true;
    } else {
      // do not call the interrupt function COMPA anymore
      TIMSK0 &= ~_BV(OCIE0A);
      usingInterrupt = false;
    }
  }

  uint16_t timer = millis();
#endif

void loop()
{
  #if defined(ENABLE_GPS)
    if (! usingInterrupt) {
      // read data from the GPS in the 'main loop'
      char c = GPS.read();
      // if you want to debug, this is a good time to do it!
      if (GPSECHO)
        if (c) UDR0 = c;
        // writing direct to UDR0 is much much faster than Serial.print 
        // but only one character can be written at a time. 
     }
  
    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
      // a tricky thing here is if we print the NMEA sentence, or data
      // we end up not listening and catching other sentences! 
      // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
      //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
      if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
        return;  // we can fail to parse a sentence in which case we should just wait for another
    }
  
    // approximately every 2 seconds or so, print out the current stats
    if (millis() - timer > 2000) { 
      timer = millis(); // reset the timer
//      Serial.print("\nTime: ");
//      Serial.print(GPS.hour, DEC); Serial.print(':');
//      Serial.print(GPS.minute, DEC); Serial.print(':');
//      Serial.print(GPS.seconds, DEC); Serial.print('.');
//      Serial.println(GPS.milliseconds);
//      Serial.print("Date: ");
//      Serial.print(GPS.day, DEC); Serial.print('/');
//      Serial.print(GPS.month, DEC); Serial.print("/20");
//      Serial.println(GPS.year, DEC);
//      Serial.print("Fix: "); Serial.print(GPS.fix);
//      Serial.print(" quality: "); Serial.println(GPS.fixquality); 
//      if (GPS.fix) {
//        Serial.print("Location: ");
//        Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
//        Serial.print(", "); 
//        Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
//        Serial.print("Speed (knots): "); Serial.println(GPS.speed);
//        Serial.print("Angle: "); Serial.println(GPS.angle);
//        Serial.print("Altitude: "); Serial.println(GPS.altitude);
//        Serial.print("Satellites: "); Serial.println(GPS.satellites);
//      }
    }
  #endif

  // IF SD Datalogger shield enabled, Put RTC info into variables //
  #if defined(ENABLE_SDLOG)
    String utime;
    String rtimestamp;
    String comma = ","; String colon = ":";
    String slash = "/"; String space = " ";
    String dot = ".";
    DateTime now = RTC.now();
    utime = (String) now.unixtime();
    char ryear[5]; dtostrf(now.year(), 4, 0, ryear);
    char rmonth[3]; dtostrf(now.month(), 2, 0, rmonth);
    char rday[3]; dtostrf(now.day(), 2, 0, rday);
    char rhour[3]; dtostrf(now.hour(), 2, 0, rhour);
    char rmin[3]; dtostrf(now.minute(), 2, 0, rmin);
    char rsec[3]; dtostrf(now.second(), 2, 0, rsec);

    if (! RTC.isrunning()) {
      rtimestamp = "RTC is NOT running!";
    }
    else {
      rtimestamp = (ryear + slash + rmonth + slash + rday + space
                  + rhour + colon + rmin + colon + rsec);
    }
  #endif

  // IF GPS is connected and SD Logger, read into variables //
  #if defined(ENABLE_GPS) && defined(ENABLE_SDLOG)
    char lat[10]; dtostrf(GPS.latitude, 9, 4, lat);
    char lon[10]; dtostrf(GPS.longitude, 9, 4, lon);
    char gspd[10]; dtostrf(GPS.speed, 4, 1, gspd);
    char galt[10]; dtostrf(GPS.altitude, 7, 0, galt);
    char gsat[5]; dtostrf(GPS.satellites, 3, 0, gsat);
    char ghour[3]; dtostrf(GPS.hour, 2, 0, ghour);
    char gmin[3]; dtostrf(GPS.minute, 2, 0, gmin);
    char gsec[4]; dtostrf(GPS.seconds, 2, 0, gsec);
    char gday[4]; dtostrf(GPS.day, 2, 0, gday);
    char gmon[2]; dtostrf(GPS.month, 1, 0, gmon);
    char gyeara[3]; dtostrf(GPS.year, 2, 0, gyeara);
    String milen("20");
    String gyearc = String(milen + gyeara);
    char gfix[5]; dtostrf(GPS.fix, 2, 0, gfix);
    char gqual[5]; dtostrf(GPS.fixquality, 2, 0, gqual);
    char gang[5]; dtostrf(GPS.angle, 3, 0, gang);
  #endif

  // IF BMP enabled, read values into variables //
  #ifdef ENABLE_BMP085
    if (millis() > nextBmp085) {
      Temperature = dps.readTemperature();
      Pressure = dps.readPressure();
      Altitude = dps.readAltitude();
      calcAltitude = dps.readAltitude(101964); //change curPress to actual pressure (PA) as reported weather.com
//      Serial.print("Temp(C):");
//      Serial.print(Temperature);
//      Serial.print("  Pressure(Pa):");
//      Serial.print(Pressure);
//      Serial.print("  Alt(m):");
//      Serial.print(Altitude);
//      Serial.print("  Calc Altitude:");
//      Serial.println(calcAltitude);
    }
  #endif 

  // IF SD Datalogger and BMP085, gather BMP info into variables //
  #if defined(ENABLE_BMP085) && defined(ENABLE_SDLOG)
    char temp[6]; dtostrf(Temperature, 4, 2, temp);
    char press[9]; dtostrf(Pressure, 6, 0, press);
    char alt[8]; dtostrf(Altitude, 4, 1, alt); 
    char calt[9]; dtostrf(calcAltitude, 8, 1, calt);
  #endif
  
  // IF ADXL enabled, grab info into variables //
  #if defined(ENABLE_ADXL)
    AccelerometerRaw raw = accel.ReadRawAxis();
    int xAxisRawData = raw.XAxis;
    int yAxisRawData = raw.YAxis;
    int zAxisRawData = raw.ZAxis;
    AccelerometerScaled scaled = accel.ReadScaledAxis();
    float xAxisGs = scaled.XAxis;
//    Serial.print("X: ");
//    Serial.print(raw.XAxis);
//    Serial.print("   Y: ");
//    Serial.print(raw.YAxis);
//    Serial.print("   Z: ");
//    Serial.println(raw.ZAxis);
  #endif
  
  #if defined(ENABLE_THERM)
//    Serial.println(thermocouple.readCelsius());
//    Serial.println(thermocouple.readFarenheit());
    double cels = thermocouple.readCelsius();
    char ccels[10]; dtostrf(cels, 7, 2, ccels);
  #endif
  
  #if defined(ENABLE_SDLOG) && defined(ENABLE_BMP085) && defined(ENABLE_ADXL)
    String record;

/* RTC */    
    record = (utime + comma + ryear + slash + rmonth + slash + rday + 
              space + rhour + colon + rmin + colon + rsec + comma);
/* AXL */
    #if defined(ENABLE_ADXL)
      record += (xAxisRawData + comma + yAxisRawData + comma + zAxisRawData + comma);
    #endif  

/* BMP */
    #if defined(ENABLE_BMP085)
      record += (temp + comma + press + comma + alt + comma + calt + comma);
    #endif

/* GPS */
    #if defined(ENABLE_GPS)
      record += (lat + comma + lon + comma + gspd + comma + galt + comma + 
                 gsat + comma + gfix + comma + gqual + comma + gang + comma +
                 gyearc + slash + gmon + slash + gday + space + 
                 ghour + colon + gmin + colon + gsec + comma);
    #endif
  
/* THM */
    #if defined(ENABLE_THERM)
      record += (ccels);
    #endif
  
//    Serial.println();
//    String testgtime = (gyearc + slash + gmon + slash + gday + space + ghour + colon + gmin + colon + gsec + comma);
//    Serial.println(testgtime);
//    Serial.println();
    File dataFile = SD.open("LOG001.csv", FILE_WRITE);
    if (dataFile)
    {
      digitalWrite(greenLEDpin, HIGH);
      dataFile.println(record);
      dataFile.close();
      digitalWrite(greenLEDpin, LOW);
      Serial.println(record);
    }
    else
    {
      digitalWrite(redLEDpin, HIGH);
      Serial.println("error opening LOG001.csv");
    }
  #endif
  
  #if defined(ENABLE_TFT) && defined(ENABLE_SDLOG)
    char time_char[32];
    rtimestamp.toCharArray(time_char, 32);
    tft.setRotation(1);
    tft.drawString(0, 20, time_char, ST7735_WHITE);
    //tft.drawString(0, 20, rtimestamp, ST7735_WHITE);
    tft.drawString(0, 30, "LOG001.CSV", ST7735_WHITE);
  #endif
  
  #if defined(ENABLE_TFT) && defined(ENABLE_GPS)
    tft.setRotation(1);
        //4002.2719N, 7503.6269W
    tft.drawString(0, 60, lat, ST7735_WHITE);
    tft.drawString(80, 60, lon, ST7735_WHITE);
    tft.drawString(0, 90, gspd, ST7735_WHITE);
    tft.drawString(50, 90, galt, ST7735_WHITE);
    tft.drawString(120, 90, gsat, ST7735_WHITE);
  #endif

  #if defined(ENABLE_TFT) && defined(ENABLE_BMP085)
    tft.setRotation(1);
    tft.drawString(0, 120, press, ST7735_WHITE);
    tft.drawString(50, 120, temp, ST7735_WHITE);
  #endif
  
  #if defined(ENABLE_TFT) && defined(ENABLE_THERM)
    //Serial.println(scels);
    //char scels_char[scels.length() + 1];
    //cels.toCharArray(scels_char, sizeof(scels_char));
    tft.setRotation(1);
    tft.drawString(100, 120, ccels, ST7735_WHITE);
  #endif
  
//  delay(10);
//  digitalWrite(redLEDpin, LOW);
//  digitalWrite(greenLEDpin, LOW);
  delay(1000);
}
