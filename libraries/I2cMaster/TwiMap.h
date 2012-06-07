/* Arduino I2cMaster Library
 * Copyright (C) 2010 by William Greiman
 *
 * This file is part of the Arduino I2cMaster Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino I2cMaster Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * \file
 * \brief I2C pin definitions
 */
#ifndef TwiMap_h
#define TwiMap_h

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
// 168 and 328 Arduinos

uint8_t const TWI_SDA_PIN = 18;
uint8_t const TWI_SCL_PIN = 19;
//------------------------------------------------------------------------------
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
// Mega 1280 and 2560

uint8_t const TWI_SDA_PIN = 20;
uint8_t const TWI_SCL_PIN = 21;
//------------------------------------------------------------------------------
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)
// Sanguino

uint8_t const TWI_SDA_PIN = 17;
uint8_t const TWI_SCL_PIN = 18;
//------------------------------------------------------------------------------
#elif defined(__AVR_ATmega32U4__)
// Teensy 2.0

uint8_t const TWI_SDA_PIN = 6;
uint8_t const TWI_SCL_PIN = 5;
//------------------------------------------------------------------------------
#elif defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
// Teensy++ 1.0 & 2.0

uint8_t const TWI_SDA_PIN = 1;
uint8_t const TWI_SCL_PIN = 0;
//------------------------------------------------------------------------------
#else  // AVR CPU
#error unknown CPU
#endif  // AVR CPU
#endif  // TwiMap_h
