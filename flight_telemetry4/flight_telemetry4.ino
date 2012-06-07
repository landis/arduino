#include <SdFat.h>
#include <SdFatUtil.h>

#define ECHO_TO_XB       1
#define CHIP_SELECT      8
#define USE_DS1307       1
#define ECHO_TO_SERIAL   1
#define LOG_INTERVAL     1000

// file system object
SdFat sd;

// text file for logging
ofstream logfile;

char name1[] = "SAPHE000.CSV";
char name2[] = "NMEA0000.CSV";

// Serial print stream
ArduinoOutStream cout(Serial);

// buffer to format data - makes it easier to echo to Serial
char buf[90];
char guf[90];

// store error strings in flash to save RAM
#define error(s) sd.errorHalt_P(PSTR(s))

#if USE_DS1307
  #include <Wire.h>
  #include <RTClib.h>
  RTC_DS1307 rtc;  // define the RTC object

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
#endif  // USE_DS1307

#if ECHO_TO_XB
  #include <SoftwareSerial.h>
  SoftwareSerial xbSerial(2, 3); // RX, TX
  ArduinoOutStream xout(xbSerial);
#endif

void setup() 
{
  Serial.begin(9600);
  
  #if ECHO_TO_XB
    xbSerial.begin(57600);
  #endif
  
  // pstr stores strings in flash to save RAM
  cout << endl << pstr("FreeRam: ") << FreeRam() << endl;
  #if ECHO_TO_XB
    xout << endl << pstr("FreeRam: ") << FreeRam() << endl;
  #endif
  
  #if USE_DS1307
    // connect to RTC
    Wire.begin();
    if (!rtc.begin()) error("RTC failed");
    
    // set date time callback function
    SdFile::dateTimeCallback(dateTime);
    DateTime now = rtc.now();
    cout << now << endl;
    #if ECHO_TO_XB
      xout << now << endl;
    #endif
  #endif  // USE_DS1307
  
  // initialize the SD card at SPI_HALF_SPEED
  if (!sd.begin(CHIP_SELECT, SPI_FULL_SPEED)) sd.initErrorHalt();
  
  // increment the file names
  for (uint8_t i = 0; i < 100; i++) {
    name1[6] = i/10 + '0';
    name1[7] = i%10 + '0';
    if (sd.exists(name1)) continue;
      logfile.open(name1);
    break;
  }
  if(!logfile.is_open()) error("log.open");
  logfile.close();
  
  for (uint8_t i = 0; i < 100; i++) {
    name2[6] = i/10 + '0';
    name2[7] = i%10 + '0';
    if (sd.exists(name2)) continue;
      logfile.open(name2);
    break;
  }
  if(!logfile.is_open()) error("gps.open");
  logfile.close();
  
  cout << pstr("Logging to: ") << name1 << endl;
  cout << pstr("GPS Log: ") << name2 << endl;
  cout << endl;
  #if ECHO_TO_XB
    xout << pstr("Logging to: ") << name1 << endl;
    xout << pstr("GPS Log: ") << name2 << endl << endl;
  #endif
  
  obufstream bout(buf, sizeof(buf));
  
  bout << pstr("secs");
  
  #if USE_DS1307
    bout << pstr(",date,time");
  #endif  // USE_DS1307
  
  logfile.open(name1);
  logfile << buf << endl << flush;
  logfile.close();
  
  #if ECHO_TO_SERIAL
    cout << buf << endl;
  #endif  // ECHO_TO_SERIAL
  
  #if ECHO_TO_XB
    xout << buf << endl;
  #endif
}

void loop() 
{
  uint32_t m;
  uint32_t s;
  
  // wait for time to be a multiple of interval
  do {
    m = millis();
    s = m/1000;
  } while (m % LOG_INTERVAL);
  
  // use buffer stream to format line
  obufstream bout(buf, sizeof(buf));
  // different buffer for gps stream
  obufstream gout(guf, sizeof(guf));
  
  // start with time in secs
  bout << s;
  gout << s;
  
  #if USE_DS1307
    DateTime now = rtc.now();
    bout << ',' << now;
    gout << ',' << now;
  #endif  //USE_DS1307
  
  //temp marker for gps lines
  gout << " gps";
  
  bout << endl;
  gout << endl;
  
  //log data and flush to SD
  logfile.open(name1);
  logfile << buf << flush;
  if (!logfile) error("write log data failed");
  logfile.close();
  
  logfile.open(name2);
  logfile << guf << flush;
  if (!logfile) error("write gps data failed");
  logfile.close();

  #if ECHO_TO_SERIAL
    cout << buf;
    cout << guf;
  #endif  //ECHO_TO_SERIAL
  
  #if ECHO_TO_XB
    xout << buf << endl;
    xout << guf << endl;
  #endif
  
  // dont log two points in the same millis
  if (m == millis()) delay(1);
}
