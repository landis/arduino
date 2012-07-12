// This class is an interface for the Arduino Wave Shield
// Pins 2, 3, 4, 5 are used by the Arduino Wave Shield

MediaPlayer::MediaPlayer():
pausePosition(0)
{ openMemoryCard();
  card.reset_dir();
  setupWaveShieldPins();
}

MediaPlayer::~MediaPlayer()
{ stop();
}

void MediaPlayer::setupWaveShieldPins()
{ pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
}

bool MediaPlayer::play(char *fileName)
{ stop();
  file = card.open_file(fileName);  
  if (!file)
  { putstring(" Couldn't open WAV file");
    return false;
  }  
  if (!waveFile.create(file))
  { putstring(" Not a valid WAV file");
    return false;
  }
  waveFile.play();
  return true;
}

void MediaPlayer::resume()
{ if (!isPlaying() & (bool)file) // If not stopped
  { waveFile.seek(pausePosition);
    waveFile.play();
  }
}

void MediaPlayer::stop()
{ if (isPlaying()) waveFile.stop();
  if (file) file = closeFile(file);
  card.reset_dir(); // if not done after card.get_next_name_in_dir() do it here
  pausePosition = 0;
}

void MediaPlayer::pause()
{ if (isPlaying())
  { pausePosition = waveFile.getSize() - waveFile.remainingBytesInChunk;
    waveFile.stop();
  }
}

File MediaPlayer::closeFile(const File file) // This should be done in AF_Wave.cpp
{ card.close_file(file);
  return 0;
}

void MediaPlayer::openMemoryCard()
{ if (!card.init_card())
  { putstring_nl("Card init failed");
    return;
  }
  if (!card.open_partition())
  { putstring_nl("No partition");
    return;
  }
  if (!card.open_filesys())
  { putstring_nl("Couldn't open filesys");
    return;
  }
 if (!card.open_rootdir())
  { putstring_nl("Couldn't open dir");
    return;
  }
}

bool MediaPlayer::isPlaying()
{ return waveFile.isplaying;
}

bool MediaPlayer::onPause()
{ return !isPlaying() & (bool)pausePosition;
}

int MediaPlayer::exploreSDcard(const bool print)
{ int fileCount=0;
  char fileName[13];
  card.reset_dir();
  while(card.get_next_name_in_dir(fileName))
  { fileCount++;
    if(print)
    { Serial.print(fileCount);
      putstring(". ");
      Serial.println(fileName);
    }
  }
  card.reset_dir();
  return fileCount;
}

void MediaPlayer::fileName(const int fileNumber, char* _fileName)
{ int fileCount=1;
  char fileName[13];
  card.reset_dir();  
  while(card.get_next_name_in_dir(fileName))
  { if(fileCount++==fileNumber) break;
    else fileName[0]=0;
  }
  card.reset_dir();  
  strncpy(_fileName, fileName, 12); // fill the string argument
  _fileName[12] = 0;    
}

 

