//NAG Version 1.6

#include <avr/pgmspace.h>  // Adding Library
#include "util.h"          // Adding Library
#include "MediaPlayer.h"   // Adding Library
#include <Wire.h>          // Adding Library
#include <LiquidCrystal.h> // Adding Library

LiquidCrystal lcd(9, 8, 7, 6, 15, 14); // Sets the pins that will be used for the LCD screen
MediaPlayer mediaPlayer;               // Class

const int sensor = 16; // Sets value for "sensor" to 16 which defines the pin for the Proximity sensor later
int seconds;       // Defining integer
int minutes;       // Defining integer
int hours;         // Defining integer
int day;           // Defining integer
int date;          // Defining integer
int month;         // Defining integer
int year;          // Defining integer
int count = 0;     // Defining integer
int motion = 0;    // Defining integer
int yes = 0;       // Defining integer
int countdelay1 = 0; //This is set to 1200 when an event is triggered by motion so the NAG doesn't NAG too much
int eventcount = 0;  //This is incremented to determine if the event should be triggered, or already has been.


void loop() //This section of code is executed in a loop repeatedly with a delay of 1 second between each go-round.	 
  {
    reset(); //Resets values
    timer(); //Decrements the countdelay1 value, acts as a timer
    movement();
    hourspeak();
    dailyevents();
    get_time();
    get_date();
    display_time();
    display_date();
    display_message();
    delay(1000);
  }
  
  
void initChrono() //Called under MediaPlayerTestFuncions tab when remark is removed from initChrono statement
  {
    set_time();
    set_date();
  }
  
  
void set_time()  //Called by initChrono to set time to values edited on MediaPlayerTestFunctions tab
  {
    Wire.beginTransmission(104);
    Wire.send(0);
    Wire.send(decToBcd(seconds));
    Wire.send(decToBcd(minutes));
    Wire.send(decToBcd(hours));
    Wire.endTransmission();
  }
  
  
void set_date() //Called by initChrono to set date to values edited on MediaPlayerTestFunctions tab
  {
    Wire.beginTransmission(104);
    Wire.send(3);
    Wire.send(decToBcd(day));
    Wire.send(decToBcd(date));
    Wire.send(decToBcd(month));
    Wire.send(decToBcd(year));
    Wire.endTransmission();
  }

void timer() //Decrements the countdelay1 value to act as a timer
  {
    countdelay1 --;
    if(countdelay1 <= 0)
      {
        countdelay1 = 0;
      }
  }
  
  
void hourspeak()  // Triggers Hour statements when desired at top of the hour edit "speak" section for files
  {
    if(minutes == 0)
      {
        speak();
      }
  }


void dailyevents() //Triggers "event" at 5 minutes after the hour for 8, 9 and 10PM
  {
    if(minutes == 5) // 5 minutes after the hour
      {
        if(hours == 20 || hours == 21 || hours == 22) // 8PM, 9PM and 10PM
          {
            event(); // Triggers for events i.e. recycles, trash, etc.
          }
      }
  }
  
  
void movement() //Checks movement with PIR Sensor, and triggers sound according to the time. 
  {
   motion = digitalRead(sensor);
   if(motion == HIGH)
      { 
       yes ++;
          {
           if(day == 4) // Wednesday
              {
               if(hours == 19 || hours == 20 || hours == 21 || hours == 22)
                  {
                   if (countdelay1 == 0)
                      {
                       mediaPlayer.play("RECYCLES.WAV");
                       countdelay1 = 1200; // Replace "1200" with "5" to test PIR sensor; will decrease the delay to 5 seconds.
                      }
                  }
              }
           if(day == 3) // Tuesday
              {
               if(hours == 19 || hours == 20 || hours == 21 || hours == 22)
                  {
                   if(countdelay1 == 0)
                      {
                       mediaPlayer.play("TRASH.WAV");
                       countdelay1 = 1200; // Replace "1200" with "5" to test PIR sensor; will decrease the delay to 5 seconds.
                      }
                  }
              }
          }
      }
  }
  
  
void reset()
  {
   if(minutes > 0)
   count = 0;
   
   if(minutes > 5)
   eventcount = 0;
  }
  
  
void event() //Called by "dailyevents"
  {
   if(day == 4) // Wednesday
      {
       if(eventcount == 0) //only plays sound file if "eventcount" is 0
          {
           mediaPlayer.play("RECYCLES.WAV"); // sound file on SD card
           eventcount ++; //increments eventcount so NAG doesn't NAG too often
          }
      }
   if(day == 3) // Tuesday
      {
       if(eventcount == 0)
          {
           mediaPlayer.play("TRASH.WAV");
           eventcount ++;
          }
      }
  }
  
  
void speak() //This is called by HourSpeak if the minutes = 0
  {
   if(hours == 19 || hours == 7)
      {
       if(count == 0)
          {
           mediaPlayer.play("SEVEN.WAV");
           count ++;
          }
      }
   if(hours == 20 || hours == 8)
      {
       if(count == 0)
          {
           mediaPlayer.play("EIGHT.WAV");
           count ++;
          }
      }
   if(hours == 21 || hours == 9)
      {
       if(count == 0)
          {
           mediaPlayer.play("NINE.WAV");
           count ++;
          }
      }
  }
  
  
