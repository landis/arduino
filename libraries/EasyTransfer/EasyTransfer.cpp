#include "EasyTransfer.h"




//Captures address and size of struct
void EasyTransfer::begin(uint8_t * ptr, uint8_t length, HardwareSerial *theSerial){
address = ptr;
size = length;
_serial = theSerial;
}

//Sends out struct in binary, with header, length info and checksum
void EasyTransfer::sendData(){
  uint8_t CS = size;
  _serial->print(0x06, BYTE);
  _serial->print(0x85, BYTE);
  _serial->print(size, BYTE);
  for(int i = 0; i<size; i++){
    CS^=*(address+i);
    _serial->print(*(address+i), BYTE);
  }
  _serial->print(CS);

}

boolean EasyTransfer::receiveData(){
  
  if(rx_len == 0){
    if(_serial->available() >= 3){
      while(_serial->read() != 0x06) {}
      if (_serial->read() == 0x85){
        rx_len = _serial->read();
        if(rx_len != size){
          rx_len = 0;
          return false;
        }
      }
    }
  }
  
  
  if(rx_len != 0){
    while(_serial->available() && rx_array_inx <= rx_len){
      rx_array[rx_array_inx++] = _serial->read();
    }
    
    if(rx_len == (rx_array_inx-1)){
      //seem to have got whole message
      //last uint8_t is CS
      calc_CS = rx_len;
      for (int i = 0; i<rx_len; i++){
        calc_CS^=rx_array[i];
      } 
      
      if(calc_CS == rx_array[rx_array_inx-1]){//CS good
        memcpy(address,&rx_array,size);
		rx_len = 0;
		rx_array_inx = 0;
		return true;
		}
		
	  else{
	  //failed checksum, need to clear this out anyway
		rx_len = 0;
		rx_array_inx = 0;
		return false;
	  }
        
    }
  }
  
  return false;
}