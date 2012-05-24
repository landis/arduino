/*
 * "Katie" 
 * Arduino program controlling a stepper motor and a laser
 * as an interface between the DAVID-Laserscanner software and 
 * a motor-driven laser for automated 3D laser scanning.
 * It supports additional switches and a LC display for manual control.
 * Some functions are only supported by DAVID 3.x.
 * 
 * Copyright (C) 2011 Sven Molkenstruck
 * 
 * This program is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the 
 * Free Software Foundation; either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * 
 * You can find the GNU General Public License here: http://www.gnu.org/licenses/
 * 
 */


// Requires Arduino version 0020 or later!

#include <LiquidCrystal.h>


const char PROGRAM_VERSION[20] = "Katie Version 1.1";


//////////////////////////////////
// GLOBAL PARAMETER DEFINITIONS //
//////////////////////////////////
// do not use laser and motor switches, instead force "remote control" by DAVID:
const bool FORCE_REMOTE_CONTROL = false;  // set "true" if you don't have any laser and motor switches

// analog input pins:
const int PIN_IN_SPEED   = A0;
const int PIN_IN_LASER   = A1;
const int PIN_IN_MOTOR   = A2;
const int PIN_IN_DIR     = A3;
const int PIN_IN_GO_ZERO = A4;

// digital output pins:
const int PIN_OUT_CLOCK  = 2;
const int PIN_OUT_DIR    = 3;
const int PIN_OUT_LASER  = 4;
const int PIN_OUT_LCD    = 8;  // and 5 more directly following (8-13)

// digital input pins:
const int PIN_IN_SET_ZERO = 5;

// speed steps and their delays [µs]:
const int NUM_SPEED_STEPS=6;
const unsigned long DELAYS[NUM_SPEED_STEPS] = {50000,20000,10000,5000,2000,1000};  // in µs
const unsigned long HOMING_MOTOR_DELAY = 1000;  // defines the speed for the "return" = "go home" = "go to zero" command [µs]

// Gearbox backlash compensation:
const long BACKLASH_STEPS = 500;  // number of steps

const long MIDDLE_POS = 2000;  // counter position where the laser is "in the middle" of the object / camera image
const long NUM_STEPS_FAST = 100;  // number of steps to go when receiving command "<" or ">"

// Time interval [ms] for display updates:
const unsigned long DISPLAY_INTERVAL = 200;  // 200ms --> 5 Hz


//////////////////////////////////
// GLOBAL VARIABLE DECLARATIONS //
//////////////////////////////////
long g_motorPos = 0;               ///< global motor step counter
int g_speed = 0;                   ///< global speed setting in [0,NUM_SPEED_STEPS-1]
bool g_isLaserOn = false;          ///< global state of laser (on or off)
int g_lastCOM = ' ';               ///< latest COM message
long g_dir = -1;                   ///< scan direction (1 or -1)

LiquidCrystal g_lcd(PIN_OUT_LCD, PIN_OUT_LCD+1, PIN_OUT_LCD+2, PIN_OUT_LCD+3, PIN_OUT_LCD+4, PIN_OUT_LCD+5);  // initialize the library with the numbers of the interface pins


///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// HELPERS /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


/// modes for laser and motor
enum eMode
{
  MODE_ON   = 0,
  MODE_AUTO = 1,  // "auto" means remote controlled, e.g. via serial COM port
  MODE_OFF  = 2,
};


/// Maps an "analog" input value to a descrete step value.
/// @param value The input value.
/// @param num The number of steps to convert to.
/// @param mod the maximum possible input value+1; assuming 0<=value<mod.
/// @return Discrete step value in [0,num-1]
long analogToDiscrete(long value, long num, long mod=1024)
{
  return (value*num)/mod;
  // Should also work, but not so nicely: return map(value,0,mod-1,0,num);
}

/// Maps an "analog" input value into one of the eModes MODE_OFF, MODE_AUTO, or MODE_ON. See #analogToDescrete and #eMode.
eMode analogToMode(long value, long mod=1024)
{
  return (eMode)analogToDiscrete(value,3,mod);
}


