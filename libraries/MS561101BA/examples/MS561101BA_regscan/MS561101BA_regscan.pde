/*HMC5883LRegisterScanner.pde
**A sketch that attempts to read every register from a slave device
**Written by Wayne Truchsess  http://dsscircuits.com
*/

#include "Wire.h"
#define I2C 0x77

byte x;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(1000);
}

void loop() {
  readRegisters();
}

void readRegisters() {
 for(int l = 0x00; l < 256; l++){
    Wire.beginTransmission(I2C);
    Wire.send(l);
    Wire.endTransmission();
    //delay(100);
    Wire.beginTransmission(I2C);
    Wire.requestFrom(I2C,1);
    x = Wire.receive();
    Serial.print("Register Address ");
    Serial.print(l,DEC);
    Serial.print("_");
    Serial.print(l,HEX);
    Serial.print(" = ");
    Serial.print(x,BIN);
    Serial.print(" = ");
    Serial.print(x,DEC);
    Serial.println("     ");
    Wire.endTransmission();
  }
}