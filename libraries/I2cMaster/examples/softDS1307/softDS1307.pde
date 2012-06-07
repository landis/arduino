// Utility sketch to explore DS1307 and
// demonstrate SoftI2cMaster and TwiMaster
//
#include <avr/pgmspace.h>
#include <I2cMaster.h>

// select software or hardware i2c
#define USE_SOFT_I2C 1

#if USE_SOFT_I2C

#if defined(__AVR_ATmega1280__)\
|| defined(__AVR_ATmega2560__)
// Mega analog pins 4 and 5
// pins for DS1307 with software i2c on Mega
#define SDA_PIN 58
#define SCL_PIN 59

#elif defined(__AVR_ATmega168__)\
||defined(__AVR_ATmega168P__)\
||defined(__AVR_ATmega328P__)
// 168 and 328 Arduinos analog pin 4 and 5
#define SDA_PIN 18
#define SCL_PIN 19

#else  // CPU type
#error unknown CPU
#endif  // CPU type

// An instance of class for software master
SoftI2cMaster rtc(SDA_PIN, SCL_PIN);

#else // USE_SOFT_I2C

// Pins for DS1307 with hardware i2c
// connect SCL to Arduino 168/328 analog pin 5
// connect SDA to Arduino 168/328 analog pin 4

// Instance of class for hardware master with pullups enabled
TwiMaster rtc(true);

#endif  // USE_SOFT_I2C

// i2c 8-bit address for DS1307. low bit is read/write
#define DS1307ADDR 0XD0
//------------------------------------------------------------------------------
/*
 * Read 'count' bytes from the DS1307 starting at 'address'
 */
uint8_t readDS1307(uint8_t address, uint8_t *buf, uint8_t count) {
  // issue a start condition, send device address and write direction bit
  if (!rtc.start(DS1307ADDR | I2C_WRITE)) return false;

  // send the DS1307 address
  if (!rtc.write(address)) return false;

  // issue a repeated start condition, send device address and read direction bit
  if (!rtc.restart(DS1307ADDR | I2C_READ))return false;

  // read data from the DS1307
  for (uint8_t i = 0; i < count; i++) {

    // send Ack until last byte then send Ack
    buf[i] = rtc.read(i == (count-1));
  }

  // issue a stop condition
  rtc.stop();
  return true;
}
//------------------------------------------------------------------------------
/*
 * write 'count' bytes to DS1307 starting at 'address'
 */
uint8_t writeDS1307(uint8_t address, uint8_t *buf, uint8_t count) {
  // issue a start condition, send device address and write direction bit
  if (!rtc.start(DS1307ADDR | I2C_WRITE)) return false;

  // send the DS1307 address
  if (!rtc.write(address)) return false;

  // send data to the DS1307
  for (uint8_t i = 0; i < count; i++) {
    if (!rtc.write(buf[i])) return false;
  }

  // issue a stop condition
  rtc.stop();
  return true;
}
//------------------------------------------------------------------------------
void setup(void) {
  Serial.begin(9600);
}
//------------------------------------------------------------------------------
/** Store and print a string in flash memory.*/
#define PgmPrint(x) SerialPrint_P(PSTR(x))
/** Store and print a string in flash memory followed by a CR/LF.*/
#define PgmPrintln(x) SerialPrintln_P(PSTR(x))
//------------------------------------------------------------------------------
/*
 * Print a string in flash memory to the serial port.
 */
static void SerialPrint_P(PGM_P str) {
  for (uint8_t c; (c = pgm_read_byte(str)); str++) Serial.write(c);
}
//------------------------------------------------------------------------------
/*
 * Print a string in flash memory followed by a CR/LF.
 */
