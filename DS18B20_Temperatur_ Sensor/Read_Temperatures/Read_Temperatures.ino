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

#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;

/*---------------------------------------------find_stored_devices-------------------------------------*/
int find_stored_devices(){
  int stored_devices, address, character;
  
  //How Many devices stored in the EEPROM?
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  if(stored_devices){
    //List the devices Srored in EEPROM
    address = 12;
    for (int i = 0; i < stored_devices; i++){
      Serial.print("Device No: ");
      Serial.print(i + 1);
      Serial.print("  Descriptiom: ");
   
      character = 0;
      for (int j = 0; (j < 12 && character != 255); j++){
        character = eeprom.EEPROM_read(address +j);
        if (character != 255){
          Serial.print(char(character));
        }
      }
      Serial.println("");  
      address += 20;
    }
    Serial.println("");
  }
  else{
    Serial.println("There are no devices stored in the EEPROM");
  }
  
  return stored_devices;
}

/*---------------------------------------------yes_or_no-------------------------------------*/
boolean yes_or_no(){
  boolean yes_no = false;
  char keyboard_entry[5];
  do{
    Serial.println("Please enter Y or y, or N or n");        
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
      Serial.println("Please Try Again");
    }
  }while (!good_choice);

  return menu_choice; 
}

/*---------------------------------------------read_temperature-------------------------------------*/
void read_temperature(int stored_devices){
  byte rom[8];
  byte scratchpad[9];
  int description[12];
  byte alarm_and_config[3] = {0x7D, 0xC9, 0x7F};
  float temp_c, temp_f;
  int address;
  int i, j, k;
  int conversion_time; 
  long which_device, measurements, delay_between_measurements, resolution;  
  boolean make_run = false;
  int init_failures = 0;
  int crc_errors = 0;
  int number_of_devices_to_run;
  int hours, minutes, seconds, milliseconds;
  boolean parasitic = false;
  int high_alarm, low_alarm;
  boolean display_scratchpad;
  int spaces;
  boolean devices2run[stored_devices];
  float averages[stored_devices];
  int character;
  
  number_of_devices_to_run = stored_devices;
  
  //Interface with user. Keep in loop until user is satisfied with choices
  do{
    //Set up array for devices to run as all true  
    for (i = 0; i < stored_devices; i++){
      devices2run[i] = true;
      averages[i] = 0;
    }
    /*Ask if all devices are to be run.  If Yes determine if any device is 
    connected using parasitic power, otherwise query each device and determine
    if the selected device is connected using parasitic power.*/
    Serial.println("Run all devices? ");
    if (yes_or_no()){  //If yes is selected this will be true
      if (ds18b20.initialize()){
        Serial.println("Initialization Failure on Read_Power_Supply");
      }
      else{
        ds18b20.skip_rom();
        if (ds18b20.read_power_supply()){
          Serial.println("\nNo device is connected using parasitic power\n");
        }
        else{
          Serial.println("\nA device is connected using parasitic power\n");
          parasitic = true;
        }
      }     
    }
    else{  //No was selected so query for each device.
      for (i = 0; i < stored_devices; i++){
        Serial.print("\nRun device number ");  
        Serial.print(i + 1);
        Serial.println("?");
        
        if (!yes_or_no()){  //If answer is no, make array entry for device false
          devices2run[i] = false;
          number_of_devices_to_run--;
        } 
        else{  //Determine if it is connected using parasitic power
          if (ds18b20.initialize()){
            Serial.println("Initialization Failure");
          }
          else{
            address = 20 * i +4;   //Get ROM code from EEPROM
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
      if (parasitic){
        Serial.println("\nA device is connected using parasitic power");
      }  
      else{
        Serial.println("\nNo device is connected using parasitic power");
      }    
      Serial.println("");
    }

    //Ask number of measurements, time between measurements
    Serial.print("How many measurements would you like to make? ");
    measurements = how_many(1, 10000000);
    Serial.println(measurements);

    //If we only make one measurement we will not ask for time between measurements
    if (measurements > 1){  
      Serial.print("\nHow much time between measurements in hours? ");
      hours = how_many(0, 10000000);
      Serial.println(hours);
      
      Serial.print("How much time between measurements in minutes? ");
      minutes = how_many(0, 60);
      Serial.println(minutes);
      
      Serial.print("How much time between measurements in seconds? ");
      seconds = how_many(0, 60);
      Serial.println(seconds);
      
      Serial.print("How much time between measurements in ms.? ");
      milliseconds = how_many(0, 1000);
      Serial.println(milliseconds);
      
      delay_between_measurements = hours * 60 *60 *1000 + minutes * 60 * 1000 + seconds * 1000 + milliseconds; 
    }
    else{
      delay_between_measurements = 0;
    } 
  
    Serial.print("\nDisplay alarm temperatures and resolution? ");
    display_scratchpad = yes_or_no();
    Serial.println("");
    
    Serial.println("OK To Make Run? ");
    make_run = yes_or_no();
    if (!make_run){
      Serial.println("");
    }
      
  }while (!make_run);
  Serial.println("");
   
  //Outer measurement loop
  //Run for each measurement
  for (i = 0; i < measurements; i++){  
    Serial.print("Measurement: ");
    Serial.println(i + 1);
    //Inner Loop
    //Run for each device
    for (j = 0; j < stored_devices; j++){  
      if (devices2run[j]){  //check to see if device is to be run
        address = 20 * j +4;   //Get ROM code from EEPROM
        for (k = 0; k < 8; k++){
          rom[k] = eeprom.EEPROM_read(address + k);
        } 
        address += 8;
        for (k = 0; k < 12; k++){  //Get description
          description[k] = eeprom.EEPROM_read(address + k);
        }
                     
        //Match ROM followed by read Scratchpad for resolution and alarm temperatures
        if (ds18b20.initialize()){
          Serial.println("Initialization Failure on Match ROM");
        }
        else{
          ds18b20.match_rom(rom); 
          //Read scratchpad and check CRC
          ds18b20.read_scratchpad(scratchpad); 
          if (ds18b20.calculateCRC_byte(scratchpad, 9)){
            Serial.println("Scratchpad failed CRC");
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
            //Retreive low temperature alarm            
            resolution = scratchpad[4];
            resolution = (resolution >> 5) + 9;
          }
          
          //Match ROM followed by Convert T if Initialization Passes
          if (ds18b20.initialize()){
            Serial.println("Initialization Failure on Convert Temperature");
          }
          else{
            //Print Device Description
            spaces = 14;
            Serial.print("  ");
            for (int k = 0; (k < 12 && description[k] != 255); k++){
              Serial.print(char(description[k]));
              spaces--;
            }
            //Print spaces after description
            for (k= 0; k < spaces; k++){
              Serial.print(" ");
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
            temp_f = 1.8 * temp_c + 32.0;
            averages[j] += temp_f;
            
            Serial.print("  Temp.: ");
            Serial.print(temp_f);
            Serial.print(" deg F");
            if (display_scratchpad){
              Serial.print("  Upper: ");
              Serial.print(high_alarm);
              Serial.print(" deg F  Lower: ");
              Serial.print(low_alarm);
              Serial.print(" deg F  Res.: ");
              Serial.print(resolution);
              Serial.println(" bits");
            }
            else{
              Serial.println("");
            }
          } 
        }
      }
    }
    if (i != (measurements -1)){
      delay(delay_between_measurements); 
    }
  }
  Serial.println("");  
  
  //Finish up. Print some results
  //Print Averages Temperatures
  if (measurements > 1){
    for (i = 0; i < stored_devices; i++){
      Serial.print("Average for Device - ");
  
      if (devices2run[i]){
        spaces = 14;
        address = 20 * i + 12;
        character = 0;
        for (int j = 0; (j < 12 && character != 255); j++){
          character = eeprom.EEPROM_read(address +j);
          if (character != 255){
            Serial.print(char(character));
            spaces--;
          }
        }
        Serial.print(":");
        for (k = 0; k < spaces; k++) Serial.print(" ");
        Serial.print(averages[i] / measurements);
        Serial.println(" deg F");      
      }
    }
    Serial.println("");
  }
  Serial.print("Parasitic Operation? ");
  if (parasitic) Serial.println("Yes");
  else Serial.println("No");
  Serial.print("Number of measurements: ");
  Serial.println(measurements);
  Serial.print("Delay Between Measurements, ms.: ");
  Serial.println(delay_between_measurements);  
  Serial.print("Number of initialization failures: ");
  Serial.println(init_failures);
  Serial.print("CRC failures: ");
  Serial.println(crc_errors);
  Serial.println("");         
}

/*---------------------------------------------setup-------------------------------------*/
void setup() {
  Serial.begin(9600);
  int stored_devices = find_stored_devices();
  if (stored_devices){
    read_temperature(stored_devices);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
