/*
 William Greiman's modified version of Ladyada's wave shield library
 I have made many changes that may have introduced bugs.  Major changes are:
 optimized DAC macros to allow 44.1 k 16-bit files
 use of FatReader to read FAT32 and FAT16 files
 modified readwavhack to be readWaveData
 use standard SD and SDHC flash cards.
 skip non-data chunks after fmt chunk
 allow 18 byte format chunk if no compression
 play stereo as mono by interleaving channels
 change method of reading fmt chunk - use union of structs
*/
#include <string.h>
#include <avr/interrupt.h>
#include <mcpDac.h>
#include <WaveHC.h>
#include <WaveUtil.h>

// verify program assumptions
#if PLAYBUFFLEN != 256 && PLAYBUFFLEN != 512
#error PLAYBUFFLEN must be 256 or 512
#endif // PLAYBUFFLEN

WaveHC *playing = 0;

uint8_t buffer1[PLAYBUFFLEN];
uint8_t buffer2[PLAYBUFFLEN];
uint8_t *playend;      // end position for current buffer
uint8_t *playpos;      // position of next sample
uint8_t *sdbuff;       // SD fill buffer
uint8_t *sdend;        // end of data in sd buffer

// status of sd
#define SD_READY 1     // buffer is ready to be played
#define SD_FILLING 2   // buffer is being filled from DS
#define SD_END_FILE 3  // reached end of file
uint8_t sdstatus = 0;

//------------------------------------------------------------------------------
// timer interrupt for DAC
ISR(TIMER1_COMPA_vect) {
  if (!playing) return;

  if (playpos >= playend) {
    if (sdstatus == SD_READY) {
    
      // swap double buffers
      playpos = sdbuff;
      playend = sdend;
      sdbuff = sdbuff != buffer1 ? buffer1 : buffer2;
      
      sdstatus = SD_FILLING;
      // interrupt to call SD reader
	    TIMSK1 |= _BV(OCIE1B);
    }
    else if (sdstatus == SD_END_FILE) {
      playing->stop();
      return;
    }
    else {
      // count overrun error if not at end of file
      if (playing->remainingBytesInChunk) {
        playing->errors++;
      }
      return;
    }
  }

  uint8_t dh, dl;
  if (playing->BitsPerSample == 16) {
  
    // 16-bit is signed
    dh = 0X80 ^ playpos[1];
    dl = playpos[0];
    playpos += 2;
  }
  else {
  
    // 8-bit is unsigned
    dh = playpos[0];
    dl = 0;
    playpos++;
  }
  
#if DVOLUME
  uint16_t tmp = (dh << 8) | dl;
  tmp >>= playing->volume;
  dh = tmp >> 8;
  dl = tmp;
#endif //DVOLUME

  // dac chip select low
  mcpDacCsLow();
  
  // send DAC config bits
  mcpDacSdiLow();
  mcpDacSckPulse();  // DAC A
  mcpDacSckPulse();  // unbuffered
  mcpDacSdiHigh();
  mcpDacSckPulse();  // 1X gain
  mcpDacSckPulse();  // no SHDN
  
  // send high 8 bits
  mcpDacSendBit(dh,  7);
  mcpDacSendBit(dh,  6);
  mcpDacSendBit(dh,  5);
  mcpDacSendBit(dh,  4);
  mcpDacSendBit(dh,  3);
  mcpDacSendBit(dh,  2);
  mcpDacSendBit(dh,  1);
  mcpDacSendBit(dh,  0);
  
  // send low 4 bits
  mcpDacSendBit(dl,  7);
  mcpDacSendBit(dl,  6);
  mcpDacSendBit(dl,  5);
  mcpDacSendBit(dl,  4);
  
  // chip select high - done
  mcpDacCsHigh();

}
//------------------------------------------------------------------------------
// this is the interrupt that fills the playbuffer

