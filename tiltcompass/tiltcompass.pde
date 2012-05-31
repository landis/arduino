// Reference the I2C Library
#include <Wire.h>
// Reference the HMC5883L Compass Library
#include <HMC5883L.h>
// Reference the ADXL345 Accelerometer Library
#include <ADXL345.h>

// Store our compass as a variable.
HMC5883L compass;
// Declare a global instance of the accelerometer.
ADXL345 accel;
// Set up a pin we are going to use to indicate our status using an LED.
int statusPin = 2; // I'm using digital pin 2.
int areConnected = 0; // Store our connection status here.

void setup()
{
  Serial.begin(9600); // Initialize the serial port.
  Wire.begin(); // Start the I2C interface.
  
  pinMode(statusPin, OUTPUT); // Ready an LED to indicate our status.

  compass = HMC5883L(); // Construct a new HMC5883 compass.
  accel = ADXL345(); // Construct a new ADXL345 accelerometer.
  
  compass.EnsureConnected();
  accel.EnsureConnected();
  if(compass.IsConnected && accel.IsConnected)
  {
    areConnected = true;
    Serial.println("Connected to HMC5883L and ADXL345.");
    digitalWrite(statusPin, HIGH);
  }
  else
  {
    areConnected = false;
    digitalWrite(statusPin, LOW);
    
    if(compass.IsConnected == 0)
      Serial.println("Could not connect to HMC5883L.");
    if(accel.IsConnected == 0)
      Serial.println("Could not connect to ADXL345.");
  }
  
  if(areConnected)
  { 
    compass.SetScale(1.3); // Set the scale of the compass.
    compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous

    accel.SetRange(2, true); // Set the range of the accelerometer to a maximum of 2G.
    accel.EnableMeasurements(); // Tell the accelerometer to start taking measurements.
  }
}

void loop()
{
  if(areConnected)
  {
    MagnetometerScaled magnetometerReadings = compass.ReadScaledAxis();
    AccelerometerScaled accelerometerReadings = accel.ReadScaledAxis();
    
    float headingNTC = CalculateHeadingNotTiltCompensated(magnetometerReadings);
    float headingTC = CalculateHeadingTiltCompensated(magnetometerReadings, accelerometerReadings);
    
    // Output the data via the serial port.
    Output(headingNTC, headingTC);
  }
}

float CalculateHeadingTiltCompensated(MagnetometerScaled mag, AccelerometerScaled acc)
{
  // We are swapping the accelerometers axis as they are opposite to the compass the way we have them mounted.
  // We are swapping the signs axis as they are opposite.
  // Configure this for your setup.
  float accX = -acc.YAxis;
  float accY = -acc.XAxis;
  
  float rollRadians = asin(accY);
  float pitchRadians = asin(accX);
  
  // We cannot correct for tilt over 40 degrees with this algorthem, if the board is tilted as such, return 0.
  if(rollRadians > 0.78 || rollRadians < -0.78 || pitchRadians > 0.78 || pitchRadians < -0.78)
  {
    return 0;
  }
  
  // Some of these are used twice, so rather than computing them twice in the algorithem we precompute them before hand.
  float cosRoll = cos(rollRadians);
  float sinRoll = sin(rollRadians);  
  float cosPitch = cos(pitchRadians);
  float sinPitch = sin(pitchRadians);
  
  float Xh = mag.XAxis * cosPitch + mag.ZAxis * sinPitch;
  float Yh = mag.XAxis * sinRoll * sinPitch + mag.YAxis * cosRoll - mag.ZAxis * sinRoll * cosPitch;
  
  float heading = atan2(Yh, Xh);
    
  return heading;
}

float CalculateHeadingNotTiltCompensated(MagnetometerScaled mag)
{
   // Calculate heading when the magnetometer is level, then correct for signs of axis.
   float heading = atan2(mag.YAxis, mag.XAxis);
   return heading;
}

float RadiansToDegrees(float rads)
{
  // Correct for when signs are reversed.
  if(rads < 0)
    rads += 2*PI;
      
  // Check for wrap due to addition of declination.
  if(rads > 2*PI)
    rads -= 2*PI;
   
  // Convert radians to degrees for readability.
  float heading = rads * 180/PI;
       
  return heading;
}

void Output(float headingNTC, float headingTC)
{
  Serial.print("Heading (Not Compensated):\t");
  Serial.print(RadiansToDegrees(headingNTC));
  Serial.print("\tHeading (Tilt Compensated):\t");   
  Serial.println(RadiansToDegrees(headingTC));
}