static  void SerialPrintln_P(PGM_P str) {
  SerialPrint_P(str);
  Serial.println();
}
//-------------------------------------------------------------------------------
// day of week U.S. convention
char *Ddd[] = {"Bad", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//------------------------------------------------------------------------------
void hexPrint(uint8_t v) {
  Serial.print(v >> 4, HEX);
  Serial.print(v & 0XF, HEX);
}
//------------------------------------------------------------------------------
void hexPrintln(uint8_t v) {
  hexPrint(v);
  Serial.println();
}
//------------------------------------------------------------------------------
// read hex input
uint8_t hexRead(uint16_t &v) {
  uint16_t n = 0;
  while (!Serial.available());
  while (Serial.available()) {
    uint8_t c = Serial.read();
    n <<= 4;
    if ('a' <= c && c <= 'f') {
      n += c - ('a' - 10);
    }
    else if ('A' <= c && c <= 'F') {
      n += c - ('A' - 10);
    }
    else if ('0' <= c && c <= '9') {
      n +=  c - '0';
    }
    else {
      PgmPrintln("Invalid entry");
      return false;
    }
    delay(10);
  }
  v = n;
  return true;
}
//------------------------------------------------------------------------------
uint8_t bcdRead(uint8_t min, uint8_t max, uint8_t &n) {
  uint16_t v;
  if (!hexRead(v)) return false;
  uint8_t d = 10 * (v >> 4) + (v & 0XF);
  if ((v >> 4) > 9 || (v & 0XF) > 9 || d < min || d > max) {
    PgmPrintln("Invalid");
    return false;
  }
  n = v;
  return true;
}
//------------------------------------------------------------------------------
void displayTime(void) {
  uint8_t r[8];
  if (!readDS1307(0, r, 8)) {
    PgmPrintln("Read Failed for display time");
    return;
  }
  PgmPrint("The current time is ");
  // month
  hexPrint(r[5]);
  Serial.write('/');
  // day
  hexPrint(r[4]);
  PgmPrint("/20");
  // year
  hexPrint(r[6]);
  Serial.write(' ');
  Serial.print(Ddd[r[3] < 8 ? r[3] : 0]);
  Serial.write(' ');
  // hour
  hexPrint(r[2]);
  Serial.write(':');
  // minute
  hexPrint(r[1]);
  Serial.write(':');
  // second
  hexPrintln(r[0]);
  PgmPrint("Control: ");
  hexPrintln(r[7]);
}
//------------------------------------------------------------------------------
// dump registers and 56 bytes of RAM
void dumpAll(void) {
  uint8_t buf[8];
  for (uint8_t a = 0; a < 64; a += 8) {
    hexPrint(a);
    Serial.write(' ');
    if (!readDS1307(a, buf, 8)) {
      PgmPrint("read failed for dumpAll");
      return;
    }
    for (uint8_t i = 0; i < 8; i++) {
      Serial.write(' ');
      hexPrint(buf[i]);
    }
    Serial.println();
  }
}
//------------------------------------------------------------------------------
// set control register
/*
The DS1307 control register is used to control the operation of the SQW/OUT pin.
+-----------------------------------------------+
|BIT 7|BIT 6|BIT 5|BIT 4|BIT 3|BIT 2|BIT 1|BIT 0|
+-----------------------------------------------+
|OUT  |  0  |  0  |SQWE |  0  |  0  | RS1 | RS0 |
+-----------------------------------------------+

OUT (Output control): This bit controls the output level of the SQW/OUT pin
when the square wave output is disabled. If SQWE = 0, the logic level on the
SQW/OUT pin is 1 if OUT = 1 and is 0 if OUT = 0.

SQWE (Square Wave Enable): This bit, when set to a logic 1, will enable the
oscillator output. The frequency of the square wave output depends upon the
value of the RS0 and RS1 bits. With the square wave output set to 1Hz, the
clock registers update on the falling edge of the square wave.

Square wave Output Frequency for SQWE = 1.
RS1 RS0 FREQUENCY
 0   0  1Hz
 0   1  4.096kHz
 1   0  8.192kHz
 1   1  32.768kHz
*/
void setControl(void) {
  PgmPrintln("SQW/OUT pin: ");
  PgmPrintln("(00) Low");
  PgmPrintln("(10) 1Hz");
  PgmPrintln("(11) 4.096kHz");
  PgmPrintln("(12) 8.192kHz");
  PgmPrintln("(13) 32.768kHz");
  PgmPrintln("(80) High");
  PgmPrint("Enter control: ");
  uint16_t r;
  if (!hexRead(r)) return;
  hexPrintln(r);
  if (!writeDS1307(7, (uint8_t *)&r, 1)) {
    PgmPrint("Write Failed for setControl");
  }
}
//------------------------------------------------------------------------------
void setDate(void) {
  uint8_t r[4];
  PgmPrint("Enter year (00-99): ");
  if (!bcdRead(0, 99, r[3])) return;
  hexPrintln(r[3]);
  PgmPrint("Enter month (01-12): ");
  if (!bcdRead(1, 12, r[2])) return;
  hexPrintln(r[2]);
  PgmPrint("Enter date (01-31): ");
  if (!bcdRead(1, 31, r[1])) return;
  hexPrintln(r[1]);
  PgmPrint("Enter day of week (01-07): ");
  if (!bcdRead(1, 7, r[0])) return;
  hexPrintln(r[0]);

  if (!writeDS1307(3, r, 4)) {
    PgmPrintln("Write failed for setDate");
  }
}
//------------------------------------------------------------------------------
void setNvRam(void) {
  uint8_t buf[8];
  PgmPrint("Enter HEX value for all NV RAM locations (00-FF): ");
  uint16_t v;
  if (!hexRead(v)) return;
  hexPrint(v);
  for (uint8_t a = 8; a < 64; a ++) {
    if (!writeDS1307(a, (uint8_t *)&v, 1)) {
      PgmPrintln("write failed for setNvRam");
    }
  }
}
//------------------------------------------------------------------------------
void setTime(void) {
  uint8_t r[3];
  PgmPrint("Enter hours (00-23): ");
  if (!bcdRead(0, 23, r[2])) return;
  hexPrintln(r[2]);
  PgmPrint("Enter minutes (00-59): ");
  if (!bcdRead(0, 59, r[1])) return;
  hexPrintln(r[1]);
  PgmPrint("Enter seconds (00-59): ");
  if (!bcdRead(0, 59, r[0])) return;
  hexPrintln(r[0]);

  if (!writeDS1307(0, r, 3)) {
    PgmPrintln("write failed in setTime");
    return;
  }
}
//------------------------------------------------------------------------------
void loop(void) {
  Serial.println();
  displayTime();
  PgmPrintln("\nOptions are:");
  PgmPrintln("(0) Display date and time");
  PgmPrintln("(1) Set time");
  PgmPrintln("(2) Set date");
  PgmPrintln("(3) Set Control");
  PgmPrintln("(4) Dump all");
  PgmPrintln("(5) Set NV RAM");
  PgmPrint("Enter option: ");

  uint16_t n;
  if (!hexRead(n)) n = 99;
  Serial.println(n, DEC);
  if (n == 0) return;
  Serial.println();
  if (n == 1) {
    setTime();
  }
  else if (n == 2) {
    setDate();
  }
  else if (n == 3) {
    setControl();
  }
  else if (n == 4) {
    dumpAll();
  }
  else if (n == 5) {
    setNvRam();
  }
  else {
    PgmPrintln("Invalid option");
  }
}
