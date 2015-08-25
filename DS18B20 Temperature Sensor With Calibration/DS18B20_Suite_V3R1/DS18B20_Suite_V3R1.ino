
/*  DS18B20_Suite.ino      thepiandi.blogspot.com     MJL August 25, 2015


Provides interface to DS18B20 temperature sensor and to the AYmega328P EEPROM. Here are the services this
menu driven sketch provides:
  Read_Temperature - reads temperature from selected, connected, sensors with options
  Scan_For_Alarm - scans connected sensors to see if any are measuring out of tolerance temperatues
  Find_new_device - detects new sensors. User adds descriptionl resolution, and alarm settings
  Edit_Stored_Information - Allows user to mmodify description, resolution, and alarm settings
  Devices_On_The_Bus - reports the ROM code of all connected sensors
  Devices_In_EEPROM - reports description and calibration of all sensors in the EEPROM
  Remove_Device_From_EEPROM - removes sensor from the E3EPROM
  
The ATmega328P's 1024 byte EEPROM is used to store data about the sensors.  The first three bytes 
(bytes - 0 2) are reserved for future use.  The forth byte (byte 3) contains the number of sensors 
listed in the EEPROM.  Following byte 3 is 22 bytes for each sensor. The first 8 bytes are the ROM
code, and the next 12 bytes make up the description. The 21st. byte is the low temperature calibration
factor (0 degrees C.), while the last byte is the high temperature calibration factor (100 degree C.).

Each DS18B20 contains a scrathpad memory.  The first two bytes are for the temperature measurements. 
The next three bytes can be written to by the user and consist of the upper and lower temperature
alarm and the resolution (9, 10, 11, or 12 bits).

Version 2:  Added the temperature corrections for the calibration factors.  Was necessary to account
  for the increase of the bytes in EEPROM, for each sensor, from 20 to 22.  - August 21, 2015
  
Version 3:  Accounts for the fact that all sensors in the EEPROM may not be connected.  Only asks
  the user to choose connected devices.  Improved the Edit EEPROM function by allowing user to
  exit without saving changes. - August 24, 2015

  
*/

#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;

boolean connected_devices[50];
int number_of_connected_devices;

/*_____________________________________FUCTIONS COMMON TO ALL FUNCTIONS______________________________*/

/*---------------------------------------------find_stored_devices-------------------------------------*/
int find_stored_devices(){
  int stored_devices, address, character;
  byte rom[8];
  byte scratchpad[9];
  int i, j, k;
 
  //How Many devices stored in the EEPROM?
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  number_of_connected_devices = stored_devices;
  
  for (i = 0; i < stored_devices; i++){
    connected_devices[i] = true;
  }  
  
  if(stored_devices){
    //List the devices Srored in EEPROM
    for (i = 0; i < stored_devices; i++){
      address = 22 * i + 4; 
      
      //Get ROM code from EEPROM
      for (j = 0; j < 8; j++){  //get the ROM code
        rom[j] = eeprom.EEPROM_read(address + j);
      } 
     
      //Get and print description
      address += 8;
      
      Serial.print(F("Device No: "));
      Serial.print(i + 1);
      Serial.print(F("  Description: "));
   
      character = 0;
      for (j = 0; (j < 12 && character != 255); j++){
        character = eeprom.EEPROM_read(address +j);
        if (character != 255){
          Serial.print(char(character));
        }
      }
      
      //find if device is connected to enclosure
      ds18b20.initialize();
      ds18b20.match_rom(rom);      
      ds18b20.read_scratchpad(scratchpad); 
 
      // check CRC, if true. CRC failed and device in not connected to enclosure
      if (ds18b20.calculateCRC_byte(scratchpad, 9)){
        connected_devices[i] = false;
        number_of_connected_devices--;
        Serial.print(F("\tDevice is NOT Connected"));
      }
      else{
       Serial.print(F("\tDevice is Connected"));
      }
      
      Serial.println(F(""));  
      address += 22;
    }
    Serial.println(F(""));
  }
  else{
    Serial.println(F("There are no devices stored in the EEPROM"));
  }
  
  return stored_devices;
}

/*---------------------------------------------yes_or_no-------------------------------------*/
boolean yes_or_no(){
  boolean yes_no = false;
  char keyboard_entry[5];
  do{
    Serial.println(F("Please enter Y or y, or N or n"));        
    while (!Serial.available() > 0);
    Serial.readBytesUntil('\n',keyboard_entry, 5);
    if (keyboard_entry[0] == 'Y' || keyboard_entry[0] == 'y'){
      yes_no = true;
    }
  }while((keyboard_entry[0] != 'y') && (keyboard_entry[0] != 'Y') && (keyboard_entry[0] != 'n') && (keyboard_entry[0] != 'N'));
  return yes_no;
}

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
      Serial.println(F("Please Try Again"));
    }
  }while (!good_choice);

  return menu_choice; 
}

/*---------------------------------------------apply_calibratiom-----------------------------------*/
float apply_calibration(byte cal_hi, byte cal_lo, float measured){
  float corrected_temp, correction_hi, correction_lo;
  int cal_hi_int, cal_lo_int;
  
  //get 100 degree C. temperature correction
  cal_hi_int = cal_hi;
  
  //check if negative
  if (cal_hi_int > 128){
    cal_hi_int += 65280;
  }
  correction_hi = cal_hi_int / 16.0;
  
  //get 0 degree C. temperature correction
  cal_lo_int = cal_lo;
  
  //check if negative
  if (cal_lo_int > 128){
    cal_lo_int += 65280;
  }
  correction_lo = cal_lo_int / 16.0;
  
  corrected_temp = measured - (measured * ( correction_hi - correction_lo ) / 100.00 + correction_lo);
  
  return corrected_temp;
}


/*________________________________________READ TEMPERATURE_________________________________________*/

