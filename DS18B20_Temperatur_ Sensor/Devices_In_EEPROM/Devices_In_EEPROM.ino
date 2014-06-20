/*  Devices_In_EEPROM.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM.
2.  For each device, the ROM code and the description is displayed.

The EEPROM_Functions library is used to read the device's ROM code and description from EEPROM.
*/
#include "EEPROM_Functions.h"
EEPROM_FUNCTIONS eeprom;

void devices_in_EEPROM(){
  int i, j, address;
  int stored_devices;
  int character;
  
  //How Many devices stored in the EEPROM?
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  if(stored_devices){
    for (i = 0; i < stored_devices; i++){
      address = 20 * i + 4;  //jump to first byte of device in EEPROM
      Serial.print("Device No: ");
      Serial.println(i + 1);
      Serial.print("\tIt's ROM code is: ");  
      for ( j = 0; j < 8; j++){
        Serial.print(eeprom.EEPROM_read(address + j), HEX);
        Serial.print(" ");
      }
      Serial.println("");
      Serial.print("\tWith this description: ");
      character = 0;
      for ( j = 8; (j < 20 && character != 255); j++){
        character = eeprom.EEPROM_read(address +j);
        if (character != 255){
          Serial.print(char(character));
        }
      }
      Serial.println("");
    }    
  }
  else{    
    Serial.println("There are no devices stored in the EEPROM");
  }
}



void setup() {
  Serial.begin(9600);
  devices_in_EEPROM();

}

void loop() {
  // put your main code here, to run repeatedly:

}
