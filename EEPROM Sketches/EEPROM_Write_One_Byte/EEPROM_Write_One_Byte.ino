#include <avr/io.h>
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"


EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;


void setup() {
  Serial.begin(9600);
  int address;
  unsigned char value;
  
  Serial.println("What Address?");
  address = term_input.termInt();
  
  Serial.println("What Value?");
  value = term_input.termInt();
  
  eeprom.EEPROM_write(address, value);
  Serial.println("Done");
  

}

void loop() {
  // put your main code here, to run repeatedly:

}