ISR(TIMER1_COMPB_vect) {

  // turn off calling interrupt
  TIMSK1 &= ~_BV(OCIE1B);
  
  if (sdstatus != SD_FILLING) return;

  // enable interrupts while reading the SD
  sei();
  
  int16_t read = playing->readWaveData(sdbuff, PLAYBUFFLEN);
  
  cli();
  if (read > 0) {
    sdend = sdbuff + read;
    sdstatus = SD_READY;
  }
  else {
    sdend = sdbuff;
    sdstatus = SD_END_FILE;
  }
}
//------------------------------------------------------------------------------
/** create an instance of WaveHC. */
WaveHC::WaveHC(void) {
  fd = 0;
}
//------------------------------------------------------------------------------
/**
 * Read a wave file's metadata and initialize member variables.
 *
 * \param[in] f A open FatReader instance for the wave file.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.  Reasons
 * for failure include I/O error, an invalid wave file or a wave
 *  file with features that WaveHC does not support.
 */
uint8_t WaveHC::create(FatReader &f) {
  // 18 byte buffer
  // can use this since Arduino and RIFF are Little Endian
  union {
    struct {
      char     id[4];
      uint32_t size;
      char     data[4];
    } riff;  // riff chunk
    struct {
      uint16_t compress;
      uint16_t channels;
      uint32_t sampleRate;
      uint32_t bytesPerSecond;
      uint16_t blockAlign;
      uint16_t bitsPerSample;
      uint16_t extraBytes;
    } fmt; // fmt data
  } buf;
  
#if OPTIMIZE_CONTIGUOUS
  // set optimized read for contiguous files
  f.optimizeContiguous();
#endif // OPTIMIZE_CONTIGUOUS

  // must start with WAVE header
  if (f.read(&buf, 12) != 12
      || strncmp(buf.riff.id, "RIFF", 4)
      || strncmp(buf.riff.data, "WAVE", 4)) {
        return false;
  }

  // next chunk must be fmt
  if (f.read(&buf, 8) != 8
      || strncmp(buf.riff.id, "fmt ", 4)) {
        return false;
  }
  
  // fmt chunk size must be 16 or 18
  uint16_t size = buf.riff.size;
  if (size == 16 || size == 18) {
    if (f.read(&buf, size) != (int16_t)size) {
      return false;
    }
  }
  else {
    // compressed data - force error
    buf.fmt.compress = 0;
  }
  
  if (buf.fmt.compress != 1 || (size == 18 && buf.fmt.extraBytes != 0)) {
    putstring_nl("Compression not supported");
    return false;
  }
  
  Channels = buf.fmt.channels;
  if (Channels > 2) {
    putstring_nl("Not mono/stereo!");
    return false;
  }
  else if (Channels > 1) {
    putstring_nl(" Warning stereo file!");
  }
  
  BitsPerSample = buf.fmt.bitsPerSample;
  if (BitsPerSample > 16) {
    putstring_nl("More than 16 bits per sample!");
    return false;
  }
  
  dwSamplesPerSec = buf.fmt.sampleRate;
  uint32_t clockRate = dwSamplesPerSec*Channels;
  uint32_t byteRate = clockRate*BitsPerSample/8;
  
#if RATE_ERROR_LEVEL > 0
  if (clockRate > MAX_CLOCK_RATE
      || byteRate > MAX_BYTE_RATE) {
    putstring_nl("Sample rate too high!");
    if (RATE_ERROR_LEVEL > 1) {
      return false;
    }
  }
  else if (byteRate > 44100 && !f.isContiguous()) {
    putstring_nl("High rate fragmented file!");
    if (RATE_ERROR_LEVEL > 1) {
      return false;
    }
  }
#endif // RATE_ERROR_LEVEL > 0

  fd = &f;

  errors = 0;
  isplaying = 0;
  remainingBytesInChunk = 0;
  
#if DVOLUME
  volume = 0;
#endif //DVOLUME
  // position to data
  return readWaveData(0, 0) < 0 ? false: true;
}
//------------------------------------------------------------------------------
/**
 * Returns true if the player is paused else false.
 */
uint8_t WaveHC::isPaused(void) {
  cli();
  uint8_t rtn = isplaying && !(TIMSK1 & _BV(OCIE1A));
  sei();
  return rtn;
}
//------------------------------------------------------------------------------
/**
 * Pause the player.
 */
void WaveHC::pause(void) {
  cli();
  TIMSK1 &= ~_BV(OCIE1A); //disable DAC interrupt
  sei();
  fd->volume()->rawDevice()->readEnd(); // redo any partial read on resume
}
//------------------------------------------------------------------------------
/**
 * Play a wave file.
 *
 * WaveHC::create() must be called before a file can be played.
 *
 * Check the member variable WaveHC::isplaying to monitor the status
 * of the player.
 */
