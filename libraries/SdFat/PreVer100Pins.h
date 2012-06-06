/* Arduino SdFat Library
 * Copyright (C) 2012 by William Greiman
 *
 * This file is part of the Arduino SdFat Library
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
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef PreVer100Pins_h
#define PreVer100Pins_h
#include <avr/io.h>
//------------------------------------------------------------------------------
#if defined(__AVR_ATmega1280__)\
|| defined(__AVR_ATmega2560__)
// Mega
uint8_t const SS = 53;    // B0
uint8_t const MOSI = 51;  // B2
uint8_t const MISO = 50;  // B3
uint8_t const SCK = 52;   // B1
//------------------------------------------------------------------------------
#elif defined(__AVR_ATmega644P__)\
|| defined(__AVR_ATmega644__)\
|| defined(__AVR_ATmega1284P__)
// Sanguino
uint8_t const SS = 4;    // B4
uint8_t const MOSI = 5;  // B5
uint8_t const MISO = 6;  // B6
uint8_t const SCK = 7;   // B7
//------------------------------------------------------------------------------
#elif defined(__AVR_ATmega32U4__)
// Teensy 2.0
uint8_t const SS = 0;    // B0
uint8_t const MOSI = 2;  // B2
uint8_t const MISO = 3;  // B3
uint8_t const SCK = 1;   // B1
//------------------------------------------------------------------------------
#elif defined(__AVR_AT90USB646__)\
|| defined(__AVR_AT90USB1286__)
// Teensy++ 1.0 & 2.0
uint8_t const SS = 20;    // B0
uint8_t const MOSI = 22;  // B2
uint8_t const MISO = 23;  // B3
uint8_t const SCK = 21;   // B1
//------------------------------------------------------------------------------
#elif defined(__AVR_ATmega168__)\
||defined(__AVR_ATmega168P__)\
||defined(__AVR_ATmega328P__)
// 168 and 328 Arduinos
uint8_t const SS = 10;    // B2
uint8_t const MOSI = 11;  // B3
uint8_t const MISO = 12;  // B4
uint8_t const SCK = 13;   // B5
//------------------------------------------------------------------------------
#else  // defined(__AVR_ATmega1280__)
#error unknown chip
#endif  // defined(__AVR_ATmega1280__)
#endif  // PreVer100Pins_h