/*  Read_Temperatures.ino    thepiandi@blogspot.com       MJL  062914

Everything but the kitchen sink program to read temperatures from the DS18B20 device.  Program:
1.    Displays the description and device number of all the devices in EEPROM
2.    Asks user if all devices are to be run.
2.a.  If not, displays each device, in turn, and asks if this device will be run
3.    Asks how many measurements are to be run
4.    Asks what the delay berween measurements is to be.  Asks hours, minutes, seconds, milliseconds
5.    Asks how many bits of resolution 9 - 12 possible
6.    Asks if it OK to run.  If not, program returns to step 2. above.
7.    Determines if any device is connected using parasitic power.

8.    Runs two loops, outer loop for each measurement, and the inner loop, for each device to be run.
9.    For each measurement retrieves ROM code from EEPROM and runs the following DS18B20 commands:
9.    Initialize, match rom, convert t, read scratchpad, CRC on scratchpad.
10.   Temperature is calculated from scratch pad and displayed in deg F.
11.   If user wishes, the upper and lower temperature limits and resolution are displayed
11.   When the run is completed, the average is displayed for each device run.
12.   Other parameters and diagnostic information is displayed.

Uses the One_wire_DS18B20 library functions for basic one wire operation.
The EEPROM_Functions library is used to read the device's ROM code and description from EEPROM.
InputFromTerminal library is used to enter integer values from the keyboard.
*/

