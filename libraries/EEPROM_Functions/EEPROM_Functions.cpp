#include "EEPROM_Functions.h"
#include <avr/io.h>

void EEPROM_FUNCTIONS::EEPROM_write(int address, unsigned char data){
  byte addrh, addrl;
 
  addrh = address / 256;
  addrl = address % 256;
  
  //wait for completion of any previous write operation
  while(EECR & (1<<EEPE));
  
  //Setup address
  EEARH = addrh;
  EEARL = addrl;
  
  //Setup data
  EEDR = data;
  
  //Setup EEPM1 EEPM0 to 00
  EECR &= !(1<<EEPM1);
  EECR &= !(1<<EEPM0);
  
  
  //Write logical 1 to EEMPE
  EECR |= (1<<EEMPE);
  
  // Trigger EEPROM write
  EECR |= (1<<EEPE);
}

unsigned char EEPROM_FUNCTIONS::EEPROM_read(int address){
  byte addrh, addrl;
 
  addrh = address / 256;
  addrl = address % 256;
  
  //wait for completion of any previous write operation
  while(EECR & (1<<EEPE));
  
  //Setup address
  EEARH = addrh;
  EEARL = addrl;
  
  //Trigger EEPROM read
  EECR |= (1<<EERE);
  
  return EEDR;
}

