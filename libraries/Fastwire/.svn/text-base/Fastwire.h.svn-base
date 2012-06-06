// I2C library
//////////////////////
// Copyright(C) 2011
// Francesco Ferrara
//////////////////////


#ifndef H_FASTWIRE
#define H_FASTWIRE

#include "WProgram.h"

/* Master */
#define TW_START		0x08
#define TW_REP_START		0x10
/* Master Transmitter */
#define TW_MT_SLA_ACK		0x18
#define TW_MT_SLA_NACK		0x20
#define TW_MT_DATA_ACK		0x28
#define TW_MT_DATA_NACK		0x30
#define TW_MT_ARB_LOST		0x38
/* Master Receiver */
#define TW_MR_ARB_LOST		0x38
#define TW_MR_SLA_ACK		0x40
#define TW_MR_SLA_NACK		0x48
#define TW_MR_DATA_ACK		0x50
#define TW_MR_DATA_NACK		0x58

class Fastwire
{
private:
  static boolean waitInt();

public:
  static void setup(int khz,boolean pullup);
  static byte write(byte device, byte address, byte value);
  static byte readBuf(byte device,byte address,byte *data,byte num);
  
};

#endif






















