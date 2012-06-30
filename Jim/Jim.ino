//------------------------------------------------------------------------------
// module selection
#define ENABLE_XB         1
#define ENABLE_WDT        0
#define ENABLE_SD         1
#define ENABLE_GPS        1
#define ENABLE_RTC        1
#define ENABLE_THERM      0
#define ENABLE_ADXL       0
#define ENABLE_COMP       0
#define ENABLE_DEBUG      1
#define WDT_TIME          WDTO_8S
#define SPI_SPD           SPI_HALF_SPEED

//------------------------------------------------------------------------------
// module options
const uint32_t XB_BAUD_RATE       = 57600;            // xbee baud
const uint32_t GPS_BAUD_RATE      = 9600;             // gps default baud
const uint32_t MAX_SYNC_TIME_MSEC = 3000;             // max sync time sd card
 char          GNAME[]            = "GPS00000.BIN";   // gps log file template
 char          SNAME[]            = "SEN00000.CSV";   // sensor log file template
const uint8_t  chipSelect         = 8;                // sd select
const uint8_t  ERROR_LED_PIN      = 7;                // error led
const uint8_t  ERROR_INIT         = 1;                // SD init error
const uint8_t  ERROR_OPEN         = 2;                // file open error
const uint8_t  ERROR_SERIAL       = 3;                // serial error
const uint8_t  ERROR_WRITE        = 4;                // SD write or sync error
      

//------------------------------------------------------------------------------
//variables
uint32_t syncTime = 0;
 uint8_t buf[256]; 
    char buf2[256];

//------------------------------------------------------------------------------
// library includes
#if ENABLE_WDT
  #include <avr/wdt.h>
#endif

#if ENABLE_SD
  #include <SdFat.h>
  #include <SdFatUtil.h>
  SdFat sd;
  SdFile file;
  ofstream logfile;
#endif

#if ENABLE_GPS
  #include <SerialPort.h>
  SerialPort<0, 4096, 0> NewSerial;
#endif

#if ENABLE_XB
  #include <SoftwareSerial.h>
  SoftwareSerial xb(2, 3);
  ArduinoOutStream xout(xb);
#endif

#if ENABLE_RTC
  #include <Wire.h>
  #include <RTClib.h>
  RTC_DS1307 rtc;
#endif

//------------------------------------------------------------------------------
// functions
// error file write
void errorBlink(uint8_t errno) {
  uint8_t i;
  while (1) {
    for (i = 0; i < errno; i++) {
      digitalWrite(ERROR_LED_PIN, HIGH);
      delay(200);
      digitalWrite(ERROR_LED_PIN, LOW);
      delay(200);
    }
    delay(1600);
  }
}

// force reboot
void forceWDT() {
  while(1) {
    digitalWrite(ERROR_LED_PIN, HIGH);
  }
}

// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
    
      // return the date using FAT_DATE macro tor format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());
    
      // return the time using FAT_TIME macro tor format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
    }

// format date/time  
ostream& operator << (ostream& os, DateTime& dt) {
  os << dt.year() << '/' << int(dt.month()) << '/' << int(dt.day()) << ',';
  os << int(dt.hour()) << ':' << setfill('0') << setw(2) << int(dt.minute());
  os << ':' << setw(2) << int(dt.second()) << setfill(' ');
  return os;
}
  