/// Delays until a given time span has passed SINCE THE LAST CALL. 
/// So when e.g. you want a delay of 1000µs between two actions, but some other code has consumed 200µs already, call this function with delay_us=1000 and it will delay only 800µs.
/// Uses lastTime to store the time stamp of the call. So you must call this function repeatedly with reference to the same lastTime variable. 
/// @param delay_us How many µs to delay (since the last call).
/// @param lastTime Reference to a (static) variable that is used to remember the time stamp of the previous call.
void delaySinceLastCall(unsigned long delay_us, unsigned long &lastTime)
{
  unsigned long m;
  
  do
  {
    m=micros();  // get current time stamp
    if (m<lastTime) lastTime=m;  // reset when micros() has flown over (happens every 2^32 µs = 71.6 minutes)
  }
  while (m-lastTime <= delay_us);  // loop until delay_us µs have passed
  
  lastTime = m;  // remember this time stamp for next time
}


///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// SETUP //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

/// The setup() method runs once, when the sketch starts.
void setup()
{
  // Init LC Display:
  g_lcd.begin(20, 2);

  // Init serial port:
  Serial.begin(9600);
  
  // Init input pins:
  pinMode(PIN_IN_SPEED,    INPUT);
  pinMode(PIN_IN_LASER,    INPUT);
  pinMode(PIN_IN_MOTOR,    INPUT);
  pinMode(PIN_IN_DIR,      INPUT);
  pinMode(PIN_IN_GO_ZERO,  INPUT);
  pinMode(PIN_IN_SET_ZERO, INPUT);

  // Pull inputs to HIGH by a pull-up resistor, in case they are left "open":
  digitalWrite(PIN_IN_SPEED, HIGH);
  digitalWrite(PIN_IN_LASER, HIGH);
  digitalWrite(PIN_IN_MOTOR, HIGH);
  digitalWrite(PIN_IN_DIR, HIGH);
  digitalWrite(PIN_IN_GO_ZERO, HIGH);
  digitalWrite(PIN_IN_SET_ZERO, HIGH);
  
  // Init output pins:
  pinMode(PIN_OUT_CLOCK, OUTPUT);
  pinMode(PIN_OUT_DIR,   OUTPUT);
  pinMode(PIN_OUT_LASER, OUTPUT);
  
  // Show welcome message:
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print(PROGRAM_VERSION);
  g_lcd.setCursor(0,1);
  g_lcd.print("DAVID-Laserscan");
  delay(3000);
  g_lcd.clear();
}


///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// DISPLAY ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

/// Prints a given floating point value on LCD, with given number of digits and precision.
/// @param lcd Reference to the LyquidCrystal object.
/// @param value The float value to be printed.
/// @param width The total number of digits to print (max. 20).
/// @param precision The number of positions after decimal point.
/// Example: value=12.345678, width=7, precision=3 will print " 12.346".
void printFormattedFloat(LiquidCrystal &lcd, float value, int width, int precision)
{
  char string[20];  // temporary string buffer
  dtostrf(value, width, precision, string);  // convert to string using dtostrf
  lcd.print(string);  // print
}


