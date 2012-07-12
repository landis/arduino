void setup()  /* Leave the time as is when testing NAG for the first time. This will set the time to 7:59:55 PM,
               thus letting the NAG trigger the 8:00PM sound file in 5 seconds. */
  {
    pinMode(sensor, INPUT);
    Wire.begin();
    lcd.begin(20, 4);
    seconds = 55;  // Change to current value when setting time
    minutes = 59;  // Change to current value when setting time
    hours = 19;    // Change to current value when setting time
    day = 3;       // Change to current value when setting time
    date = 14;     // Change to current value when setting time
    month = 2;     // Change to current value when setting time
    year = 12;     // Change to current value when setting time
//  initChrono();  // remark this line when not setting time
  }


void playComplete(char* fileName)
  {
    mediaPlayer.play(fileName);
    while (mediaPlayer.isPlaying())
      { 
        putstring(".");
        delay(100);
      }
  }


void playComplete(const int fileNumber)
  { 
    char fileName[13];
    mediaPlayer.fileName(fileNumber, fileName);
    playComplete(fileName);
  }

