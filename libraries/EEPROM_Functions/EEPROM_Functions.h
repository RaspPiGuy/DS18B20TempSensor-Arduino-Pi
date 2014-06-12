
/* 
Write to and reads from the ATmega328P EEPROM
 
MJL - thepiandi.blogspot.com  05/09/14
*/

#ifndef EEPROM_FUNCTIONS_H
#define EEPROM_FUNCTIONS_H

#include <avr/io.h>
#include "Arduino.h"

class EEPROM_FUNCTIONS{
  public:
    unsigned char EEPROM_read(int address);
    void EEPROM_write(int address, unsigned char data);
};

#endif
      