/// Shows the current state (step counter, laser on/off, motor speed, COM message) on LCD.
/// Prints only those values to the LCD which have changed.
void showState()
{
  // static variables remembering the last state (which is shown on display),
  // so that this function can know what has changed since last time,
  // and does not need to write values to display more than necessary:
  static int  last_isLaserOn = -42;
  static long last_motorPos = -42;  // for step display
  static long last_motorPos2 = -42;  // for speed measurement
  static int  last_speed = -42;
  static int  last_ch = 0;
  
  static unsigned long lastTime = 0;  // remember last call time stamp

  //-----------------
  // FIRST LCD LINE:
  //-----------------
  // Laser on/off:
  if (g_isLaserOn!=last_isLaserOn)  // if the laser has changed since last time
  {
    g_lcd.setCursor(0,0);
    g_lcd.print("Laser:");
    if (g_isLaserOn) g_lcd.print("1"); 
    else             g_lcd.print("0");
    last_isLaserOn = g_isLaserOn;  // remember new laser state
  }
  
  // motor step counter:
  if(last_motorPos!=g_motorPos)  // if the motor position has changed since last time
  {
    g_lcd.setCursor(8,0);
    g_lcd.print("Stp        ");
    g_lcd.setCursor(12,0);
    g_lcd.print(":");
    g_lcd.setCursor(13,0);
    g_lcd.print(g_motorPos);
    last_motorPos = g_motorPos;  // remember new motor position
  }
  
  //------------------
  // SECOND LCD LINE:
  //------------------
  // COM:
  if (last_ch!=g_lastCOM)
  {
    g_lcd.setCursor(0,1);
    g_lcd.print("C: ");
    g_lcd.setCursor(4,1);
    g_lcd.print((char)g_lastCOM);
    last_ch = g_lastCOM;
  }
  g_lastCOM = ' ';  // show nothing next time

  // Speed setting:
  if (last_speed!=g_speed)  // if speed has changed
  {
    g_lcd.setCursor(6,1);
    g_lcd.print("Spd:");
    if (g_speed<0) g_lcd.print("R");        // Show "R" for "Returning"
    else           g_lcd.print(g_speed+1);  // speed is [0-5], but show as [1-6]
    last_speed = g_speed;  // remember for next time
  }
  
  // Measured speed:
  {
    g_lcd.setCursor(13,1);
    long stepsPassed = g_motorPos-last_motorPos2;
    unsigned long now = micros();  // current time stamp
    if (0!=stepsPassed)
    {
      float dt = (float)(now-lastTime)/1E6;  // delta t in s
      float spd = (float)stepsPassed / dt;  // steps per second
      printFormattedFloat(g_lcd, fabs(spd), 5, 1);  // remove sign and convert speed to Hz in ###.# format
      g_lcd.print("Hz");
      lastTime=now;  // remember time stamp for next time
      last_motorPos2=g_motorPos;  // remember new motor position
    }
    else
    {
      g_lcd.print("       ");  // show no speed
    }
  } 
}


///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// MOTOR //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

/// Lets the motor move a given number of steps.
/// This is the only function that really sends signals to the motor and changes g_motorPos!
/// @param steps The number of steps. Use a negative value for backward direction.
/// @param motorDelay Delay between rising and falling edges [µs]. Default HOMING_MOTOR_DELAY.
void moveMotorRel(long steps, unsigned long motorDelay=HOMING_MOTOR_DELAY)
{
  if (steps>0) digitalWrite(PIN_OUT_DIR, HIGH);  // move forward
  else         digitalWrite(PIN_OUT_DIR, LOW);   // move backward
  delayMicroseconds(50);  // Time for the motor driver to set direction

  static unsigned long lastTime=0;
  for (long i=0; i<abs(steps); ++i)
  {
    delaySinceLastCall(motorDelay,lastTime);
    digitalWrite(PIN_OUT_CLOCK, HIGH);
    delaySinceLastCall(motorDelay,lastTime);
    digitalWrite(PIN_OUT_CLOCK, LOW);
  }

  g_motorPos += steps;  // update the global step counter
}

/// Moves the motor to a given absolute step position.
void moveMotorAbs(long pos, unsigned long motorDelay=HOMING_MOTOR_DELAY)
{
  moveMotorRel(pos-g_motorPos, motorDelay);
}