/*---------------------------------------------read_temperature-------------------------------------*/
void read_temperature(){
  byte rom[8];
  byte scratchpad[9];
  int description[12];
  byte alarm_and_config[3] = {0x7D, 0xC9, 0x7F};
  float temp_c, corrected_temp_c, temp_f;
  int address;
  int i, j, k;
  int conversion_time; 
  long which_device, delay_between_measurements, resolution; 
  long measurements = 0; 
  int make_run;
  int init_failures = 0;
  int crc_errors = 0;
  int number_of_devices_to_run;
  int hours, minutes, seconds, milliseconds;
  boolean parasitic = false;
  int high_alarm, low_alarm;
  boolean display_scratchpad;
  int spaces;
  int character;
  byte cal_factor_lo, cal_factor_hi;
  boolean all_or_one = false;
  
  int stored_devices = find_stored_devices();
  boolean devices2run[stored_devices];
  float averages[stored_devices];

  number_of_devices_to_run = number_of_connected_devices;
  
  //exit if there are no connected devices
  if (!number_of_connected_devices){
    Serial.println(F("No devices connected"));
    delay(1000);
    return;
  }
   
  //Interface with user. Keep in loop until user is satisfied with choices
  do{
    //Set up array for devices to run  
    for (i = 0; i < stored_devices; i++){
      devices2run[i] = connected_devices[i];
      averages[i] = 0;
    }
    
    //If only one connected device don't ask to run all devices
    if (number_of_connected_devices > 1){
      Serial.println(F("Run all devices? "));
     
      //Ask if all devices are to be run.  If No, query each device and determine
      //if the selected device is connected using parasitic power.
      if (!yes_or_no()){  //If no is selected this will be true

        //no was selected so query each device
        for (i = 0; i < stored_devices; i++){
          if (devices2run[i]){
            Serial.print(F("\nRun device number "));  
            Serial.print(i + 1);
            Serial.println(F("?"));
       
            if (!yes_or_no()){  //If answer is no, make array entry for device false
              devices2run[i] = false;
              number_of_devices_to_run--;
            } 
            else{  //Determine if it is connected using parasitic power
              if (ds18b20.initialize()){
                Serial.println(F("Initialization Failure"));
              }
              else{
                address = 22 * i +4;   //Get ROM code from EEPROM
                for (k = 0; k < 8; k++){   
                  rom[k] = eeprom.EEPROM_read(address +k);
                } 
                ds18b20.match_rom(rom); 
                if (!ds18b20.read_power_supply()){  //A zero means parasitic
                  parasitic = true;
                }
              }
            }
          }
        }
      }
      
      //selected to run all devices
      else{
        all_or_one = true;
      }
    }
     
    //only one device
    else{
      all_or_one = true;
    }
    
    //do if only one or all devices
    if (all_or_one){
      if (ds18b20.initialize()){
        Serial.println(F("Initialization Failure on Read_Power_Supply"));
      }
      else{
        ds18b20.skip_rom();
        if (!ds18b20.read_power_supply()){
          parasitic = true;
        }
      }     
    }
    
    if (parasitic){
      Serial.println(F("\nA device is connected using parasitic power"));
    }  
    else{
      Serial.println(F("\nNo device is connected using parasitic power"));
    }    
    Serial.println(F(""));

    //Ask number of measurements, time between measurements
    Serial.print(F("How many measurements would you like to make? "));
    measurements = how_many(1, 10000000);
    Serial.println(measurements);

    //If we only make one measurement we will not ask for time between measurements
    if (measurements > 1){  
      Serial.print(F("\nHow much time between measurements in hours? "));
      hours = how_many(0, 10000000);
      Serial.println(hours);
      
      Serial.print(F("How much time between measurements in minutes? "));
      minutes = how_many(0, 60);
      Serial.println(minutes);
      
      Serial.print(F("How much time between measurements in seconds? "));
      seconds = how_many(0, 60);
      Serial.println(seconds);
      
      Serial.print(F("How much time between measurements in ms.? "));
      milliseconds = how_many(0, 1000);
      Serial.println(milliseconds);
      
      delay_between_measurements = hours * 60 *60 *1000 + minutes * 60 * 1000 + seconds * 1000 + milliseconds; 
    }
    else{
      delay_between_measurements = 0;
    } 
  
    Serial.print(F("\nDisplay alarm temperatures and resolution? "));
    display_scratchpad = yes_or_no();
    Serial.println(F(""));
    
    //check if at least one device has been selected
    if (number_of_devices_to_run){
      Serial.println(F("Select 1 to Make Run, 2 to Edit Choices, 3 to Exit "));
      make_run = how_many(1, 3);
      if (make_run == 3){
        return;
      }
      else if (make_run == 2){
        Serial.println(F(""));
      } 
    }
    else{
      Serial.println(F("Select at least one device, please\n\n"));
      delay(1000);
      number_of_devices_to_run = number_of_connected_devices;
      make_run = 2;
    }      
  }while (make_run == 2);
  Serial.println(F(""));  
  
  //finally ready to make measurements 
  //Outer measurement loop
  //Run for each measurement
  for (i = 0; i < measurements; i++){  
    Serial.print(F("Measurement: "));
    Serial.println(i + 1);
    //Inner Loop
    //Run for each device
    for (j = 0; j < stored_devices; j++){  
      if (devices2run[j]){  //check to see if device is to be run
        address = 22 * j +4;   //Get ROM code from EEPROM
        for (k = 0; k < 8; k++){
          rom[k] = eeprom.EEPROM_read(address + k);
        } 
        //Get Descriotion
        address += 8;
        for (k = 0; k < 12; k++){  //Get description
          description[k] = eeprom.EEPROM_read(address + k);
        }
        
        //Get upper and lower calibration factors
        address += 12;
        cal_factor_lo = eeprom.EEPROM_read(address);
        cal_factor_hi = eeprom.EEPROM_read(address + 1);
        
        //debug:
        //Serial.println(cal_factor_lo);
        //Serial.println(cal_factor_hi);
                     
        //Match ROM followed by read Scratchpad for resolution and alarm temperatures
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure on Match ROM"));
        }
        else{
          ds18b20.match_rom(rom); 
          //Read scratchpad and check CRC
          ds18b20.read_scratchpad(scratchpad); 
          if (ds18b20.calculateCRC_byte(scratchpad, 9)){
            Serial.println(F("Scratchpad failed CRC"));
            crc_errors++;
          }
          else{
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
            //Retreive resolution            
            resolution = scratchpad[4];
            resolution = (resolution >> 5) + 9;
          }
          
          //Match ROM followed by Convert T if Initialization Passes
          if (ds18b20.initialize()){
            Serial.println(F("Initialization Failure on Convert Temperature"));
          }
          else{
            //Print Device Description
            spaces = 14;
            Serial.print(F("  "));
            for (int k = 0; (k < 12 && description[k] != 255); k++){
              Serial.print(char(description[k]));
              spaces--;
            }
            //Print spaces after description
            for (k= 0; k < spaces; k++){
              Serial.print(F(" "));
            }
                        
            //Match ROM followed by Convert T
            if (ds18b20.initialize()) init_failures++; 
            ds18b20.match_rom(rom);
            ds18b20.convert_t(parasitic, resolution);
            
            //Match ROM followed by Read Scratchpad
            if (ds18b20.initialize()) init_failures++; 
            ds18b20.match_rom(rom);
            ds18b20.read_scratchpad(scratchpad);
            
            //Check scratchpad CRC
            if (ds18b20.calculateCRC_byte(scratchpad, 9)) crc_errors++;
            
            //Calculate Temperature and Print Results
            temp_c = (256 * scratchpad[1] + scratchpad[0]);
            if (scratchpad[1] > 127){  //Temperature below zero
              temp_c -= 65536;          //calculates 2's complement
            }
            temp_c /= 16.0;
            
            corrected_temp_c = apply_calibration(cal_factor_hi, cal_factor_lo, temp_c);
            
            //debug:
            //Serial.println(temp_c);
            //Serial.println(corrected_temp_c);
            
            
            temp_f = 1.8 * corrected_temp_c + 32.0;
            averages[j] += temp_f;
            
            Serial.print(F("  Temp.: "));
            Serial.print(temp_f);
            Serial.print(F(" deg F"));
            if (display_scratchpad){
              Serial.print(F("  Upper: "));
              Serial.print(high_alarm);
              Serial.print(F(" deg F  Lower: "));
              Serial.print(low_alarm);
              Serial.print(F(" deg F  Res.: "));
              Serial.print(resolution);
              Serial.println(F(" bits"));
            }
            else{
              Serial.println(F(""));
            }
          } 
        }
      }
    }
    if (i != (measurements -1)){
      delay(delay_between_measurements); 
    }
  }
  Serial.println(F(""));  
  
  //Finish up. Print some results
  //Print Averages Temperatures
  if (measurements > 1){
    for (i = 0; i < stored_devices; i++){ 
      if (devices2run[i]){
        Serial.print(F("Average for Device - "));
        spaces = 14;
        address = 22 * i + 12;
        character = 0;
        for (int j = 0; (j < 12 && character != 255); j++){
          character = eeprom.EEPROM_read(address +j);
          if (character != 255){
            Serial.print(char(character));
            spaces--;
          }
        }
        Serial.print(F(":"));
        for (k = 0; k < spaces; k++) Serial.print(F(" "));
        Serial.print(averages[i] / measurements);
        Serial.println(F(" deg F"));      
      }
    }
    Serial.println(F(""));
  }
  Serial.print(F("Parasitic Operation? "));
  if (parasitic) Serial.println(F("Yes"));
  else Serial.println(F("No"));
  Serial.print(F("Number of measurements: "));
  Serial.println(measurements);
  Serial.print(F("Delay Between Measurements, ms.: "));
  Serial.println(delay_between_measurements);  
  Serial.print(F("Number of initialization failures: "));
  Serial.println(init_failures);
  Serial.print(F("CRC failures: "));
  Serial.println(crc_errors);
  Serial.println(F(""));         
}

/*______________________________________SCAN FOR ALARMS__________________________________*/