//------------------------------------------------------------------------------
// setup
void setup() {
  pinMode(ERROR_LED_PIN, OUTPUT);
  
  #if ENABLE_WDT
    wdt_enable(WDT_TIME);
    wdt_reset();
  #endif
  
  #if ENABLE_XB
    xb.begin(57600);  
  #endif
  
  #if ENABLE_GPS
    NewSerial.begin(GPS_BAUD_RATE);
  #endif

  #if ENABLE_RTC || ENABLE_AXL
    Wire.begin();
  #endif
  
  #if ENABLE_RTC
    if (!rtc.begin()) errorBlink(ERROR_INIT);
    SdFile::dateTimeCallback(dateTime);
    DateTime now = rtc.now();
  #endif 
  
  #if ENABLE_SD
    if (!sd.init()) {
      errorBlink(ERROR_INIT);
    }
  #endif
  
  #if ENABLE_SD && ENABLE_GPS
 //   if (!sd.begin(chipSelect, SPI_SPD)) errorBlink(ERROR_INIT);
  
    for (uint8_t i = 0; i < 100000; i++) {
      GNAME[3] = (i / 10000) % 10 + '0';
      GNAME[4] = (i / 1000) % 10 + '0';
      GNAME[5] = (i / 100) % 10 + '0';
      GNAME[6] = (i / 10) % 10 + '0';
      GNAME[7] = i % 10 + '0';
      if (sd.exists(GNAME)) continue;
        logfile.open(GNAME);
      break;
    }
  
    if (!logfile.is_open()) {
      errorBlink(ERROR_OPEN);
    }
    logfile.close();
  
 //   if (SdFat.fileSize() == 0) {
 //     // make sure first cluster is allocated
 //     logfile.write((uint8_t)0);
 //     logfile.rewind();
 //     logfile.sync();
 //     logfile.close();
 //   }
  #endif
  
  #if ENABLE_SD && ENABLE_RTC
    for (uint8_t i = 0; i < 100000; i++) {
      SNAME[3] = (i / 10000) % 10 + '0';
      SNAME[4] = (i / 1000) % 10 + '0';
      SNAME[5] = (i / 100) % 10 + '0';
      SNAME[6] = (i / 10) % 10 + '0';
      SNAME[7] = i % 10 + '0';
      if (sd.exists(SNAME)) continue;
        logfile.open(SNAME);
      break;
    }
  
    if (!logfile.is_open()) {
      errorBlink(ERROR_OPEN);
    }
    logfile.close();
//    if (logfile.fileSize() == 0) {
      // make sure first cluster is allocated
//      logfile.write((uint8_t)0);
//      logfile.rewind();
//      logfile.sync();
//      logfile.close();
//    }
  #endif
  
  #if ENABLE_DEBUG
    xout << endl << pstr("FreeRam: ") << FreeRam() << endl;
    #if ENABLE_RTC
      xout << now << endl;
    #endif
    #if ENABLE_SD
      xout << pstr("GPS File: ") << GNAME << endl;
      xout << pstr("Sensor File: ") << SNAME << endl;
    #endif
  #endif
  
  obufstream bout(buf2, sizeof(buf2));
  bout << pstr("secs");
  #if ENABLE_RTC
    bout << pstr(",date,time");
  #endif
  
  #if ENABLE_THERM
    bout << pstr(",t_adc,resistance,ext_temp_c");
  #endif
  
  #if ENABLE_BARO
    bout << pstr(",int_temp_c,pascals,meters");
  #endif
  
  #if ENABLE_ADXL
    bout << pstr(",freefall,inactivity,activity,doubletap,tap,x,y,z,xG,yG,zG");
  #endif
  
  #if ENABLE_COMP
    bout << pstr("mx,my,mz,heading");
  #endif
  
  #if ENABLE_SD
    logfile.open(SNAME);
    logfile << buf2 << endl << flush;
    logfile.close();
  #endif
  
  #if ENABLE_XB
   // xout << pstr("Header") << endl << buf2 << endl;
  #endif
}

//------------------------------------------------------------------------------
// loop
void loop() {
  #if ENABLE_GPS
    if (NewSerial.getRxError()) {
      errorBlink(ERROR_SERIAL);    
    }
    
    uint8_t n = NewSerial.read(buf, sizeof(buf));
    logfile.open(GNAME);
    logfile << buf;
    xout << buf << endl;;
    xout << pstr("yo") << endl;

    if ((millis() - syncTime) < MAX_SYNC_TIME_MSEC) {
      if (!logfile.flush()) {
        errorBlink(ERROR_WRITE); 
      }
    }
    syncTime = millis();
    logfile.close();
    
  #endif
}
