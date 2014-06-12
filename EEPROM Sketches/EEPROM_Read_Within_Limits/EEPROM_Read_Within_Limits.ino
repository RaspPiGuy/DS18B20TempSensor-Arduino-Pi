#include <avr/io.h>
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"

EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;

void setup() {
 
  Serial.begin(9600);
  
  unsigned char data;
  int address;
  int add_high, add_low;

  
  Serial.println("Beginning Address");
  add_low = term_input.termInt();
  Serial.println("Ending Address");
  add_high = term_input.termInt();

  
  for (address = add_low; address <= add_high; address++){
    data = eeprom.EEPROM_read(address);
    Serial.print("Address: ");
    Serial.print(address);
    Serial.print(" value: ");
    Serial.println(data, HEX);
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