/*  Scan_For_Alarm.ino  thepiandi@blogspot.com      MJL  070214

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


void alarms(){
  int i, address;
  int stored_devices;
  int loop_device, loop_bytes;
  boolean match;
  boolean found_new = false;
  char device_description[13];
  int no_chars_from_term;
  int high_alarm, low_alarm, resolution;
  
  int last_device;
  byte Rom_no[8] = {0,0,0,0,0,0,0,0};
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
    Serial.println(F("Initialization Failure on Read_Power_Supply"));
    proceed = false;
  }
  if (proceed){
    ds18b20.skip_rom();
    if (!ds18b20.read_power_supply()){ //will return 0 if any parasitic
      parasitic_all_devices = true;
    }
    if (ds18b20.initialize()){
      Serial.println(F("Initialization Failure on Skip ROM"));
      proceed = false;
    }
  }
  
  //Command skip_rom() followed by Convert_t()
  if (proceed){
    ds18b20.skip_rom();
    ds18b20.convert_t(parasitic_all_devices, 12);

    //Do until the last device with an alarm flag set is found by alarm_search().
    //If no device has its alarm flag set, Rom_no[] will return as 00000000.
    last_device = 0;
    do {
      if (ds18b20.initialize()){  //Must initialize before finding each device.
        Serial.println(F("Initialization Failure on Alarm Scan"));
        proceed = false;
      }
      if (proceed){
        //Rom_no will have the ROM code for the device returned
        last_device = ds18b20.alarm_search(Rom_no, last_device);

        Serial.println(F(""));
        //Test to see if there are no device that has its alarm flag set
        if (Rom_no[0] == 0){
          Serial.println(F("All Devices Are Within Set Temperature Limits"));
          proceed = false;
        }
      }
      
      if (proceed){
        /*
          If we got this far there is at least one device with its alarm flag set:
          
          The ROM code that was just found is copied to Rom_no which is passed to alarm_search()
        to possibly find another device w1th an alarm flag set
    
          Now that we know that a device has its alarm flag set, we will use the ROM
        code found in Rom_no[] to search the ATmega EEPROM to find the device's 
        description and calibration factors. 
        
          We will search the device's scratchpad for the temperature limits and its resolution. 
          
          Next we will do a temperature measurement of the device, apply cal. factors and print out
        this measurement along with the temperature upper and lower limit and resolution.
        */
        
        //We'll do a CRC check on the ROM code
        if (ds18b20.calculateCRC_byte(Rom_no, 8)){
          Serial.println(F("Device ROM failed CRC"));
          proceed = false;
        }        
      }
      if (proceed){

          //loop for each device in EEPROM. When a match is found, exit the loop
          match = false;
          for (loop_device = 0; (loop_device != stored_devices && !match); loop_device++){
            address = 22 * loop_device + 4;  //jump to first byte of device in EEPROM
            match = true;
            //loop for each byte.  Compares byte in EEPROM with byte in Rom_no.
            //Exits if a byte does not match. and makes match false. 
            for (loop_bytes = 0; (loop_bytes < 8 && match); loop_bytes++){
              if (Rom_no[loop_bytes] != eeprom.EEPROM_read(address)){
                match = false;  // A byte does not match try the next device
              }
              address++;  //get next byte
            }  
          }
          if (!match){
            Serial.println(F("Could not find device in EEPROM"));
            proceed = false;
          }
      }
      if (proceed){
        Serial.print(F("The Device With Temperature Out Of Limits is: "));
        character = 0;
        for (int i = 0; (i < 12 && character != 255); i++){
          character = eeprom.EEPROM_read(address +i);
          if (character != 255){
            Serial.print(char(character));
          }
        }
        Serial.println(F(""));
        //Going to make a temperature measurement  
        //First if variable parasitic_all_devices is true then check the current device
        //for parasitic operation
        if (parasitic_all_devices){            
          if (ds18b20.initialize()){
            Serial.println(F("Initialization Failure on read power supply"));
            proceed = false;
          }
          if (proceed){
            ds18b20.match_rom(Rom_no); 
            if (!ds18b20.read_power_supply()){  //A zero means parasitic
              parasitic = true;
              Serial.println(F("\nDevice is connected using parasitic power"));
            }  
            else{
              Serial.println(F("\nDevice is not connected using parasitic power"));
            }    
            Serial.println(F(""));
          }
        }
        
        //Match ROM followed by read Scratchpad for resolution and alarm temperatures
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure on Match ROM"));
          proceed = false;
        }
      }
      if (proceed){
        ds18b20.match_rom(Rom_no); 
        //Read scratchpad and check CRC
        ds18b20.read_scratchpad(scratchpad); 
        if (ds18b20.calculateCRC_byte(scratchpad, 9)){
          Serial.println(F("Scratchpad failed CRC"));
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
        //Retreive resolution            
        resolution = scratchpad[4];
        resolution = (resolution >> 5) + 9;
        
        //Match ROM followed by Convert T if Initialization Passes
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure on Convert Temperature"));
          proceed = false;
        }
      }
      //Do convert_t on device
      if (proceed){
        ds18b20.match_rom(Rom_no);
        ds18b20.convert_t(parasitic, resolution);
        
        //Match ROM followed by Read Scratchpad
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure on Match Rom")); 
          proceed = false;
        }
      }   
      if (proceed){
        ds18b20.match_rom(Rom_no);
        ds18b20.read_scratchpad(scratchpad);
                  
        //Check scratchpad CRC
        if (ds18b20.calculateCRC_byte(scratchpad, 9)){
          Serial.println(F("CRC Error On Scratchpad Read")); 
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
        Serial.print(F("\tTemperature: "));
        Serial.print(temp_f);
        Serial.println(F(" deg F"));
        Serial.print(F("\tUpper Temperature Limit: "));
        Serial.print(high_alarm);
        Serial.println(F(" deg F"));
        Serial.print(F("\tLower Temperature Limit: "));
        Serial.print(low_alarm);
        Serial.println(F(" deg F"));
        Serial.print(F("\tResolution: "));
        Serial.print(resolution);
        Serial.println(F(" bits"));
  
      }    
    }while (last_device && proceed);
  }
  
}
/*______________________________________FIND NEW DEVICE__________________________________*/