void WaveHC::play(void) {
  // setup the interrupt as necessary

  int16_t read;

  playing = this;

  // fill the play buffer
  read = readWaveData(buffer1, PLAYBUFFLEN);
  if (read <= 0) return;
  playpos = buffer1;
  playend = buffer1 + read;

  // fill the second buffer
  read = readWaveData(buffer2, PLAYBUFFLEN);
  if (read < 0) return;
  sdbuff = buffer2;
  sdend = sdbuff + read;
  sdstatus = SD_READY;
  
  // its official!
  isplaying = 1;
  
  // Setup mode for DAC ports
  mcpDacInit();
  
  // Set up timer one
  // Normal operation - no pwm not connected to pins
  TCCR1A = 0;
  // no prescaling, CTC mode
  TCCR1B = _BV(WGM12) | _BV(CS10); 
  // Sample rate - play stereo interleaved
  OCR1A =  F_CPU / (dwSamplesPerSec*Channels);
  // SD fill interrupt happens at TCNT1 == 1
  OCR1B = 1;
  // Enable timer interrupt for DAC ISR
  TIMSK1 |= _BV(OCIE1A);
}
//------------------------------------------------------------------------------
/** Read wave data.
 *
 * Not for use in applications.  Must be public so SD read ISR can access it.
 * Insures SD sectors are aligned with buffers.
 */
int16_t WaveHC::readWaveData(uint8_t *buff, uint16_t len) {

  if (remainingBytesInChunk == 0) {
    struct {
      char     id[4];
      uint32_t size;
    } header;
    while (1) {
      if (fd->read(&header, 8) != 8) return -1;
      if (!strncmp(header.id, "data", 4)) {
        remainingBytesInChunk = header.size;
        break;
      }
 
      // if not "data" then skip it!
      if (!fd->seekCur(header.size)) {
        return -1;
      }
    }
  }

  // make sure buffers are aligned on SD sectors
  uint16_t maxLen = PLAYBUFFLEN - fd->readPosition() % PLAYBUFFLEN;
  if (len > maxLen) len = maxLen;

  if (len > remainingBytesInChunk) {
    len = remainingBytesInChunk;
  }
  
  int16_t ret = fd->read(buff, len);
  if (ret > 0) remainingBytesInChunk -= ret;
  return ret;
}
//------------------------------------------------------------------------------
/** Resume a paused player. */
void WaveHC::resume(void) {
  cli();
  // enable DAC interrupt
  if(isplaying) TIMSK1 |= _BV(OCIE1A);
  sei();
}
//------------------------------------------------------------------------------
/**
 * Reposition a wave file.
 *
 * \param[in] pos seek will attempt to position the file near \a pos.
 * \a pos is the byte number from the beginning of file.
 */
void WaveHC::seek(uint32_t pos) {
  // make sure buffer fill interrupt doesn't happen
  cli();
  if (fd) {
    pos -= pos % PLAYBUFFLEN;
    if (pos < PLAYBUFFLEN) pos = PLAYBUFFLEN; //don't play metadata
    uint32_t maxPos = fd->readPosition() + remainingBytesInChunk;
    if (maxPos > fd->fileSize()) maxPos = fd->fileSize();
    if (pos > maxPos) pos = maxPos;
    if (fd->seekSet(pos)) {
      // assumes a lot about the wave file
      remainingBytesInChunk = maxPos - pos;
    }
  }
  sei();
}
//------------------------------------------------------------------------------
/** Set the player's sample rate.
 *
 * \param[in] samplerate The new sample rate in samples per second.
 * No checks are done on the input parameter.
 */
void WaveHC::setSampleRate(uint32_t samplerate) {
  if (samplerate < 500) samplerate = 500;
  if (samplerate > 50000) samplerate = 50000;
  // from ladayada's library.
  cli();
  while (TCNT0 != 0);
  
  OCR1A = F_CPU / samplerate;
  sei();
}
//------------------------------------------------------------------------------
/** Stop the player. */
void WaveHC::stop(void) {
  TIMSK1 &= ~_BV(OCIE1A);   // turn off interrupt
  playing->isplaying = 0;
  playing = 0;
}
