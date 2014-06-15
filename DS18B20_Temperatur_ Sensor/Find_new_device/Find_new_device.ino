/*  Find_new_device.ino    thepiandi@blogspot.com       MJL  061514

1.  Uses the DS18B20 Search rom command to find the ROM code of each device that it finds on the bus.  
2.  Once a new device is found, it's ROM code is displayed
3.  A CRC check is made on the ROM code and the results of the CRC is displayed.
4.  The user is asked to input a description.
5.  The device number, ROM code, and the description is written to the EEPROM.
6.  If no new device is found, the user is made aware of that fact

Uses the One_wire_DS18B20 library functions for basic one wire operation.
The EEPROM_Functions library is used to read from and write to the EEPROM
*/


#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;

void new_device_to_EEPROM(){
  int i, address;
  int stored_devices;
  int loop_device, loop_bytes;
  boolean match;
  boolean found_new = false;
  char device_description[20];
  int no_chars_from_term;
  
  int last_device;
  byte Rom_no[8] = {0,0,0,0,0,0,0,0};
  byte New_Rom_no[8] = {0,0,0,0,0,0,0,0};
  boolean proceed = true;
 
   //Read number of devices
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  if (stored_devices < 50){
    last_device = 0;
    //Do until the last device connected to the bus is found by search_rom().
    do {
      if (!ds18b20.initialize()){  //Must initialize before finding each device.
        last_device = ds18b20.search_rom(Rom_no, New_Rom_no, last_device);      
        match = false;
        //loop for each device in EEPROM. If a match is found, exit the loop
        for (loop_device = 0; (loop_device != stored_devices && !match); loop_device++){
          address = 20 * loop_device + 4;  //jump to first byte of device in EEPROM
          match = true;
          //loop for each byte.  Compares byte in EEPROM with byte in New_Rom_no.
          //Exits if a byte does not match. and makes match false. 
          for (loop_bytes = 0; (loop_bytes < 8 && match); loop_bytes++){
            if (New_Rom_no[loop_bytes] != eeprom.EEPROM_read(address)){
              match = false;  // A byte does not match
            }
            address++;  //get next byte
          }  
        }
      }
      else{
        Serial.println("Initialization Failure on ROM Read"); 
        proceed = false; 
      }  
      //if there was no Initialization failure and the ROM code did not match
      //any already existing in the EEPROM, a CRC check on the new ROM code will
      //be done, and the user will be aksed to enter a description.
      if (proceed){
        if (!match){   //We have found a new device
          found_new = true;
          Serial.println("A new device has been found.");
          //We'll do a CRC check first
          if (ds18b20.calculateCRC_byte(New_Rom_no, 8)){
            Serial.println("Device ROM failed CRC");
          }
          else{
            //Storing the Rom Code into the next device location in EEPROM
            address = 20 * stored_devices + 4;  
            for (i = 0; i < 8; i++){
              eeprom.EEPROM_write(address, New_Rom_no[i]);
              address += 1;
            }
            //Asking for a description
            Serial.println("Device ROM passed CRC");
            Serial.println("Please enter a description. 12 Characters Maximum:");
            while (!Serial.available() > 0);
            no_chars_from_term = Serial.readBytesUntil('\n', device_description, 13);
            
            //Storing the new description in EEPROM 
            for (int i = 0; i < (no_chars_from_term); i++){
              eeprom.EEPROM_write(address, device_description[i]);
              address += 1;
            }
            if (no_chars_from_term < 12){
              eeprom.EEPROM_write(address, 255); 
            }  
            
            //update the number of devices in the EEPROM at address 3
            stored_devices++;
            eeprom.EEPROM_write(3, stored_devices);
            
            //Report to user info about new device
            Serial.print("\tThe new device is device number: ");
            Serial.println(stored_devices);
            Serial.print("\tIt's ROM code is: ");
            for (int i = 0; i < 8; i++){
              Serial.print(New_Rom_no[i], HEX);
              Serial.print(" ");
            }
            Serial.println("");
            Serial.print("\tWith this description: ");
            for (i = 0; i < no_chars_from_term; i++){
              Serial.print(device_description[i]);
            }
            Serial.println("\n"); 
          }   
        }
      }  
      //The just found ROM code is copied to Rom_no which is passed to search rom
      for (i = 0; i < 8; i++){
        Rom_no[i] = New_Rom_no[i];
        New_Rom_no[i] = 0;
      }
    //search_rom() will return last_device as 0 when last device on bus has been found
    }while (last_device && proceed); 
    if(!found_new){
      Serial.println("Did not find a new device");
    }
    else{
      Serial.println("Done");
    }
  }
  else {
    Serial.println("You already have 50 devices stored in EEPROM - No more room");
  }  
}

  

void setup() {
  Serial.begin(9600);

  new_device_to_EEPROM();
}

void loop() {
}