/*  Find_new_device.ino    thepiandi@blogspot.com       MJL  070214

1.  Uses the DS18B20 Search rom command to find the ROM code of each device that it finds on the bus.  
2.  Once a new device is found, it's ROM code is displayed
3.  A CRC check is made on the ROM code and the results of the CRC is displayed.
4.  The user is asked to input a description.
5.  The device number, ROM code, and the description is written to the EEPROM.
6.  If no new device is found, the user is made aware of that fact

Uses the One_wire_DS18B20 library functions for basic one wire operation.
The EEPROM_Functions library is used to read from and write to the EEPROM
*/

/*---------------------------------------------new_device_to_EEPROM--------------------------------*/
void new_device_to_EEPROM(){
  int i, address;
  int stored_devices;
  int loop_device, loop_bytes;
  boolean match;
  boolean found_new = false;
  char device_description[13];
  int no_chars_from_term;
  int high_alarm, low_alarm, resolution;
  
  int last_device;
  byte Rom_no[8] = {0,0,0,0,0,0,0,0};
  byte alarm_and_config[3];
  boolean proceed = true;
  boolean parasitic;
 
   //Read number of devices
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  if (stored_devices < 46){
    last_device = 0;
    //Do until the last device connected to the bus is found by search_rom().
    do {
      if (!ds18b20.initialize()){  //Must initialize before finding each device.
        last_device = ds18b20.search_rom(Rom_no, last_device);      
        match = false;
        //loop for each device in EEPROM. If a match is found, exit the loop
        for (loop_device = 0; (loop_device != stored_devices && !match); loop_device++){
          address = 22 * loop_device + 4;  //jump to first byte of device in EEPROM
          match = true;
          //loop for each byte.  Compares byte in EEPROM with byte in Rom_no.
          //Exits if a byte does not match. and makes match false. 
          for (loop_bytes = 0; (loop_bytes < 8 && match); loop_bytes++){
            if (Rom_no[loop_bytes] != eeprom.EEPROM_read(address)){
              match = false;  // A byte does not match
            }
            address++;  //get next byte
          }  
        }
      }
      else{
        Serial.println(F("Initialization Failure on ROM Read")); 
        proceed = false; 
      }  
      //if there was no Initialization failure and the ROM code did not match
      //any already existing in the EEPROM, a CRC check on the new ROM code will
      //be done, and the user will be aksed to enter a description.
      if (proceed){
        if (!match){   //We have found a new device
          found_new = true;
          Serial.println(F("A new device has been found."));
          //We'll do a CRC check first
          if (ds18b20.calculateCRC_byte(Rom_no, 8)){
            Serial.println(F("Device ROM failed CRC"));
          }
          else{
            /*This is where we store ROM code and description into the ATmega EEPROM and
            we store upper and lower alarm temperatures and configuration data
            into DS18B20's scratch pad.  Configuration data is the resolution */
            
            //Storing the Rom Code into the next device location in EEPROM
            address = 22 * stored_devices + 4;  
            for (i = 0; i < 8; i++){
              eeprom.EEPROM_write(address, Rom_no[i]);
              address += 1;
            }
            //Asking for a description
            Serial.println(F("Device ROM passed CRC"));
            Serial.println(F("Please enter a description. 12 Characters Maximum:"));
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
            
            //Store 0's into the two calibration bytes
            address = 22 * (loop_device + 1) + 2;  //jump to next device -2
            eeprom.EEPROM_write(address, 0);
            address += 1;
            eeprom.EEPROM_write(address, 0);
            
            //update the number of devices in the EEPROM at address 3
            stored_devices++;
            eeprom.EEPROM_write(3, stored_devices);
            
            //We ask user the upper and lower temperature alarms
            Serial.print(F("Please enter the upper alarm temperature +257 > -67, deg F: "));
            high_alarm = how_many(-67,257);
            Serial.print(high_alarm);
            Serial.println(F(" deg F"));
            alarm_and_config[0] = (float(high_alarm) - 32.0) / 1.8; //convert to deg C            
            if (alarm_and_config[0] < 0){
              alarm_and_config[0] -= 256;  //makes 2's complement
            }
           
            Serial.print(F("Please enter the lower alarm temperature +257 > -67, deg F: "));
            low_alarm = how_many(-67,257);
            Serial.print(low_alarm);
            Serial.println(F(" deg F"));
            alarm_and_config[1] = (float(low_alarm) - 32.0) / 1.8; //convert to deg C            
            if (alarm_and_config[1] < 0){
              alarm_and_config[1] -= 256;    //makes 2's complement
            }
            
            //We ask user the resolution
            Serial.print(F("Please resolution, 9 - 12 bits: "));
            resolution = how_many(9, 12);
            Serial.print(resolution);
            Serial.println(F(" bits"));
            alarm_and_config[2] = 0x1F + 0x10 * 2 * (byte(resolution) - 9);  
            
            //Write upper alarm, lower alarm, and configuration (resolution) to scratchpad            
            //Match ROM followed by Write 3 bytes to ScratchPad
            if (ds18b20.initialize()){
              Serial.println(F("Initialization Failure on Match ROM"));
            }
            else{
              //Match ROM followed by Write Scratchpad with resolution and alarms
              ds18b20.match_rom(Rom_no); 
              ds18b20.write_scratchpad(alarm_and_config); 
            }
            
            //In prepation for writing scratchpad to DS18B20's EEPROM
            //will fimd out if it connected using parasitic power
            if (ds18b20.initialize()){
              Serial.println(F("Initialization Failure"));
            }
            else{
              ds18b20.match_rom(Rom_no); 
              parasitic = false;
              if (!ds18b20.read_power_supply()){  //A zero means parasitic
                parasitic = true;
              }
            }
            
            //Now we copy the scrathpad to the DS18B20's EEPROM
            if (ds18b20.initialize()){
              Serial.println(F("Initialization Failure"));
            }
            else{
              ds18b20.match_rom(Rom_no); 
              ds18b20.copy_scratchpad(parasitic);
            }
            
            //Report to user info about the new device
            Serial.print(F("\tThe new device is device number: "));
            Serial.println(stored_devices);
            Serial.print(F("\tIt's ROM code is: "));
            for (int i = 0; i < 8; i++){
              Serial.print(Rom_no[i], HEX);
              Serial.print(F(" "));
            }
            Serial.println(F(""));
            Serial.print(F("\tWith this description: "));
            for (i = 0; i < no_chars_from_term; i++){
              Serial.print(device_description[i]);
            }
            Serial.println(F(""));
            
            Serial.print(F("\tUpper alarm set to: "));
            Serial.print(high_alarm);
            Serial.print(F(" deg F   Lower alarm set to: "));
            Serial.print(low_alarm);
            Serial.println(F(" deg F"));
            Serial.print(F("\tResolution: "));
            Serial.print(resolution);
            Serial.println(F(" bits"));
            
            if (parasitic){
              Serial.println(F("\tDevice is connected using parasitic power\n"));
            }
            else{
              Serial.println(F("\tDevice is NOT connected using parasitic power\n"));             
            }
          }   
        }
      }  
    //search_rom() will return last_device as 0 when last device on bus has been found
    }while (last_device && proceed); 
    if(!found_new){
      Serial.println(F("Did not find a new device"));
    }
    else{
      Serial.println(F("Done"));
    }
  }
  else {
    Serial.println(F("You already have 50 devices stored in EEPROM - No more room"));
  }  
}
/*______________________________________EDIT DESCRIP[TION__________________________________*/

