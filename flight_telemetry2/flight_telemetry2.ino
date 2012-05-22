
/*** Configuration ***/
#define ENABLE_RTC
#define ENABLE_SERIAL


//ENABLE_RTC | Chronodot ***?
#ifdef ENABLE_RTC
  #include <Wire.h>
  #include <RTClib.h>
  RTC_DS1307 RTC;
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
}

void loop() {
  #ifdef ENABLE_RTC
    DateTime now = RTC.now();
    #ifdef ENABLE_SERIAL
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
      delay(1000);
    #endif
  #endif
}
