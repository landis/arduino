/* Project SAPHE */

#define ENABLE_SD

#if defined(ENABLE_SD)
  #include <SD.h>
  const int sdCS = 8;
  const int sdELED = 6;
  char filename[] = "LOG00.csv";
  File logFile;
#endif

void setup()
{
  Serial.begin(115200);
  
  #if defined(ENABLE_SD)
    pinMode(sdCS, OUTPUT);
    pinMode(sdELED, OUTPUT);
    if (!SD.begin(sdCS)) {
      Serial.println("Card failed, or not present");
      digitalWrite(sdELED, HIGH);
      return;
    }
    else {
      Serial.println("Card is initialized");
      return;
    }
  #endif
}

void loop()
{
  
}