/*  Edit_Description.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM
2.  For each device, the description is displayed
3.  Asks the user which device to change
4.  Asks the user for the new description
5.  If the user is satisfied with the new description, it is written to the EEPROM

The EEPROM_Functions library is used to read thE` device's description from EEPROM.
InputFromTerminal library is used to enter integer values from the keyboard.
 
*/

/*---------------------------------------------edit_description-------------------------------------*/
void edit_description(int which_device){
  int no_chars_from_term;
  int make_run;
  int character;
  int address;
  char device_description[13];
  int i, j;
  
  //Interfce with user. Keep in loop until user is satisfied with choice
  do{
    address = 22 * (which_device - 1) + 12;
    
    Serial.print(F("\nCurrent description for device number ")); //Print current description
    Serial.print(which_device);
    Serial.print(F(": "));
    character = 0;
    for (int i = 0; (i < 12 && character != 255); i++){
      character = eeprom.EEPROM_read(address +i);
      if (character != 255){
        Serial.print(char(character));
      }
    }
    Serial.println(F(""));
     
    Serial.println(F("\nPlease enter a new description. 12 Characters Maximum:"));
    while (!Serial.available() > 0);
    no_chars_from_term = Serial.readBytesUntil('\n', device_description, 12);
    
    Serial.print(F("\nFor Device No: "));
    Serial.print(which_device);
    
    Serial.print(F(" The New description is: ")); //Print new description
    character = 0;
    for (int i = 0; (i < no_chars_from_term); i++){
      character = device_description[i];
      if (character != 255){
        Serial.print(char(character));
      }
    }
    Serial.println(F(""));
    
    //Check if user is satisfied
    Serial.println(F("\nSelect 1 if satisfied with description, 2 to try again, 3 to exit? "));
    make_run = how_many(1, 3);
    
    if (make_run == 3){
      return;
    }
    else if (make_run == 2){
      Serial.println(F(""));
    }      
  }while (make_run == 2);

  //Write New Description  
  for (int i = 0; i < (no_chars_from_term); i++){
    eeprom.EEPROM_write(address, device_description[i]);
    address += 1;
  }
  if (no_chars_from_term < 12){
    eeprom.EEPROM_write(address, 255); 
  }   
  Serial.println(F("Done With Description"));
}
/*-------------------------------------------edit_scratchpad---------------------------------------*/
void edit_scratchpad(int which_device){
  boolean done = false;
  byte scratchpad[9];
  byte rom[8];
  int address;
  byte alarm_and_config[3];
  int high_alarm, low_alarm, resolution;
  boolean parasitic;
  
  Serial.print(F("\nScratchpad changes for device number: "));
  Serial.println(which_device);
  //Interfce with user. Keep in loop until user is satisfied with choice
  do{
    //Going to retreive upper and lower alarms and resolution from scratchpad
    //First must reteive ROM code using match_rom()
    if (ds18b20.initialize()){
      Serial.println(F("Initialization Failure"));
      done = true;
    }
    else{
      address = 22 * (which_device - 1) + 4;   //Get ROM code from EEPROM
      for (int k = 0; k < 8; k++){   
        rom[k] = eeprom.EEPROM_read(address +k);
      } 
      ds18b20.match_rom(rom);
      //Read scratchpad and check CRC
      ds18b20.read_scratchpad(scratchpad); 
      
      //if CRC fails, device is not connected so exit edit
      if (ds18b20.calculateCRC_byte(scratchpad, 9)){
        Serial.println(F("Device is NOT Connected To Enclosure"));
        done = true;
      }

      else{            
        //Print out alarm temperatures and resolution
        Serial.print(F("\tUpper alarm set to: "));
        if (scratchpad[2] > 127){
          Serial.print(float(scratchpad[2] - 256) * 1.8 + 32.0);
        }
        else{
           Serial.print(float(scratchpad[2]) * 1.8 + 32.0);
        } 
        Serial.print(F(" deg F   Lower alarm set to: "));
        if (scratchpad[3] > 127){
          Serial.print(float(scratchpad[3] - 256) * 1.8 + 32.0);
        }
        else{
           Serial.print(float(scratchpad[3]) * 1.8 + 32.0);
        } 
        Serial.println(F(" deg F"));
        Serial.print(F("\tResolution: "));
        resolution = scratchpad[4];
        resolution = (resolution >> 5) + 9;
        Serial.print(resolution);
        Serial.println(F(" bits"));
      
        //We ask user the upper and lower temperature alarms
        Serial.print(F("Please enter the upper alarm temperature +257 > -67, deg F: "));
        high_alarm = how_many(-67,257);
        Serial.print(high_alarm);
        Serial.println(F(" deg F"));
        alarm_and_config[0] = (float(high_alarm) - 32.0) / 1.8; //convert to deg C            
        if (alarm_and_config[0] < 0){
          alarm_and_config[0] -= 256;  //makes 2's complement
        }
       
        Serial.print(F("Please enter the lower alarm temperature +257 > -67, deg F: "));
        low_alarm = how_many(-67,257);
        Serial.print(low_alarm);
        Serial.println(F(" deg F"));
        alarm_and_config[1] = (float(low_alarm) - 32.0) / 1.8; //convert to deg C            
        if (alarm_and_config[1] < 0){
          alarm_and_config[1] -= 256;    //makes 2's complement
        }
        
        //We ask user the resolution
        Serial.print(F("Please resolution, 9 - 12 bits: "));
        resolution = how_many(9, 12);
        Serial.println(resolution);
        alarm_and_config[2] = 0x1F + 0x10 * 2 * (byte(resolution) - 9);  
        
        //Write upper alarm, lower alarm, and configuration (resolution) to scratchpad            
        //Match ROM followed by Write 3 bytes to ScratchPad
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure on Match ROM"));
        }
        else{
          //Match ROM followed by Write Scratchpad with resolution and alarms
          ds18b20.match_rom(rom); 
          ds18b20.write_scratchpad(alarm_and_config); 
        }
    
        //In prepation for writing scratchpad to DS18B20's EEPROM
        //will fimd out if it connected using parasitic power
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure"));
        }
        else{
          ds18b20.match_rom(rom); 
          parasitic = false;
          if (!ds18b20.read_power_supply()){  //A zero means parasitic
            parasitic = true;
          }
        }
        
        //Now we copy the scrathpad to the DS18B20's EEPROM
        if (ds18b20.initialize()){
          Serial.println(F("Initialization Failure"));
        }
        else{
          ds18b20.match_rom(rom); 
          ds18b20.copy_scratchpad(parasitic);
        }
    
        //Check to see if we are done
        Serial.println(F("\nSatisfied With Changes?"));
        done = yes_or_no();
        if (!done){
          Serial.println(F(""));
        }
      }
    }
  }while (!done); 
  Serial.println(F("Done With Scratchpad"));  
}

