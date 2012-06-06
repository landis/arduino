#include <Wire.h>
#include <HMC.h>
int a;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  int x,y,z;
  delay(100); // There will be new values every 100ms
  HMC.getValues(&x,&y,&z);
  Serial.print("x:");
  Serial.print(x);
  Serial.print(" y:");
  Serial.print(y);
  Serial.print(" z:");
  Serial.println(z);
  
  HMC.getAngle(&a);
  Serial.println(a);
}
