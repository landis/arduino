/*
MS5611-01BA.cpp - Interfaces a Measurement Specialities MS5611-01BA with Arduino
See http://www.meas-spec.com/downloads/MS5611-01BA01.pdf for the device datasheet

Copyright (C) 2011 Fabio Varesano <fvaresano@yahoo.it>

Development of this code has been supported by the Department of Computer Science,
Universita' degli Studi di Torino, Italy within the Piemonte Project
http://www.piemonte.di.unito.it/


This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "MS561101BA.h"
#define EXTRA_PRECISION 5 // trick to add more precision to the pressure and temp readings
#define CONVERSION_TIME 10000l // conversion time in microseconds

MS561101BA::MS561101BA() {
  ;
}

void MS561101BA::init(uint8_t address) {  
  _addr =  address;
  
  // disable internal pullups of the ATMEGA which Wire enable by default
  #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
    // deactivate internal pull-ups for twi
    // as per note from atmega8 manual pg167
    cbi(PORTC, 4);
    cbi(PORTC, 5);
  #else
    // deactivate internal pull-ups for twi
    // as per note from atmega128 manual pg204
    cbi(PORTD, 0);
    cbi(PORTD, 1);
  #endif
 
  reset(); // reset the device to populate its internal PROM registers
  delay(1000); // some safety time 
  readPROM(); // reads the PROM into object variables for later use
}

float MS561101BA::getPressure(uint8_t OSR) {
  // see datasheet page 7 for formulas
  int32_t rawPress = rawPressure(OSR);
  int64_t dT   = getDeltaTemp(OSR);
  if(dT == NULL) {
    return NULL;
  }
  int64_t off  = (((int64_t)_C[1]) << 16) + ((_C[3] * dT) >> 7);
  int64_t sens = (((int64_t)_C[0]) << 15) + ((_C[2] * dT) >> 8);
  if(rawPress != NULL) {
    return ((((rawPress * sens) >> 21) - off) >> (15-EXTRA_PRECISION)) / ((1<<EXTRA_PRECISION) * 100.0);
  }
  else {
    return NULL;
  }
}

float MS561101BA::getTemperature(uint8_t OSR) {
  // see datasheet page 7 for formulas
  int64_t dT = getDeltaTemp(OSR);
  
  if(dT != NULL) {
    return ((1<<EXTRA_PRECISION)*2000l + ((dT * _C[5]) >> (23-EXTRA_PRECISION))) / ((1<<EXTRA_PRECISION) * 100.0);
  }
  else {
    return NULL;
  }
}

int64_t MS561101BA::getDeltaTemp(uint8_t OSR) {
  int32_t rawTemp = rawTemperature(OSR);
  if(rawTemp != NULL) {
    return rawTemp - (((int32_t)_C[4]) << 8);
  }
  else {
    return NULL;
  }
}

int32_t MS561101BA::rawPressure(uint8_t OSR) {
  unsigned long now = micros();
  if(lastPresConv != 0 && (now - lastPresConv) >= CONVERSION_TIME) {
    lastPresConv = 0;
    return getConversion(MS561101BA_D1 + OSR);
  }
  else {
    if(lastPresConv == 0 && lastTempConv == 0) {
      startConversion(MS561101BA_D1 + OSR);
      lastPresConv = now;
    }
    return NULL;
  }
}

int32_t MS561101BA::rawTemperature(uint8_t OSR) {
  unsigned long now = micros();
  if(lastTempConv != 0 && (now - lastTempConv) >= CONVERSION_TIME) {
    lastTempConv = 0;
    tempCache = getConversion(MS561101BA_D2 + OSR);
    return tempCache;
  }
  else {
    if(lastTempConv == 0 && lastPresConv == 0) {
      startConversion(MS561101BA_D2 + OSR);
      lastTempConv = now;
    }
    else if(lastPresConv != 0) { // there is a Pressure reading in process
      return tempCache;
    }
    else {
      return NULL;
    }
  }
}


// see page 11 of the datasheet
void MS561101BA::startConversion(uint8_t command) {
  // initialize pressure conversion
  Wire.beginTransmission(_addr);
  Wire.write(command);
  Wire.endTransmission();
}

unsigned long MS561101BA::getConversion(uint8_t command) {
  unsigned long conversion = 0;
  
  // start read sequence
  Wire.beginTransmission(_addr);
  Wire.write(0);
  Wire.endTransmission();
  
  Wire.beginTransmission(_addr);
  Wire.requestFrom(_addr, (uint8_t) MS561101BA_D1D2_SIZE);
  if(Wire.available()) {
    conversion = Wire.read() * 65536 + Wire.read() * 256 + Wire.read();
  }
  else {
    conversion = -1;
  }
  
  return conversion;
}


/**
 * Reads factory calibration and store it into object variables.
*/
int MS561101BA::readPROM() {
  for (int i=0;i<MS561101BA_PROM_REG_COUNT;i++) {
    Wire.beginTransmission(_addr);
    Wire.write(MS561101BA_PROM_BASE_ADDR + (i * MS561101BA_PROM_REG_SIZE));
    Wire.endTransmission();
    
    Wire.beginTransmission(_addr);
    Wire.requestFrom(_addr, (uint8_t) MS561101BA_PROM_REG_SIZE);
    if(Wire.available()) {
      _C[i] = Wire.read() << 8 | Wire.read();
      
      //DEBUG_PRINT(_C[i]);
    }
    else {
      return -1; // error reading the PROM or communicating with the device
    }
  }
  return 0;
}


/**
 * Send a reset command to the device. With the reset command the device
 * populates its internal registers with the values read from the PROM.
*/
void MS561101BA::reset() {
  Wire.beginTransmission(_addr);
  Wire.write(MS561101BA_RESET);
  Wire.endTransmission();
}
