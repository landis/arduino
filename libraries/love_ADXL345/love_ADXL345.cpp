/*
ADXL345.cpp - Header file for the ADXL345 Triple Axis Accelerometer Arduino Library.
Copyright (C) 2011 Love Electronics (loveelectronics.co.uk)

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

 Datasheet for ADXL345:
 http://www.analog.com/static/imported-files/data_sheets/ADXL345.pdf

*/

#include <WProgram.h>
#include "love_ADXL345.h"

//#define SerialDebug

ADXL345::ADXL345()
{
  m_Address = DefaultADXL345_Address;
}

ADXL345::ADXL345(uint8_t customAddress)
{
	m_Address = customAddress;
}

AccelerometerRaw ADXL345::ReadRawAxis()
{
#ifdef SerialDebug
	Serial.println("Reading raw axis.");
#endif

	uint8_t* buffer = Read(Register_DataX, 6);
	AccelerometerRaw raw = AccelerometerRaw();
	raw.XAxis = (buffer[1] << 8) | buffer[0];
	raw.YAxis = (buffer[3] << 8) | buffer[2];
	raw.ZAxis = (buffer[5] << 8) | buffer[4];
	return raw;
}

AccelerometerScaled ADXL345::ReadScaledAxis()
{
	AccelerometerRaw raw = ReadRawAxis();
	AccelerometerScaled scaled = AccelerometerScaled();
	scaled.XAxis = raw.XAxis * m_Scale;
	scaled.YAxis = raw.YAxis * m_Scale;
	scaled.ZAxis = raw.ZAxis * m_Scale;
	return scaled;
}

int ADXL345::EnableMeasurements()
{
#ifdef SerialDebug
	Serial.println("Enabling measurements.");
#endif

	Write(Register_PowerControl, 0x08);
}

int ADXL345::SetRange(int range, bool fullResolution)
{
#ifdef SerialDebug
	Serial.print("Setting range to: ");
	Serial.println(range);
#endif

	// Get current data from this register.
	uint8_t data = Read(Register_DataFormat, 1)[0];

	// We AND with 0xF4 to clear the bits are going to set.
	// Clearing ----X-XX
	data &= 0xF4;

	// By default (range 2) or FullResolution = true, scale is 2G.
	m_Scale = ScaleFor2G;

	// Set the range bits.
	switch(range)
	{
		case 2:
			break;
		case 4:
			data |= 0x01;
			if(!fullResolution) { m_Scale = ScaleFor4G; }
			break;
		case 8:
			data |= 0x02;
			if(!fullResolution) { m_Scale = ScaleFor8G; }
			break;
		case 16:
			data |= 0x03;
			if(!fullResolution) { m_Scale = ScaleFor16G; }
			break;
		default:
			return ErrorCode_1_Num;
	}

	// Set the full resolution bit.
	if(fullResolution)
		data |= 0x08;

	Write(Register_DataFormat, data);
}

uint8_t ADXL345::EnsureConnected()
{
	uint8_t data = Read(0x00, 1)[0];

	if(data == 0xE5)
		IsConnected = true;
	else
		IsConnected = false;

	return IsConnected;
}

void ADXL345::Write(int address, int data)
{
#ifdef SerialDebug
	Serial.print("Writing ");
	Serial.print(data, HEX);
	Serial.print(" to register ");
	Serial.println(address, HEX);
#endif

	Wire.beginTransmission(m_Address);
	Wire.write(address);
	Wire.write(data);
	Wire.endTransmission();
}

uint8_t* ADXL345::Read(int address, int length)
{
	Wire.beginTransmission(m_Address);
	Wire.write(address);
	Wire.endTransmission();

	Wire.beginTransmission(m_Address);
	Wire.requestFrom(m_Address, length);

	uint8_t buffer[length];
	if(Wire.available() == length)
	{
		for(uint8_t i = 0; i < length; i++)
		{
			buffer[i] = Wire.read();
		}
	}
	Wire.endTransmission();

	return buffer;
}

char* ADXL345::GetErrorText(int errorCode)
{
	if(ErrorCode_1_Num == 1)
		return ErrorCode_1;

	return "Error not defined.";
}