/// Moves the motor to a given absolute step position, and updates display regularly.
void moveMotorAbs_withDisplay(long pos, unsigned long motorDelay=HOMING_MOTOR_DELAY)
{
  // Go step by step:
  long stepsToGo = abs(pos-g_motorPos);  // how many steps
  long dir = (pos>g_motorPos) ? 1 : -1;  // direction of each step (1 or -1)

  unsigned long lastTime=0;  // time stamp for display update
  for (long i=0; i<stepsToGo; ++i)  // each step separately
  {
    moveMotorRel(dir,motorDelay);  // make one step

    unsigned long time=millis();      // in ms
    if (time-lastTime>DISPLAY_INTERVAL)  // if at least DISPLAY_INTERVAL ms have passed
    {
      showState();  // update display
      lastTime=time;  // reset time stamp
    }
  }
}

/// Moves the motor to the zero position. Always goes BACKLASH_STEPS over 0 to compensate gearbox backlash.
void moveMotorHome(unsigned long motorDelay=HOMING_MOTOR_DELAY)
{
  // move a little over 0 to compensate backlash:
  moveMotorAbs_withDisplay(-g_dir*BACKLASH_STEPS,  motorDelay);

  // move to 0:
  moveMotorAbs(0, motorDelay);
}

/// Moves the motor to the zero position. Always goes BACKLASH_STEPS over 0 to compensate gearbox backlash.
/// Uses moveMotorAbs_withDisplay which regularly updates the display.
void moveMotorHome_withDisplay(unsigned long motorDelay=HOMING_MOTOR_DELAY)
{
  int rememberedSpeed = g_speed;
  g_speed = -1;  // for display
  
  // move a little over 0 to compensate backlash:
  moveMotorAbs_withDisplay(-g_dir*BACKLASH_STEPS,  motorDelay);

  // update display:
  showState();
  delay(200);

  // move to 0:
  moveMotorAbs_withDisplay(0, motorDelay);
  delay(200);
  
  g_speed = rememberedSpeed;  // re-set speed
}



///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// LASER //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

/// setLaser(true) switches the laser on. setLaser(false) switches the laser off.
void setLaser(bool on)
{
  if (on==g_isLaserOn) return;  // nothing to do
  
  if (on==true)
  {
    // switch the laser on
    digitalWrite(PIN_OUT_LASER, HIGH);
  }
  else
  {
    // switch the laser off
    digitalWrite(PIN_OUT_LASER, LOW);
  }

  g_isLaserOn = on;  // remember the new laser state
}


///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// MAIN LOOP //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

