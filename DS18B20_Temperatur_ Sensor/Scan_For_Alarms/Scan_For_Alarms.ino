/*  Scan_For_Alarm.ino  thepiandi@blogspot.com      MJL  062714

1.  Uses the DS18B20 Alarm Search command to find the ROM code of each device whose 
temperature is higher than the upper temperature limit or lower that the lower 
termperature limit.
2.  Before running the Alarm Search command it is necessary to determine if any device
on the bus is connected using parasitic power.  This is done by issuing a Skip Rom command
followe by a Read Power Supply command.  Then, a Convert_t command preceeded by a Skip Rom 
command is run.  The operation of the Convert_t command is altered if any device is 
connected using parasitic power.  Skip Rom command will result in all 
devices on the bus preforming the convert function. 
3.  If the ROM code returned is 00000000, temperature measured by all devices are
within the set limits.
4.  If a ROM code of a connected device is returned, that device is out of tolerance.
Temperature measurement is made and the results displayed along with the device's
description, the upper and lower temperature limits, and the measurment resolutuin,
devices are connected using parasitic power.
5.  The last step is repeated if more than one device has its temoerature out of
bounds.
6.  Everytime a ROM code is returned or the scratchpad read, a CRC is performed.
*/

#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;

void alarms(){
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
  boolean parasitic = false;
  boolean parasitic_all_devices = false;
  int character;
  byte scratchpad[9];  
  float temp_c, temp_f;

  //Read number of devices
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }

  //Very first operation is to see if any device is connected using parasitic power
  if (ds18b20.initialize()){
    Serial.println("Initialization Failure on Read_Power_Supply");
    proceed = false;
  }
  if (proceed){
    ds18b20.skip_rom();
    if (!ds18b20.read_power_supply()){ //will return 0 if any parasitic
      parasitic_all_devices = true;
    }
    //Command skip_rom() followed by Convert_t()
    if (ds18b20.initialize()){
      Serial.println("Initialization Failure on Skip ROM");
      proceed = false;
    }
  }
  //Do a convert t
  if (proceed){
    ds18b20.skip_rom();
    ds18b20.convert_t(parasitic_all_devices, 12);

    //Do until the last device with an alarm flag set is found by alarm_search().
    //If no device has its alarm flag set, New_Rom_no[] will return as 00000000.
    last_device = 0;
    do {
      if (ds18b20.initialize()){  //Must initialize before finding each device.
        Serial.println("Initialization Failure on Alarm Scan");
        proceed = false;
      }
      if (proceed){
        //New_Rom_no will have the ROM code for the device returned
        last_device = ds18b20.alarm_search(Rom_no, New_Rom_no, last_device);

        Serial.println("");
        //Test to see if there are no device that has its alarm flag set
        if (New_Rom_no[0] == 0){
          Serial.println("All Devices Are Within Set Temperature Limits");
          proceed = false;
        }
      }
      
      if (proceed){
        //If we got this far there is at least one device with its alarm flag set:
        //The just found ROM code is copied to Rom_no which is passed to alarm_search()
        //to possibly find another device woth an alarm flag set
    
        //Now that we know that a device has its alarm flag set, we will use the ROM
        //code found in New_Rom_no[] to search the ATmega EEPROM to find the device's 
        //description. We will search the device's scratchpad for the temperature limits
        //and its resolution. 
        //Next we will do a temperature measurement of the device and print out
        //this measurement along with the temperature upper and lower limit and resolution.
        
        //We'll do a CRC check on the ROM code
        if (ds18b20.calculateCRC_byte(New_Rom_no, 8)){
          Serial.println("Device ROM failed CRC");
          proceed = false;
        }        
      }
      if (proceed){

          //loop for each device in EEPROM. When a match is found, exit the loop
          match = false;
          for (loop_device = 0; (loop_device != stored_devices && !match); loop_device++){
            address = 20 * loop_device + 4;  //jump to first byte of device in EEPROM
            match = true;
            //loop for each byte.  Compares byte in EEPROM with byte in New_Rom_no.
            //Exits if a byte does not match. and makes match false. 
            for (loop_bytes = 0; (loop_bytes < 8 && match); loop_bytes++){
              if (New_Rom_no[loop_bytes] != eeprom.EEPROM_read(address)){
                match = false;  // A byte does not match try the next device
              }
              address++;  //get next byte
            }  
          }
          if (!match){
            Serial.println("Could not find device in EEPROM");
            proceed = false;
          }
      }
      if (proceed){
        Serial.print("The Device With Temperature Out Of Limits is: ");
        character = 0;
        for (int i = 0; (i < 12 && character != 255); i++){
          character = eeprom.EEPROM_read(address +i);
          if (character != 255){
            Serial.print(char(character));
          }
        }
        Serial.println("");
        //Going to make a temperature measurement  
        //First if variable parasitic_all_devices is true then check the current device
        //for parasitic operation
        if (parasitic_all_devices){            
          if (ds18b20.initialize()){
            Serial.println("Initialization Failure on read power supply");
            proceed = false;
          }
          if (proceed){
            ds18b20.match_rom(New_Rom_no); 
            if (!ds18b20.read_power_supply()){  //A zero means parasitic
              parasitic = true;
              Serial.println("\nDevice is connected using parasitic power");
            }  
            else{
              Serial.println("\nDevice is not connected using parasitic power");
            }    
            Serial.println("");
          }
        }
        
        //Match ROM followed by read Scratchpad for resolution and alarm temperatures
        if (ds18b20.initialize()){
          Serial.println("Initialization Failure on Match ROM");
          proceed = false;
        }
      }
      if (proceed){
        ds18b20.match_rom(New_Rom_no); 
        //Read scratchpad and check CRC
        ds18b20.read_scratchpad(scratchpad); 
        if (ds18b20.calculateCRC_byte(scratchpad, 9)){
          Serial.println("Scratchpad failed CRC");
          proceed = false;
        }     
      }
      if (proceed){
        //Retreive high temperature alarm
        if (scratchpad[2] > 127){
          high_alarm = float(scratchpad[2] - 256) * 1.8 + 32.0;  //convert to deg F
        }
        else{
          high_alarm = float(scratchpad[2]) * 1.8 + 32.0;  //convert to deg F
        }
        //Retreive low temperature alarm
        if (scratchpad[3] > 127){
          low_alarm = float(scratchpad[3] - 256) * 1.8 + 32.0;  //convert to deg F
        }
        else{
          low_alarm = float(scratchpad[3]) * 1.8 + 32.0;  //convert to deg F
        }
        //Retreive low temperature alarm            
        resolution = scratchpad[4];
        resolution = (resolution >> 5) + 9;
        
        //Match ROM followed by Convert T if Initialization Passes
        if (ds18b20.initialize()){
          Serial.println("Initialization Failure on Convert Temperature");
          proceed = false;
        }
      }
      //Do convert_t on device
      if (proceed){
        ds18b20.match_rom(New_Rom_no);
        ds18b20.convert_t(parasitic, resolution);
        
        //Match ROM followed by Read Scratchpad
        if (ds18b20.initialize()){
          Serial.println("Initialization Failure on Match Rom"); 
          proceed = false;
        }
      }   
      if (proceed){
        ds18b20.match_rom(New_Rom_no);
        ds18b20.read_scratchpad(scratchpad);
                  
        //Check scratchpad CRC
        if (ds18b20.calculateCRC_byte(scratchpad, 9)){
          Serial.println("CRC Error On Scratchpad Read"); 
          proceed = false; 
        }
      }
      if (proceed){
        //Calculate Temperature and Print Results
        temp_c = (256 * scratchpad[1] + scratchpad[0]);
        if (scratchpad[1] > 127){  //Temperature below zero
          temp_c -= 65536;          //calculates 2's complement
        }
        temp_c /= 16.0;
        temp_f = 1.8 * temp_c + 32.0;
    \                  
        Serial.print("\tTemperature: ");
        Serial.print(temp_f);
        Serial.println(" deg F");
        Serial.print("\tUpper Temperature Limit: ");
        Serial.print(high_alarm);
        Serial.println(" deg F");
        Serial.print("\tLower Temperature Limit: ");
        Serial.print(low_alarm);
        Serial.println(" deg F");
        Serial.print("\tResolution: ");
        Serial.print(resolution);
        Serial.println(" bits");
  
        //Final operation is to copy New_Rom_no to Rom_no and zero New_Rom_no to
        //prepare for next alarm scan
        for (i = 0; i < 8; i++){
          Rom_no[i] = New_Rom_no[i];
          New_Rom_no[i] = 0;
        }
      }    
    }while (last_device && proceed);
  }
  
}

void setup() {
  Serial.begin(9600);
  alarms();


}

void loop() {
}
