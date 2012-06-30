//re-write with fat16lib's serial port logger as base
//also including watchdog timer after re-burning bootloader
//of mega2560 with fix for watchdog

#include <avr/wdt.h>

#include <SdFat.h>
#include <SdFatUtil.h>
#include <Wire.h>

#define ECHO_TO_XB       1
#define CHIP_SELECT      8
#define USE_DS1307       1
#define ECHO_TO_SERIAL   1
#define LOG_INTERVAL     1000
#define LED_PIN          7

// thermistor
  #define SERIESRESISTOR     9810
  #define THERMISTORPIN      A1
  // resistance at 25 degrees C
  #define THERMISTORNOMINAL  10000      
  // temp. for nominal resistance (almost always 25 C)
  #define TEMPERATURENOMINAL 25   
  // The beta coefficient of the thermistor (usually 3000-4000)
  #define BCOEFFICIENT       3950
// ****

// bmp085
  #include <BMP085.h>
  BMP085 dps = BMP085();
  int ALT_CM = 2400;
  long dpspress = 0, dpstemp = 0, dpsaltitude = 0;
// ****  

// adxl345
  #include <bildr_ADXL345.h>
  ADXL345 adxl;

// hmc5883l
  #include <HMC58X3.h>
  HMC58X3 magn;
  
// gps
  #include <Adafruit_GPS.h>
  Adafruit_GPS GPS(&Serial3);
  #define GPSECHO true
  boolean usingInterrupt = false;
  
  #define GPSBUFFSIZE 90
  char gpsbuffer[GPSBUFFSIZE];
  String gpsbuffer2;
  uint8_t bufferidx = 0;
  uint8_t fix = 0;
  
// read a Hex value and return the decimal equiv
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// file system object
  SdFat sd;

// text file for logging
  ofstream logfile;

  char name1[] = "SENS0000.CSV";
  char name2[] = "NMEA0000.BIN";

// Serial print stream
  ArduinoOutStream cout(Serial);

// buffer to format data - makes it easier to echo to Serial
  char buf[256];
  char guf[256];
  
// store error strings in flash to save RAM
  #define error(s) sd.errorHalt_P(PSTR(s))

  #if USE_DS1307
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

//==================  setup  ====================//
void setup() 
{
  Serial.begin(9600);
  
  //blinky
  pinMode(LED_PIN, OUTPUT);

  #if ECHO_TO_XB
    xbSerial.begin(57600);
  #endif

  // thermistor aref
  analogReference(EXTERNAL);

  //i2c
  Wire.begin();
  
  // adxl345 setup
  adxl.powerOn();
  adxl.setActivityThreshold(75);
  adxl.setInactivityThreshold(75);
  adxl.setTimeInactivity(10);
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(1);
  adxl.setInactivityX(1);
  adxl.setInactivityY(1);
  adxl.setInactivityZ(1);
  adxl.setTapDetectionOnX(0);
  adxl.setTapDetectionOnY(0);
  adxl.setTapDetectionOnZ(1);
  adxl.setTapThreshold(50);
  adxl.setTapDuration(15);
  adxl.setDoubleTapLatency(80);
  adxl.setDoubleTapWindow(200);
  adxl.setFreeFallThreshold(5);
  adxl.setFreeFallDuration(45);
  adxl.setInterruptMapping( ADXL345_INT_SINGLE_TAP_BIT,  ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_DOUBLE_TAP_BIT,  ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_FREE_FALL_BIT,  ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_ACTIVITY_BIT,   ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_INACTIVITY_BIT,  ADXL345_INT1_PIN );
  adxl.setInterrupt( ADXL345_INT_SINGLE_TAP_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_DOUBLE_TAP_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_FREE_FALL_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_ACTIVITY_BIT,  1);
  adxl.setInterrupt( ADXL345_INT_INACTIVITY_BIT, 1);
 
  // hmc5883l
  magn.init(false);
  //magn.calibrate(1, 32);
  magn.setMode(0);
  
  // gps
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PMTK_LOCUS_STARTLOG);
  useInterrupt(true);
  
  
