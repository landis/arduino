/*
 * HMC.cpp - Interface a Honeywell HMC5843 magnetometer to an AVR via i2c
 * Version 0.1 - http://www.timzaman.com/
 * Copyright (c) 2011 Tim Zaman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "HMC.h"
#include "WProgram.h"
#include <Wire.h>

/* PUBLIC METHODS */

HMC5843::HMC5843()
{
}

void HMC5843::init()
{
}

// This can be called at 100ms intervals to get new data
void HMC5843::getValues(int *x, int *y, int *z)
{
	Wire.begin();       //Initiate the Wire library and join the I2C bus as a master
	int regb=0x01;
	int regbdata=0x40;
	int outputData[6];
	int i;
    double angle;

 
    Wire.beginTransmission(HMC5883_WriteAddress);
    Wire.send(regb);
    Wire.send(regbdata);
    Wire.endTransmission();
 
    delay(1000);
    Wire.beginTransmission(HMC5883_WriteAddress); //Initiate a transmission with HMC5883 (Write address).
    Wire.send(HMC5883_ModeRegisterAddress);       //Place the Mode Register Address in send-buffer.
    Wire.send(HMC5883_ContinuousModeCommand);     //Place the command for Continuous operation Mode in send-buffer.
    Wire.endTransmission();                       //Send the send-buffer to HMC5883 and end the I2C transmission.
    delay(100);
 
 
    Wire.beginTransmission(HMC5883_WriteAddress);  //Initiate a transmission with HMC5883 (Write address).
    Wire.requestFrom(HMC5883_WriteAddress,6);      //Request 6 bytes of data from the address specified.
 
    delay(500);
 
 
    //Read the value of magnetic components X,Y and Z
 
    if(6 <= Wire.available()) // If the number of bytes available for reading be <=6.
    {
        for(i=0;i<6;i++)
        {
            outputData[i]=Wire.receive();  //Store the data in outputData buffer
        }
    }
 
    *x=outputData[0] << 8 | outputData[1]; //Combine MSB and LSB of X Data output register
    *z=outputData[2] << 8 | outputData[3]; //Combine MSB and LSB of Z Data output register
    *y=outputData[4] << 8 | outputData[5]; //Combine MSB and LSB of Y Data output register
}

void HMC5843::getAngle(int *a)
{
	int fx,fy,fz;
	getValues(&fx,&fy,&fz);
	*a= atan((double)fy/(double)fx)*(360/PI); // angle in degrees
}



// Set the default object
HMC5843 HMC = HMC5843();