/// the loop() method runs over and over again, as long as the Arduino has power
void loop()
{
  //------------------
  // Read input pins:
  //------------------
  // Laser: OFF, AUTO, or ON. See #eMode declaration.
  eMode laserMode = analogToMode(analogRead(PIN_IN_LASER));

  // Motor: OFF, AUTO, or ON. See #eMode declaration.
  eMode motorMode = analogToMode(analogRead(PIN_IN_MOTOR));

  // Direction: -1=down, 1=up
  g_dir = analogToDiscrete(analogRead(PIN_IN_DIR),2) * 2 - 1;
  
  if (FORCE_REMOTE_CONTROL)
  {
	// Overwrite to AUTO:
	laserMode = MODE_AUTO;
	motorMode = MODE_AUTO;
	g_dir = 1;
  }
  
  // Speed: 0 to NUM_SPEED_STEPS-1
  g_speed = analogToDiscrete(analogRead(PIN_IN_SPEED),NUM_SPEED_STEPS);

  // Move to zero
  bool moveHome = 1-analogToDiscrete(analogRead(PIN_IN_GO_ZERO),2);

  // Set counter to zero:
  bool setZero = !digitalRead(PIN_IN_SET_ZERO);

  //--------------------------
  // Act according to inputs:
  //--------------------------
  // Set laser:
  if      (MODE_OFF  ==laserMode) setLaser(false);
  else if (MODE_ON   ==laserMode) setLaser(true);
  else if (MODE_AUTO ==laserMode && MODE_AUTO!=motorMode) setLaser(MODE_ON==motorMode);
  
  // Motor:
  if (moveHome) moveMotorHome_withDisplay(HOMING_MOTOR_DELAY);
  if (MODE_ON==motorMode) moveMotorRel(g_dir,DELAYS[g_speed]);  // move one step in the desired direction

  // Counter:
  if (setZero) g_motorPos = 0;
  
  //----------------------------
  // Read serial input into ch:
  //----------------------------
  int ch = 0;
  if (Serial.available() > 0)
  {
    // read the incoming byte:
    ch = Serial.read();
    g_lastCOM = ch;  // remember in g_lastCOM (for display)
  }
  
  //--------------------------------------------------------
  // Act according to ch, if motorMode and laserMode allow:
  //--------------------------------------------------------
  if (MODE_AUTO==motorMode)  // allow remote control of motor
  {
    if ('+' == ch)      moveMotorRel(1);                                      // one step forward
    else if ('-' == ch) moveMotorRel(-1);                                     // one step backward
    else if ('>' == ch) moveMotorRel(NUM_STEPS_FAST);                         // several steps forward
    else if ('<' == ch) moveMotorRel(-NUM_STEPS_FAST);                        // several steps backward
    else if ('M' == ch) moveMotorRel(g_dir);                                  // one step forward
    else if ('Q' == ch) moveMotorHome_withDisplay(HOMING_MOTOR_DELAY);        // move to zero [deprecated?]
    else if ('#' == ch)  // move laser into camera image:
    {
      if (MODE_AUTO==laserMode) setLaser(true);                               // switch laser on
      moveMotorAbs_withDisplay(g_dir*MIDDLE_POS, HOMING_MOTOR_DELAY);         // move to MIDDLE_POS
    }
    else if ('P' == ch)  // Prepare for next scan:
    {
      if (MODE_AUTO==laserMode) setLaser(true);                               // switch laser on
      moveMotorHome_withDisplay(HOMING_MOTOR_DELAY);                          // move to zero 
      if (MODE_AUTO==laserMode) setLaser(false);                              // switch laser off
    }
  }
  
  if ('m' == ch)    // move motor with given number of steps
  {
    int sign = 1;  // The direction (+1 or -1)
    int numSteps = 0;  // number of steps to move
    for (int i=0; i<6; ++i)  // read up to 6 characters
    {
      while (Serial.available()<1) {};  // wait for new character
      int ch2 = Serial.read();  // read one new character
      if      ('+'==ch2) sign = 1;  // set positive direction
      else if ('-'==ch2) sign = -1;  // set negative direction
      else if (ch2>='0' && ch2<='9') numSteps = numSteps*10 + (ch2-'0');  // read one numeric digit into numSteps
      else break;  // invalid character, e.g. semicolon --> stop reading
    }
    numSteps *= sign*g_dir;  // apply direction + or -
    if (MODE_AUTO==motorMode)
    {
      moveMotorRel(numSteps);  // move!
    }
  }

  if (MODE_AUTO==laserMode)  // allow remote control of laser
  {
    if ('S' == ch) setLaser(true);                                            // scan start --> switch laser on
    else if ('L' == ch) setLaser(true);                                       // switch laser on
    else if ('l' == ch) setLaser(false);                                      // switch laser off
    else if ('1' == ch) setLaser(false);                                      // cam.calib --> switch laser off
    else if ('3' == ch) setLaser(false);                                      // texturing --> switch laser off
  }
  
  if ('T' == ch)  // received "Stop"
  {
    if (MODE_AUTO==laserMode) setLaser(false);                                // switch laser off
    if (MODE_AUTO==motorMode) moveMotorHome_withDisplay(HOMING_MOTOR_DELAY);  // move to zero
  }

  if ('0' == ch) g_motorPos = 0;                                              // Reset step counter

  //------------------------------------
  // Show the current state on display:
  //------------------------------------
  static unsigned long lastTime=0;  // in ms
  unsigned long time=millis();      // in ms
  if (time-lastTime>DISPLAY_INTERVAL)  // if at least DISPLAY_INTERVAL ms have passed
  {
    showState();  // update display
    lastTime=time;  // reset timer
  }
}