void display_time() // Sets start point for LCD print functions and prints data and quoted words
  {
   char buf[12];
   lcd.setCursor(0, 0);
   
   if(hours == 0)
      {
       lcd.clear();
      }
   lcd.print("Time ");
   
   if(hours < 13)
      {
       if(hours < 10 && hours > 0)
          {
           lcd.print("0");
          }
       if (hours == 0)
          {
           lcd.print("12");
          }
       else
          {
           lcd.print(itoa(hours, buf, 10));
          }
      }
      
   if(hours >= 13)
      {
       if(hours - 12 < 10)
          {
           lcd.print("0");
          }
       lcd.print(itoa(hours - 12, buf, 10));
      }
      
   lcd.print(":");
   
   if(minutes < 10)
     {
      lcd.print("0");
     }
   lcd.print(itoa(minutes, buf, 10));
   lcd.print(":");
   
   if(seconds < 10)
      {
       lcd.print("0");
      }
      
   lcd.print(itoa(seconds, buf, 10));
   
   if(hours >= 12)
      {
       lcd.print("PM v1.6");
      }
      
   if(hours < 12)
      {
       lcd.print("AM v1.6");
      }
  }
  
  
void display_date() // Sets start point for LCD print functions and prints data and quoted words
  {
   char buf[12];
   lcd.setCursor(0, 1);
   lcd.print("Date ");
   
   if(day == 1)
      {
       lcd.print("SUN");
      }
     
   if(day == 2)
      {
       lcd.print("MON");
      }
      
   if(day == 3)
      {
       lcd.print("TUE");
      }
      
   if(day == 4)
      {
       lcd.print("WED");
      }
      
   if(day == 5)
      {
       lcd.print("THU");
      }
      
   if(day == 6)
      {
       lcd.print("FRI");
      }
      
   if(day == 7)
      {
       lcd.print("SAT");
      }
      
   lcd.print(" ");
   
   if(month < 10)
      {
       lcd.print("0");
      } 
      
   lcd.print(itoa(month, buf, 10));
   lcd.print("/");
  
   if(date < 10)
      {
       lcd.print("0");
      }
      
   lcd.print(itoa(date, buf, 10)); 
   lcd.print("/"); 
   
   if(year < 10)
      {
       lcd.print("0");
      } 
      
   lcd.print(itoa(year, buf, 10)); 
  }
  
  
void display_message()  // Sets LCD start point and prints Event Messages, i.e. Birthday, Holiday, etc.
  {
   lcd.setCursor(0, 2);
   
   if(month == 2 && date == 13)
      {
       lcd.print("Valentines Tomorrow");
      }
      
   if(month == 2 && date == 15)
      {
       lcd.print("Chaumont Tomorrow!");
      }
      
   if(month == 2 && date == 14)
      {
       lcd.print("Happy Valentine's!!!");
      }
      
   if(month == 2 && date == 20)
      {
       lcd.print("Trina B-Day Tomorrow");
      }
      
   if(month == 2 && date == 21)
      {
       lcd.print("Happy B-Day Trina!");
      }
      
   if(month == 3 && date == 28)
      {
       lcd.print("Taylor B-day Tmrw!!");
      }
      
   if(month == 3 && date == 29)
      {
       lcd.print("Happy B-Day Taylor!");
      }
      
   if(month == 3 && date == 20)
      {
       lcd.print("Dad B-day Tomorrow!!");
      }
      
   if(month == 3 && date == 21)
      {
       lcd.print("Happy B-Day Dad!");
      }
      
   if(month == 4 && date == 13)
      {
       lcd.print("Mom B-day Tomorrow!!");
      }
      
   if(month == 4 && date == 14)
     {
      lcd.print("Happy B-Day Mom!!");
     }

  // Sets LCD start point and prints Weekly Messages, i.e. Trash night, Recycles, etc.
  
   lcd.setCursor(0, 3);
   
   if(day == 1)
      {
       lcd.print("Work Tomorrow!");
      }
      
  if(day == 2)
     {
      lcd.print("Mondays Rock!!");
     }
     
   if(day == 4)
      {
       lcd.print("Recycle Night");
      }
      
   if(day == 3)
      {
       lcd.print("Trash Night");
      }
      
   if(day == 5)
      {
       lcd.print("Thirsty Thursday");
      }
      
   if(day == 6)
      {
       lcd.print("TGIF!!!!!");
      }
      
   if(day == 7)
      {
       lcd.print("It's Saturday!");
      }
  }


void get_date() // queries Chronodot for current Date info.
  {
   Wire.beginTransmission(104); 
   Wire.send(3);//set register to 3 (day)
   Wire.endTransmission();
   Wire.requestFrom(104, 4); //get 5 bytes(day,date,month,year,control);
   day   = bcdToDec(Wire.receive());
   date  = bcdToDec(Wire.receive());
   month = bcdToDec(Wire.receive());
   year  = bcdToDec(Wire.receive());
  }
  
  
void get_time() // Queries Chronodot for current Time info.
  {
   Wire.beginTransmission(104); 
   Wire.send(0);//set register to 0
   Wire.endTransmission();
   Wire.requestFrom(104, 3);//get 3 bytes (seconds,minutes,hours);
   seconds = bcdToDec(Wire.receive() & 0x7f);
   minutes = bcdToDec(Wire.receive());
   hours = bcdToDec(Wire.receive() & 0x3f);
  }
  
  
byte decToBcd(byte val)
  {
   return ( (val/10*16) + (val%10) );
  }
  
  
byte bcdToDec(byte val)
  {
   return ( (val/16*10) + (val%16) );
  }
