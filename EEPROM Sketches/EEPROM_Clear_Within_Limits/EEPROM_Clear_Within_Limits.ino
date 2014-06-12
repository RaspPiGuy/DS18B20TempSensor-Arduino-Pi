#include <avr/io.h>
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"


EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;


void setup() {
  Serial.begin(9600);
  int address;
  int add_high, add_low;
  unsigned char value;
  
  Serial.println("Beginning Address");
  add_low = term_input.termInt();
  Serial.println("Ending Address");
  add_high = term_input.termInt();

  Serial.println("Starting to write");
  
  for (address = add_low; address <= add_high; address++){
    value = 255;   
    eeprom.EEPROM_write(address, value);
  }
    
  Serial.println("Finished Writing");
  

}

void loop() {
  // put your main code here, to run repeatedly:

}
