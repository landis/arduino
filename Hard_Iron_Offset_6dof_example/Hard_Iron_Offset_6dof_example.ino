/************************************************************************************
 * 	
 * 	Name    : Comp6DOF_n0m1 Library Example: Hard Iron Offset                       
 * 	Author  : Noah Shibley, Michael Grant, NoMi Design Ltd. http://n0m1.com                       
 * 	Date    : Feb 27th 2012                                    
 * 	Version : 0.1                                              
 * 	Notes   : Arduino Library for compass tilt compensation and hard iron offset 
 *
 ***********************************************************************************/

#include <Wire.h>
#include <bildr_ADXL345.h>
#include <Comp6DOF_n0m1.h>
#include <HMC5883L.h> // Reference the HMC5883L Compass Library

ADXL345 accel;
Comp6DOF_n0m1 sixDOF;
HMC5883L compass;

// Record any errors that may occur in the compass.
int error = 0;
float gauss = 1.3;

void setup()
{
  Serial.begin(9600);
  accel.setRangeSetting(2); //enable highRes 10bit, 2g range [2g,4g,8g]

  compass = HMC5883L(); // Construct a new HMC5883 compass.

//  compass.SetScale(gauss);

  error = compass.SetScale(1.3); // Set the scale of the compass.
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));

//  compass.SetMeasurementMode(Measurement_Continuous);

  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));

  Serial.println("n0m1.com");

  //Hard iron offset calculations 
  int doneoffset =0;
  while (doneoffset ==0)
  {
    // a blink would be nice right here
    int donecombo = 0;
    while ( donecombo == 0 )   // load tuning array 
    {  
      delay (50);
      MagnetometerRaw  raw = compass.ReadRawAxis();
      donecombo = sixDOF.deviantSpread (raw.XAxis, raw.YAxis, raw.ZAxis);
    }

    doneoffset = sixDOF.calOffsets();
  }

  // print offsets
  Serial.println ("");
  Serial.print ("  Xoff: ");
  Serial.print (sixDOF.xHardOff());   
  Serial.print ("  Yoff: ");
  Serial.print (sixDOF.yHardOff());   
  Serial.print ("  Zoff: ");
  Serial.print (sixDOF.zHardOff());   
  Serial.println ("");

}

void loop()
{

  // spin wheels, do nothing, hard iron offset done in setup
  delay (5);

}




