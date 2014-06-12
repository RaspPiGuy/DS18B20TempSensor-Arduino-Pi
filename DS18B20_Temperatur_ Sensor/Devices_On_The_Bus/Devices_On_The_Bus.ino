/*  Devices_On_The_Bus.ino    thepiandi@blogspot.com       MJL  060614

Uses the DS18B20 Search rom command in the One_wire_DS18B20 library to find the ROM code
of each device that it finds on the bus.  Once the ROM code is found, CRC is performed and
the results displayed.
*/

#include "One_wire_DS18B20.h"

DS18B20_INTERFACE ds18b20;

void devices_on_bus(){
  int i; 
  int last_device;
  byte Rom_no[8] = {0,0,0,0,0,0,0,0};
  byte New_Rom_no[8] = {0,0,0,0,0,0,0,0};
 
  last_device = 0;
  //Do until the last device connected to the bus is found by search_rom().
  do {
    if (!ds18b20.initialize()){  //Must initialize before finding each device.
      last_device = ds18b20.search_rom(Rom_no, New_Rom_no, last_device);      

      for (i = 0; i < 8; i++){
        Serial.print(New_Rom_no[i], HEX);
        Serial.print(" ");
      }
      if (ds18b20.calculateCRC_byte(New_Rom_no, 8)){
        Serial.println("Device ROM failed CRC");
      }
      else{
        Serial.println("   CRC Passed");
      }
      for (i = 0; i < 8; i++){
        Rom_no[i] = New_Rom_no[i];
        New_Rom_no[i] = 0;
      }
    }
    else{
      Serial.println("Initialization Failure on ROM Read. Perhaps No Device On The Bus"); 
    }  
   //search_rom() will return last_device as 0 when last device on bus has been found
  }while (last_device);
}
 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  devices_on_bus();
}

void loop() {
  // put your main code here, to run repeatedly:

}
