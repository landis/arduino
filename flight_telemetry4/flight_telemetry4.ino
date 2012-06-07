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

// Serial print stream
ArduinoOutStream cout(Serial);

// buffer to format data - makes it easier to echo to Serial
char buf[80];

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
  
  // create a new file in root
  char name[] = "SAPHE000.CSV";
  
  for (uint8_t i = 0; i < 100; i++) {
    name[6] = i/10 + '0';
    name[7] = i%10 + '0';
    if (sd.exists(name)) continue;
    logfile.open(name);
    break;
  }
  if(!logfile.is_open()) error("file.open");
  
  cout << pstr("Logging to: ") << name << endl;
  cout << endl;
  #if ECHO_TO_XB
    xout << pstr("Logging to: ") << name << endl;
    xout << endl;
  #endif
  
  obufstream bout(buf, sizeof(buf));
  
  bout << pstr("millis");
  
  #if USE_DS1307
    bout << pstr(",date,time");
  #endif  // USE_DS1307
  
  logfile << buf << endl;
  
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
  
  // wait for time to be a multiple of interval
  do {
    m = millis();
  } while (m % LOG_INTERVAL);
  
  // use buffer stream to format line
  obufstream bout(buf, sizeof(buf));
  
  // start with time in millis
  bout << m;
  
  #if USE_DS1307
    DateTime now = rtc.now();
    bout << ',' << now;
  #endif  //USE_DS1307
  
  bout << endl;
  
  //log data and flush to SD
  logfile << buf << flush;
  
  // check for error
  if (!logfile) error("write data failed");
  
  #if ECHO_TO_SERIAL
    cout << buf;
  #endif  //ECHO_TO_SERIAL
  
  #if ECHO_TO_XB
    xout << buf;
    xout << endl;
  #endif
  
  // dont log two points in the same millis
  if (m == millis()) delay(1);
}
