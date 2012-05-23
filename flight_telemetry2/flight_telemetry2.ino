/***
RTC
3.3V to VCC
GND to GND
A4 to RTC SDA
A5 to RTC SCL

GPS
3.3V to VIN
GND to GND
D3 to GPS TX
D2 to GPS RX

/*** Configuration ***/
#define ENABLE_RTC
#define ENABLE_SERIAL
#define ENABLE_GPS
#define ENABLE_SOFTSERIAL
//#define ENABLE_HARDSERIAL
//#define ENABLE_INT

//ENABLE_RTC | Chronodot ***?
#ifdef ENABLE_RTC
  #include <Wire.h>
  #include <RTClib.h>
  RTC_DS1307 RTC;
#endif

//ENABLE_GPS | MTK3339 chipset
#ifdef ENABLE_GPS
  #include <Adafruit_GPS.h>
  #if ARDUINO >= 100
    #include <SoftwareSerial.h>
  #else
    #include <NewSoftSerial.h>
  #endif
#endif

#ifdef ENABLE_SOFTSERIAL
  #if ARDUINO >= 100
    SoftwareSerial mySerial(3, 2);
  #else
    NewSoftSerial mySerial(3, 2);
  #endif
  Adafruit_GPS GPS(&mySerial);
#endif

#ifdef ENABLE_HARDSERIAL
  Adafruit_GPS GPS(&Serial1);
#endif

#if defined(ENABLE_SERIAL) && defined(ENABLE_GPS)
  #define GPSECHO true
#else
  #if defined(ENABLE_GPS)
    #define GPSECHO false
  #endif
#endif

#if defined(ENABLE_GPS) && defined(ENABLE_INT)
  boolean usingInterrupt = true;
  void useInterrupt(boolean);
#else
  #if defined(ENABLE_GPS)
    boolean usingInterrupt = false;
    void useInterrupt(boolean);
  #endif
#endif

void setup() {
  Serial.begin(115200);
  
  #ifdef ENABLE_RTC
    Wire.begin();
    RTC.begin();
    if (! RTC.isrunning()) {
      #ifdef ENABLE_SERIAL
        Serial.println("RTC is NOT running!");
      #endif
    }
  #endif
  
    #ifdef ENABLE_GPS
      #ifdef ENABLE_SERIAL
        Serial.println("Adafruit GPS logging start test!");
      #endif
    
      GPS.begin(9600);
      GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
      GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

      useInterrupt(  true);

      while (true) {
        #ifdef ENABLE_SERIAL
          Serial.print("Starting logging....");
          if (GPS.LOCUS_StartLogger()) {
            Serial.println(" STARTED!");
            break;
          } else {
            Serial.println(" no response :(");
          }
        #endif
      }
    #endif
}

void loop() {
  #ifdef ENABLE_RTC
    DateTime now = RTC.now();
    #ifdef ENABLE_SERIAL
      Serial.println();
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(' ');
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();
    #endif
  #endif
  
  #ifdef ENABLE_GPS
    delay(1000);
    if (GPS.LOCUS_ReadStatus()) {
      Serial.print("\n\nLog #");
      Serial.print(GPS.LOCUS_serial, DEC);
      if (GPS.LOCUS_type == LOCUS_OVERLAP)
        Serial.print(", Overlap, ");
      else if (GPS.LOCUS_type == LOCUS_FULLSTOP)
        Serial.print(", Full Stop, Logging");
      
      if (GPS.LOCUS_mode & 0x1) Serial.print(" AlwaysLocate");
      if (GPS.LOCUS_mode & 0x2) Serial.print(" FixOnly");
      if (GPS.LOCUS_mode & 0x4) Serial.print(" Normal");
      if (GPS.LOCUS_mode & 0x8) Serial.print(" Interval");
      if (GPS.LOCUS_mode & 0x10) Serial.print(" Distance");
      if (GPS.LOCUS_mode & 0x20) Serial.print(" Speed");
      
      Serial.print(", Content "); Serial.print((int)GPS.LOCUS_config);
      Serial.print(", Interval "); Serial.print((int)GPS.LOCUS_interval);
      Serial.print(" sec, Distance "); Serial.print((int)GPS.LOCUS_distance);
      Serial.print(" m, Speed "); Serial.print((int)GPS.LOCUS_speed);
      Serial.print(" m/s, Status "); 
      if (GPS.LOCUS_status) 
        Serial.print("LOGGING, ");
      else 
        Serial.print("OFF, ");
      Serial.print((int)GPS.LOCUS_records); Serial.print(" Records, ");
      Serial.print((int)GPS.LOCUS_percent); Serial.print("% Used ");
    }
  #endif
}

/******************************************************************/
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