// bmp085
  dps.init(MODE_ULTRA_HIGHRES, ALT_CM, true);
  delay(1000);

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
  for (uint8_t i = 0; i < 100000; i++) {
    name[3] = (i / 10000) % 10 + '0';
    name[4] = (i / 1000) % 10 + '0';
    name[5] = (i / 100) % 10 + '0';
    name[6] = (i / 10) % 10 + '0';
    name[7] = i % 10 + '0';
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
    bout << pstr(",date,time,t_adc,t_resistance,ext_temp_c,int_temp_c,pascals,meters,freefall,inactivity,activity,doubletap,tap,x,y,z,xG,yG,zG,mx,my,mz,heading");
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

//==================  loop  ====================//

void loop() 
{
  uint32_t m;
  uint32_t s;
  int x,y,z;

  //blinky
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  
  // use buffer stream to format line
  obufstream bout(buf, sizeof(buf));
  // different buffer for gps stream
  obufstream gout(guf, sizeof(guf));
  
  // wait for time to be a multiple of interval
  do {
    m = millis();
    s = m/1000;
  } while (m % LOG_INTERVAL);
  

  
  // start with time in secs
  bout << s;
  //gout << s;
  
  #if USE_DS1307
    DateTime now = rtc.now();
    bout << ',' << now;
  //  gout << ',' << now;
  #endif  //USE_DS1307

  // thermistor
  float t_reading;
  float t_resistance;
  float steinhart;
    
  t_reading = analogRead(THERMISTORPIN);
  t_resistance = (1023 / t_reading) - 1;
  t_resistance = SERIESRESISTOR / t_resistance;
  
  steinhart = t_resistance / THERMISTORNOMINAL;       // (R/Ro)
  steinhart = log(steinhart);                         // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                          // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);   // + (1/To)
  steinhart = 1.0 / steinhart;                        // Invert
  steinhart -= 273.15;
  
  bout << ',' << t_reading << ',' << t_resistance <<  ',' << steinhart;
  
  // bmp085
  dps.getTemperature(&dpstemp);
  dps.getPressure(&dpspress);
  dps.getAltitude(&dpsaltitude);
  float dpsTempC = dpstemp;
  float dpsAltM = dpsaltitude;
  dpsTempC = dpsTempC / 10;
  dpsAltM = dpsAltM / 100;
  
  bout << ',' << dpsTempC << ',' << dpspress << ',' << dpsAltM;
  
  // adxl345
  adxl.readAccel(&x, &y, &z);
  byte interrupts = adxl.getInterruptSource();
    //freefall
    if(adxl.triggered(interrupts, ADXL345_FREE_FALL))
      bout << ',' << '1';
    else bout << ',' << '0';
    
    //inactivity
    if(adxl.triggered(interrupts, ADXL345_INACTIVITY))
      bout << ',' << '1';
    else bout << ',' << '0';
 
    //activity
    if(adxl.triggered(interrupts, ADXL345_ACTIVITY))
      bout << ',' << '1';
    else bout << ',' << '0';

 
    //double tap
    if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP))
      bout << ',' << '1';
    else bout << ',' << '0';
 
    //tap
    if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP))
      bout << ',' << '1';
    else bout << ',' << '0';
    float xG,yG,zG;
    xG = x * 0.0039;
    yG = y * 0.0039;
    zG = z * 0.0039;
  bout << ',' << x << ',' << y << ',' << z << ',' << xG << ',' << yG << ',' << zG;
  
  // hmc5883l
  float fx, fy, fz;
  
  magn.getValues(&fx,&fy,&fz);
  float heading = atan2(fy, fx); 
  if(heading < 0) {
    heading += 2 * M_PI;
  }
  float modHeading = heading * 180/M_PI;
  bout << ',' << fx << ',' << fy << ',' << fz << ',' << modHeading;

  bout << endl;
  
  // gps
//  char c;
//  uint8_t sum;
//  uint8_t j = 0;
  
//while (Serial3.available()) {
//  c = Serial3.read();
//   while (c != '$') {
//      c = Serial3.read()
//      while (c !=;
//      j = 0;
//      gpsbuffer[j] = c;
//    }
//    while (j < 60) {
//      c = Serial3.read();
//      gpsbuffer[j+1] = c;
//      j++;
//    }
//  }
//  Serial.println(gpsbuffer);
  char charbuf[256];
  gpsbuffer2.toCharArray(charbuf, 256);
  gout << charbuf;

  //log data and flush to SD
  logfile.open(name1, ios::app);
  logfile << buf << flush;
  if (!logfile) error("write log data failed");
  logfile.close();
  
  logfile.open(name2, ios::app);
  logfile << guf << flush;
  if (!logfile) error("write gps data failed");
  logfile.close();

  #if ECHO_TO_SERIAL
    cout << buf;
    cout << guf << endl;
  #endif  //ECHO_TO_SERIAL
  
  #if ECHO_TO_XB
    xout << buf << endl;
    xout << guf << endl;
  #endif
  
  // dont log two points in the same millis
//  if (m == millis()) delay(1);
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    
    if (c) UDR0 = c;  
    if (c) gpsbuffer2 += c;
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