/*-----------------------------------------edit_device_information-----------------------------*/
void edit_device_information(){
  boolean done = false;
  int which_device;
  int stored_devices;
  
  do{
    stored_devices = find_stored_devices();
    if (stored_devices){
      Serial.print(F("Which Device To Edit, 0 to Exit? "));
      which_device = how_many(0, stored_devices);
      Serial.println(which_device);
      if (which_device){
        Serial.println(F("\nEnter 1 for Change Description or 2 for Change Scratchpad"));
        if (how_many(1, 2) == 1){
          edit_description(which_device);
        }
        else{
          edit_scratchpad(which_device);       
        }
      }
    }
    Serial.println(F("\nFinished?"));
    done = yes_or_no();
    if (!done){
      Serial.println(F(""));
    }
  }while(!done);
  Serial.println(F("\nDone"));    
}
/*______________________________________DEVICES ON THE BUS__________________________________*/

/*  Devices_On_The_Bus.ino    thepiandi@blogspot.com       MJL  070214

Uses the DS18B20 Search rom command in the One_wire_DS18B20 library to find the ROM code
of each device that it finds on the bus.  Once the ROM code is found, CRC is performed and
the results displayed.
*/

void devices_on_bus(){
  int i; 
  int last_device;
  byte Rom_no[8] = {0,0,0,0,0,0,0,0};
 
  last_device = 0;
  //Do until the last device connected to the bus is found by search_rom().
  do {
    if (!ds18b20.initialize()){  //Must initialize before finding each device.
      last_device = ds18b20.search_rom(Rom_no, last_device);      

      for (i = 0; i < 8; i++){
        Serial.print(Rom_no[i], HEX);
        Serial.print(F(" "));
      }
      if (ds18b20.calculateCRC_byte(Rom_no, 8)){
        Serial.println(F("Device ROM failed CRC"));
      }
      else{
        Serial.println(F("   CRC Passed"));
      }
    }
    else{
      Serial.println(F("Initialization Failure on ROM Read. Perhaps No Device On The Bus")); 
    }  
   //search_rom() will return last_device as 0 when last device on bus has been found
  }while (last_device);
}
/*______________________________________DEVICES IN EEPROM__________________________________*/

/*  Devices_In_EEPROM.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM.
2.  For each device, the ROM code and the description is displayed.

The EEPROM_Functions library is used to read the device's ROM code and description from EEPROM.
*/

