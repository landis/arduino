// Warning only use this for hardware debug!
// See which addresses respond to a start condition.

#include <I2cMaster.h>

// select software or hardware i2c
#define USE_SOFT_I2C 0

#if USE_SOFT_I2C
#if defined(__AVR_ATmega1280__)\
|| defined(__AVR_ATmega2560__)
// Mega analog pins 4 and 5
// pins for 168/328 shield on Mega
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
// hardware master with pullups enabled
TwiMaster rtc(true);
#endif  // USE_SOFT_I2C

//------------------------------------------------------------------------------
void setup(void) {
  Serial.begin(9600);

  uint8_t add = 0;

  // try read
  do {
    if (rtc.start(add | I2C_READ)) {
      Serial.print("Add read: ");
      Serial.println(add, HEX);
      rtc.read(true);
    }
    rtc.stop();
    add += 2;
  } while (add);

  // try write
  add = 0;
  do {
    if (rtc.start(add | I2C_WRITE)) {
      Serial.print("Add write: ");
      Serial.println(add, HEX);
    }
    rtc.stop();
    add += 2;
  } while (add);

  Serial.println("Done");
}
void loop(void){}
