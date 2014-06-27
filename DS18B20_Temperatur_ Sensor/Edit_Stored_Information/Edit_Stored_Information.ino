/*  Edit_Description.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM
2.  For each device, the description is displayed
3.  Asks the user which device to change
4.  Asks the user for the new description
5.  If the user is satisfied with the new description, it is written to the EEPROM

The EEPROM_Functions library is used to read thE` device's description from EEPROM.
InputFromTerminal library is used to enter integer values from the keyboard.
 
*/
#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;

/*---------------------------------------------find_stored_devices-------------------------------*/
int find_stored_devices(){
  int stored_devices, address, character;
  int resolution;
  byte scratchpad[9];
  byte rom[8];
  
  //How Many devices stored in the EEPROM?
  stored_devices = eeprom.EEPROM_read(3); 
  if (stored_devices == 0xFF){   //condition if EEPROM has been cleared.
    stored_devices = 0;  
  }
  if(stored_devices){
    //List the devices Srored in EEPROM
    for (int i = 0; i < stored_devices; i++){
      Serial.print("Device No: ");
      Serial.print(i + 1);
      Serial.print("  Descriptiom: ");
      
      //Print out description
      character = 0;
      address = 20 * i + 12;
      for (int j = 0; (j < 12 && character != 255); j++){
        character = eeprom.EEPROM_read(address +j);
        if (character != 255){
          Serial.print(char(character));
        }
      }
      Serial.println("");

      //Going to retreive upper and lower alarms and resolution from scratchpad
      //First must reteive ROM code using match_rom()
      if (ds18b20.initialize()){
        Serial.println("Initialization Failure");
      }
      else{
        Serial.print("\tROM Code: ");
        address = 20 * i + 4;   //Get ROM code from EEPROM and Print it out
        for (int k = 0; k < 8; k++){   
          rom[k] = eeprom.EEPROM_read(address +k);
          Serial.print(rom[k], HEX);
          Serial.print(" ");
        } 
        Serial.println("");
        
        
        ds18b20.match_rom(rom);
        //Read scratchpad and check CRC
        ds18b20.read_scratchpad(scratchpad); 
        if (ds18b20.calculateCRC_byte(scratchpad, 9)){
          Serial.println("Scratchpad failed CRC");
        }
        else{            
          //Print out alarm temperatures and resolution
          Serial.print("\tUpper alarm set to: ");
          if (scratchpad[2] > 127){
            Serial.print(float(scratchpad[2] - 256) * 1.8 + 32.0);
          }
          else{
             Serial.print(float(scratchpad[2]) * 1.8 + 32.0);
          } 
          Serial.print(" deg F   Lower alarm set to: ");
          if (scratchpad[3] > 127){
            Serial.print(float(scratchpad[3] - 256) * 1.8 + 32.0);
          }
          else{
             Serial.print(float(scratchpad[3]) * 1.8 + 32.0);
          } 
          Serial.println(" deg F");
          Serial.print("\tResolution: ");
          resolution = scratchpad[4];
          resolution = (resolution >> 5) + 9;
          Serial.print(resolution);
          Serial.println(" bits");
        }
      Serial.println("");     
      }
    }
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
/*---------------------------------------------edit_description-------------------------------------*/
void edit_description(int which_device){
  int no_chars_from_term;
  boolean make_run = false;
  int character;
  int address;
  char device_description[20];
  int i, j;
  
  //Interfce with user. Keep in loop until user is satisfied with choice
  do{
    address = 20 * which_device - 8;
    
    Serial.print("\nCurrent description for device number "); //Print current description
    Serial.print(which_device);
    Serial.print(": ");
    character = 0;
    for (int i = 0; (i < 12 && character != 255); i++){
      character = eeprom.EEPROM_read(address +i);
      if (character != 255){
        Serial.print(char(character));
      }
    }
    Serial.println("");
     
    Serial.println("\nPlease enter a new description. 12 Characters Maximum:");
    while (!Serial.available() > 0);
    no_chars_from_term = Serial.readBytesUntil('\n', device_description, 12);
    
    Serial.print("\nFor Device No: ");
    Serial.print(which_device);
    
    Serial.print(" The New description is: "); //Print new description
    character = 0;
    for (int i = 0; (i < no_chars_from_term); i++){
      character = device_description[i];
      if (character != 255){
        Serial.print(char(character));
      }
    }
    Serial.println("");
    
    //Check if user is satisfied
    Serial.println("\nSatisfied With Description? ");
    make_run = yes_or_no();
    if (!make_run){
      Serial.println("");
    }      
  }while (!make_run);

  //Write New Description  
  for (int i = 0; i < (no_chars_from_term); i++){
    eeprom.EEPROM_write(address, device_description[i]);
    address += 1;
  }
  if (no_chars_from_term < 12){
    eeprom.EEPROM_write(address, 255); 
  }   
  Serial.println("Done With Description");
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
  
  Serial.print("\nScratchpad changes for device number: ");
  Serial.println(which_device);
  //Interfce with user. Keep in loop until user is satisfied with choice
  do{
    //Going to retreive upper and lower alarms and resolution from scratchpad
    //First must reteive ROM code using match_rom()
    if (ds18b20.initialize()){
      Serial.println("Initialization Failure");
    }
    else{
      address = 20 * which_device - 16;   //Get ROM code from EEPROM
      for (int k = 0; k < 8; k++){   
        rom[k] = eeprom.EEPROM_read(address +k);
      } 
      ds18b20.match_rom(rom);
      //Read scratchpad and check CRC
      ds18b20.read_scratchpad(scratchpad); 
      if (ds18b20.calculateCRC_byte(scratchpad, 9)){
        Serial.println("Device ROM failed CRC");
      }

      else{            
        //Print out alarm temperatures and resolution
        Serial.print("\tUpper alarm set to: ");
        if (scratchpad[2] > 127){
          Serial.print(float(scratchpad[2] - 256) * 1.8 + 32.0);
        }
        else{
           Serial.print(float(scratchpad[2]) * 1.8 + 32.0);
        } 
        Serial.print(" deg F   Lower alarm set to: ");
        if (scratchpad[3] > 127){
          Serial.print(float(scratchpad[3] - 256) * 1.8 + 32.0);
        }
        else{
           Serial.print(float(scratchpad[3]) * 1.8 + 32.0);
        } 
        Serial.println(" deg F");
        Serial.print("\tResolution: ");
        resolution = scratchpad[4];
        resolution = (resolution >> 5) + 9;
        Serial.print(resolution);
        Serial.println(" bits");
      }
      
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
      Serial.println(resolution);
      alarm_and_config[2] = 0x1F + 0x10 * 2 * (byte(resolution) - 9);  
      
      //Write upper alarm, lower alarm, and configuration (resolution) to scratchpad            
      //Match ROM followed by Write 3 bytes to ScratchPad
      if (ds18b20.initialize()){
        Serial.println("Initialization Failure on Match ROM");
      }
      else{
        //Match ROM followed by Write Scratchpad with resolution and alarms
        ds18b20.match_rom(rom); 
        ds18b20.write_scratchpad(alarm_and_config); 
      }
  
      //In prepation for writing scratchpad to DS18B20's EEPROM
      //will fimd out if it connected using parasitic power
      if (ds18b20.initialize()){
        Serial.println("Initialization Failure");
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
        Serial.println("Initialization Failure");
      }
      else{
        ds18b20.match_rom(rom); 
        ds18b20.copy_scratchpad(parasitic);
      }
    }
     Serial.println("\nSatisfied With Changes?");
    done = yes_or_no();
    if (!done){
      Serial.println("");
    }
  }while (!done); 
  Serial.println("Done With Scratchpad");  
}

/*-----------------------------------------edit_device_information-----------------------------*/
void edit_device_information(){
  boolean done = false;
  int which_device;
  int stored_devices;
  
  do{
    stored_devices = find_stored_devices();
    if (stored_devices){
      Serial.print("Which Device To Edit, 0 to Exit? ");
      which_device = how_many(0, stored_devices);
      Serial.println(which_device);
      if (which_device){
        Serial.println("\nEnter 1 for Change Description or 2 for Change Scratchpad");
        if (how_many(1, 2) == 1){
          edit_description(which_device);
        }
        else{
          edit_scratchpad(which_device);       
        }
      }
    }
    Serial.println("\nFinished?");
    done = yes_or_no();
    if (!done){
      Serial.println("");
    }
  }while(!done);
  Serial.println("\nDone");  
  
    
    
    

  
}

/*---------------------------------------------setup------------------------------------------*/

void setup() {
  Serial.begin(9600);
  edit_device_information();
}

void loop() {

}