void devices_in_EEPROM(){
  int i, j, address;
  int stored_devices;
  int character;
  float correction;
  int cal_factor;
  
  //How Many devices stored in the EEPROM?
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  if(stored_devices){
    for (i = 0; i < stored_devices; i++){
      address = 22 * i + 4;  //jump to first byte of device in EEPROM
      
      //Print the device number:
      Serial.print(F("Device No: "));
      Serial.println(i + 1);
      
      //Print the ROM Code:      
      Serial.print(F("\tIt's ROM code is: "));  
      for ( j = 0; j < 8; j++){
        Serial.print(eeprom.EEPROM_read(address + j), HEX);
        Serial.print(F(" "));
      }
      Serial.println(F(""));
      
      //Print the description:
      Serial.print(F("\tWith this description: "));
      character = 0;
      for ( j = 8; (j < 20 && character != 255); j++){
        character = eeprom.EEPROM_read(address +j);
        if (character != 255){
          Serial.print(char(character));
        }
      }
      Serial.println(F(""));
      
      //Print Low and High Calibration Offsets:
      Serial.print(F("\tLow Temperature Offset (subtract this): "));
      cal_factor = eeprom.EEPROM_read(address + 20);
      
      //check if negative
      if (cal_factor > 128){
        cal_factor += 65280;
      }
      correction = 1.8 * (cal_factor / 16.0);
      Serial.print(correction, 4);
      Serial.println(F(" deg F"));
 
      Serial.print(F("\tHigh Temperature Offset (subtract this): "));
      cal_factor = eeprom.EEPROM_read(address + 21);
      
      //check if negative
      if (cal_factor > 128){
        cal_factor += 65280;
      }
      correction = 1.8 * (cal_factor / 16.0);
      Serial.print(correction, 4); 
      Serial.println(F(" deg F"));
    }    
  }
  else{    
    Serial.println(F("There are no devices stored in the EEPROM"));
  }
}

/*__________________________________REMOVE DEVICE FROM EEPROM______________________________*/

/*  Remove_Device_From_EEPROM.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM
2.  For each device, the description is displayed
3.  Asks the user which device to remove
4.  If the user is satisfied, the 20 bytes of the device's location are overwritten.
5.  Reduce the number of devices stored in EEPROM position 3.

The EEPROM_Functions library is used to read thE` device's description from EEPROM.
InputFromTerminal library is used to enter integer values from the keyboard.
 
*/


/*------------------------------------------remove_from_EEPROM-------------------------------------*/
void remove_from_EEPROM(int stored_devices){
  int address, character;
  long which_device;  
  boolean make_run = false;
  int bytes_to_move;
  int i, j;
  
  //Interfce with user. Keep in loop until user is satisfied with choice
  
  Serial.print(F("Which Device To Remove, 0 to Exit? "));
  which_device = how_many(0, stored_devices);
  Serial.println(which_device);
  
  if (which_device){
    Serial.println(F("\nOK To Remove Device? "));
    if (yes_or_no()){
      Serial.println("");  
      //Find address of first byte to be overwritten, and address of first byte to be moved
      address = 22 * (which_device - 1) + 4;
      //Calculate number of bytes to moce
      bytes_to_move = 22 * (stored_devices - which_device) + 22;
      //Loop to read byte 22 positions away and write to byte to be wirtten over 
      for (i = 0; i < bytes_to_move; i++){
          character = eeprom.EEPROM_read(address + 22 +i);
          eeprom.EEPROM_write((address + i), character);
      }
      //Reduce the number of devices in EEPROM address 3 (the number of stored devices)
      eeprom.EEPROM_write(3, (stored_devices - 1));
      // Say operation is complete
      Serial.println(F("Operation complete"));  
    }
  }
}

/*-----------------------------remove_from_EEPROM setup-------------------------------------*/
void remove_from_EEPROM_setup() {
  boolean done = false;
  
  do{
    int stored_devices = find_stored_devices();
    if (stored_devices){
      remove_from_EEPROM(stored_devices);
    }
    Serial.println(F("\nFinished?"));
    done = yes_or_no();
    if (!done){
      Serial.println(F(""));
    }
  }while(!done);
  Serial.println(F("\nDone")); 
}

/*___________________________________________________________________________________________*/

void setup() {
  Serial.begin(9600);
  
  int choice;
  int stored_devices = find_stored_devices();
  boolean done = false;
  
  Serial.println(F("END OF LINE CHARACTER MUST BE SET TO Newline\n\n"));
 
  do{ 
    if (stored_devices){
      Serial.println(F("What Would You Like To Do? Press 1 to 7, 0 to Exit."));
      Serial.println(F("  1.  Make Temperature Measurements"));
      Serial.println(F("  2.  Check for Alarms"));
      Serial.println(F("  3.  Add New Device To 1-Wire Bus"));
      Serial.println(F("  4.  Display Devices On The 1-Wire Bus"));
      Serial.println(F("  5.  Display Devices In The EEPROM"));
      Serial.println(F("  6.  Edit Stored Information"));
      Serial.println(F("  7.  Remove Device From The EEPROM"));  
      Serial.println(F("  0.  Exit")); 
      Serial.println(F("Enter 0 to 7"));
      
      choice = how_many(0, 7);
      Serial.println("");
      if (choice == 1){
        read_temperature(); 
        Serial.println(F("\n\n\n\n\n"));
      }
      else if (choice == 2){
        alarms();
        Serial.println(F("\n\n\n\n\n"));
      }  
      else if (choice == 3){
        new_device_to_EEPROM();
        Serial.println(F("\n\n\n\n\n"));
      }
      else if (choice == 4){
        devices_on_bus();
        Serial.println(F("\n\n\n\n\n"));
      }
      else if (choice == 5){
        devices_in_EEPROM();
        Serial.println(F("\n\n\n\n\n"));
      }
      else if (choice == 6){
        edit_device_information();
        Serial.println(F("\n\n\n\n\n"));
      }
      else if (choice == 7){
        remove_from_EEPROM_setup();
        Serial.println(F("\n\n\n\n\n"));
      }
      else if (choice == 0){
        Serial.println(F("\n\nWe are done"));
        done = true;
      }
    }
    else{
      Serial.println(F("Add New Device To 1-Wire Bus"));
      choice = yes_or_no();
      if (yes_or_no){
        new_device_to_EEPROM(); 
        Serial.println(F("\n\n\n\n\n"));
     }
    }
  }while(!done);


/*
  read_temp_setup(); 
  alarms();
  new_device_to_EEPROM();
  edit_device_information();
  devices_on_bus();
  devices_in_EEPROM();
  remove_from_EEPROM_setup();
*/  
}

void loop() {
}
