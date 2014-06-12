/*  Read_Temperatures.ino    thepiandi@blogspot.com       MJL  060614

Everything but the kitchen sink program to read temperatures from the DS18B20 device.  Program:
1.    Displays the description and device number of all the devices in EEPROM
2.    Asks user if all devices are to be run.
2.a.  If not, displays each device, in turn, and asks if this device will be run
3.    Asks how many measurements are to be run
4.    Asks what the delay berween measurements is to be.  Asks hours, minutes, seconds, milliseconds
5.    Asks how many bits of resolution 9 - 12 possible
6.    Asks if it OK to run.  If not, program returns to step 2. above.

7.    Runs two loops, one for each measurement, and the inner loop, one for each device to be run.
8.    For each measurement retrieves ROM code from EEPROM and runs the following DS18B20 commands:
8.a.    Initialize, match rom, write scratchpad with resolution (alarm limits set to max and min).
8.b.    Initialize, match rom, convert t, read scratchpad, CRC on scratchpad.
9.    Temperature is calculated from scratch pad and displayed in deg F.
10.   When the run is completed, the average is displayed for each device run.
11.   Other parameters and diagnostic information are displayed.

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
    Serial.println("Please enter Y or y, or N or n/n");        
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
  byte alarm_and_config[3] = {0x7D, 0xC9, 0x7F};
  float temp_c, temp_f;
  int address;
  int i, j, k;
  int conversion_time; 
  long which_device, measurements, delay_between_measurements, resolution;  
  boolean make_run = false;
  int init_failures = 0;
  int crc_errors = 0;
  byte res_byte;
  int number_of_devices_to_run;
  int hours, minutes, seconds, milliseconds;
  
  boolean devices2run[stored_devices];
  float averages[stored_devices];
  number_of_devices_to_run = stored_devices;
  
  //Interfce with user. Keep in loop until user is satisfied with choices
  do{
    //Set up array for devices to run as all true  
    for (i = 0; i < stored_devices; i++){
      devices2run[i] = true;
      averages[i] = 0;
    }
    //Ask if all devices are to be run.  If Yes do nothing otherwise query each device
    Serial.println("Run all devices? ");
    if (!yes_or_no()){  //If no is selected this will be true
      for (i = 0; i < stored_devices; i++){
        Serial.print("\nRun device number ");  
        Serial.print(i + 1);
        Serial.println("?");
        
        if (!yes_or_no()){  //If answer is no, make array entry for device false
          devices2run[i] = false;
          number_of_devices_to_run--;
        }     
      }
      Serial.println("");
    }
    else{
      Serial.println("");
    }
    //Ask number of measurements, time between measurements and resolution
    Serial.print("How many measurements would you like to make? ");
    measurements = how_many(1, 10000000);
    Serial.println(measurements);
  
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
  
    Serial.print("\nHow many bits of resolution, 9 - 12? ");
    resolution = how_many(9, 12);
    Serial.println(resolution); 
    res_byte = 0x1F + 0x10 * 2 * (byte(resolution) - 9);  
    Serial.println("");
    
    Serial.println("OK To Make Run? ");
    make_run = yes_or_no();
    if (!make_run){
      Serial.println("");
    }
      
  }while (!make_run);
  Serial.println("");

  //Main measurement loop
  for (i = 0; i < measurements; i++){
    Serial.print("Measurement: ");
    Serial.println(i + 1);
    for (j = 0; j < stored_devices; j++){
      if (devices2run[j]){  //check to see if device is to be run
        address = 20 * j +4;   //Get ROM code from EEPROM
        for (k = 0; k < 8; k++){
          rom[k] = eeprom.EEPROM_read(address +k);
        } 
            
        //Match ROM followed by Write 3 bytes to ScratchPad
        if (ds18b20.initialize()){
          Serial.println("Initialization Failure on Matcch ROM");
        }
        else{
          //Match ROM followed by Write Scratchpad with resolution
          ds18b20.match_rom(rom); 
          alarm_and_config[2] = res_byte; 
          ds18b20.write_scratchpad(alarm_and_config); 
         
          //Match ROM followed by Convert T
          if (ds18b20.initialize()){
            Serial.println("Initialization Failure on Convert Temperature");
          }
          else{
            //Print Out Measurement Number
            Serial.print("  Device Number: ");
            Serial.print(j + 1);
            
            //Match ROM followed by Convert T
            if (ds18b20.initialize()) init_failures++; 
            ds18b20.match_rom(rom);
            ds18b20.convert_t();
            
            //Match ROM followed by Read Scratchpad
            if (ds18b20.initialize()) init_failures++; 
            ds18b20.match_rom(rom);
            ds18b20.read_scratchpad(scratchpad);
            
            //Check scratchpad CRC
            if (ds18b20.calculateCRC_byte(scratchpad, 9)) crc_errors++;
            
            //Calculate Temperature and Print Results
            temp_c = (256 * scratchpad[1] + scratchpad[0]);
            if (scratchpad[1] > 127){  //Temperature below zero
              temp_c -= 4096;          //calculates 2's complement
            }
            temp_c /= 16.0;
            temp_f = 1.8 * temp_c + 32.0;
            averages[j] += temp_f;
            
            Serial.print("\tTemperature: ");
            Serial.print(temp_f);
            Serial.println(" degF");
          } 
        }
      }
    }
    if (i != (measurements -1)){
      delay(delay_between_measurements); 
    }
  }
  //Finish up. Print some results
  //Print Averages Temperatures
  Serial.println("");
  for (i = 0; i < stored_devices; i++){
    if (devices2run[i]){
      Serial.print("Average for Device Number ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(averages[i] / measurements);
      Serial.println(" degF");      
    }
  }
  Serial.print("\nResolution: ");
  Serial.print(resolution);
  Serial.println(" bits.");
  Serial.print("Number of measurements: ");
  Serial.println(measurements * number_of_devices_to_run);
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
