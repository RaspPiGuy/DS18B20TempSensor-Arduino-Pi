/*  Find_new_device.ino    thepiandi@blogspot.com       MJL  062114

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
#include "InputFromTerminal.h"

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;

/*---------------------------------------------how_many-------------------------------------*/
long how_many(long lower_limit, long upper_limit){
  long menu_choice;
  boolean good_choice;

  good_choice = false;
  do{
    menu_choice = term_input.termInt();
    if (menu_choice >= lower_limit && menu_choice <= upper_limit){
      good_choice = true;
    }
    else{
      Serial.println("Please Try Again");
    }
  }while (!good_choice);

  return menu_choice; 
}

/*---------------------------------------------new_device_to_EEPROM--------------------------------*/
void new_device_to_EEPROM(){
  int i, address;
  int stored_devices;
  int loop_device, loop_bytes;
  boolean match;
  boolean found_new = false;
  char device_description[20];
  int no_chars_from_term;
  int high_alarm, low_alarm, resolution;
  
  int last_device;
  byte Rom_no[8] = {0,0,0,0,0,0,0,0};
  byte New_Rom_no[8] = {0,0,0,0,0,0,0,0};
  byte alarm_and_config[3];
  boolean proceed = true;
  boolean parasitic;
 
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
            /*This is where we store ROM code and description into the ATmega EEPROM and
            we store upper and lower alarm temperatures and configuration data
            into DS18B20's scratch pad.  Configuration data is the resolution */
            
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
            
            //We ask user the upper and lower temperature alarms
            Serial.print("Please enter the upper alarm temperature +257 > -67, deg F: ");
            high_alarm = how_many(-67,257);
            Serial.print(high_alarm);
            Serial.println(" deg F");
            alarm_and_config[0] = (float(high_alarm) - 32.0) / 1.8; //convert to deg C            
            if (alarm_and_config[0] < 0){
              alarm_and_config[0] -= 256;  //makes 2's complement
            }
           
            Serial.print("Please enter the lower alarm temperature +257 > -67, deg F: ");
            low_alarm = how_many(-67,257);
            Serial.print(low_alarm);
            Serial.println(" deg F");
            alarm_and_config[1] = (float(low_alarm) - 32.0) / 1.8; //convert to deg C            
            if (alarm_and_config[1] < 0){
              alarm_and_config[1] -= 256;    //makes 2's complement
            }
            
            //We ask user the resolution
            Serial.print("Please resolution, 9 - 12 bits: ");
            resolution = how_many(9, 12);
            Serial.print(resolution);
            Serial.println(" bits");
            alarm_and_config[2] = 0x1F + 0x10 * 2 * (byte(resolution) - 9);  
            
            //Write upper alarm, lower alarm, and configuration (resolution) to scratchpad            
            //Match ROM followed by Write 3 bytes to ScratchPad
            if (ds18b20.initialize()){
              Serial.println("Initialization Failure on Match ROM");
            }
            else{
              //Match ROM followed by Write Scratchpad with resolution and alarms
              ds18b20.match_rom(New_Rom_no); 
              ds18b20.write_scratchpad(alarm_and_config); 
            }
            
            //In prepation for writing scratchpad to DS18B20's EEPROM
            //will fimd out if it connected using parasitic power
            if (ds18b20.initialize()){
              Serial.println("Initialization Failure");
            }
            else{
              ds18b20.match_rom(New_Rom_no); 
              parasitic = false;
              if (!ds18b20.read_power_supply()){  //A zero means parasitic
                parasitic = true;
              }
            }
            
            //Now we copy the scrathpad to the DS18B20's EEPROM
            if (ds18b20.initialize()){
              Serial.println("Initialization Failure");
            }
            else{
              ds18b20.match_rom(New_Rom_no); 
              ds18b20.copy_scratchpad(parasitic);
            }
            
            //Report to user info about the new device
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
            Serial.println("");
            
            Serial.print("\tUpper alarm set to: ");
            Serial.print(high_alarm);
            Serial.print(" deg F   Lower alarm set to: ");
            Serial.print(low_alarm);
            Serial.println(" deg F");
            Serial.print("\tResolution: ");
            Serial.print(resolution);
            Serial.println(" bits");
            
            if (parasitic){
              Serial.println("\tDevice is connected using parasitic power\n");
            }
            else{
              Serial.println("\tDevice is NOT connected using parasitic power\n");             
            }
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
