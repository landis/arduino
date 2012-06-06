/*
FastWire 0.1
This is a library to help faster programs to read I2C devices.
Copyright(C) 2011 Francesco Ferrara
occhiobello at gmail dot com
*/

#include "Fastwire.h"

boolean Fastwire::waitInt()
  {
    int l=250;

    while (!(TWCR & (1<<TWINT)) && l-->0);
    return l>0;
  }



  void Fastwire::setup(int khz,boolean pullup)
  {
    TWCR=0;
    if (pullup)
      PORTC |= ((1<<4)|(1<<5)); // TODO other micro
    else
      PORTC &= ~((1<<4)|(1<<5)); // TODO other micro



    TWSR = 0;        // no prescaler => prescaler = 1
    TWBR = ((16000L / khz) - 16) / 2; // change the I2C clock rate
    TWCR = 1<<TWEN;  // enable twi module, no interrupt
  }


  /************************************************************************************************/
  byte Fastwire::write(byte device, byte address, byte value)
  {
    byte twst,retry;

    retry=2;
    do
    {
      TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO) | (1<<TWSTA)   ; 
      if (!waitInt()) return 1;
      twst=TWSR & 0xF8;
      if (twst != TW_START && twst != TW_REP_START) return 2;

      TWDR = device&0xFE; // send device address without read bit (1)
      TWCR = (1<<TWINT) | (1<<TWEN);
      if (!waitInt()) return 3;
      twst=TWSR & 0xF8;
    } 
    while (twst == TW_MT_SLA_NACK && retry-- > 0);
    if (twst != TW_MT_SLA_ACK) return 4;


    TWDR = address; // send data to the previously addressed device
    TWCR = (1<<TWINT) | (1<<TWEN);
    if (!waitInt()) return 5;
    twst=TWSR & 0xF8;
    if (twst != TW_MT_DATA_ACK) return 6;

    TWDR = value; // send data to the previously addressed device
    TWCR = (1<<TWINT) | (1<<TWEN);
    if (!waitInt()) return 7;
    twst=TWSR & 0xF8;
    if (twst != TW_MT_DATA_ACK) return 8;

    return 0;
  }

  /************************************************************************************************/

  byte Fastwire::readBuf(byte device,byte address,byte *data,byte num)
  {
    byte twst,retry;

    retry=2;
    do
    {
      TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO)  | (1<<TWSTA)  ; 
      if (!waitInt()) return 16;
      twst=TWSR & 0xF8;
      if (twst != TW_START && twst != TW_REP_START) return 17;


      TWDR = device & 0xfe; // send device address to write
      TWCR = (1<<TWINT) | (1<<TWEN);
      if (!waitInt()) return 18;
      twst=TWSR & 0xF8;
    } 
    while (twst == TW_MT_SLA_NACK && retry-- > 0);
    if (twst != TW_MT_SLA_ACK) return 19;

    TWDR = address; // send data to the previously addressed device
    TWCR = (1<<TWINT) | (1<<TWEN);
    if (!waitInt()) return 20;
    twst=TWSR & 0xF8;
    if (twst != TW_MT_DATA_ACK) return 21;




    /***/

    retry=2;
    do
    {
      TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO) | (1<<TWSTA)  ; 
      if (!waitInt()) return 22;
      twst=TWSR & 0xF8;
      if (twst != TW_START && twst != TW_REP_START) return 23;

      TWDR = device|0x01; // send device address with the read bit (1)
      TWCR = (1<<TWINT) | (1<<TWEN);
      if (!waitInt()) return 24;
      twst=TWSR & 0xF8;
    } 
    while (twst == TW_MR_SLA_NACK && retry-- > 0);
    if (twst != TW_MR_SLA_ACK) return 25;

    for(uint8_t i=0;i<num;i++)
    {
      if (i==num-1)
        TWCR = (1<<TWINT) | (1<<TWEN) ;
      else
        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
      if (!waitInt()) return 26;
      twst=TWSR & 0xF8;
      if (twst != TW_MR_DATA_ACK && twst != TW_MR_DATA_NACK) return twst;
      data[i]=TWDR;
    }

    return 0;

  }
