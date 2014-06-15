/*  Temperature_Support.ino    thepiandi@blogspot.com       MJL  061514

Brings together all of the sketches except Read_Temperatures.ino.  There was
not enough memory in Arduino's space for global and local variables to include the
Read_Temperature.ino.  There were simply too many print statements.

This brings menu driven capability to choose the desired operation.

The following functions are included here:
1.  Find_new_device
2.  Devices_On_The_Bus
3.  Devices_In_EEPROM
4.  Edit_Description
5.  Demove_Device_From_EEPROM

If there are no stored devices in the EEPROM only selection 1 is available.
Functions common to more than one sketch are included here only one time.  This
includes "find_stored_devices", "yes_Or_no", and "how_many".

Menu loops around until user is finished.
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

/*--------------------------------------------new_device_to_EEPROM-------------------------------------*/
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

/*--------------------------------------------devices_in_EEPROM-------------------------------------*/
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
/*--------------------------------------devices_on_bus-------------------------------------*/
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
/*---------------------------------------------edit_description-------------------------------------*/
void edit_description(int stored_devices){
  int which_device;
  int no_chars_from_term;
  boolean make_run = false;
  int character;
  int address;
  char device_description[20];
  int i, j;
  
  //Interfce with user. Keep in loop until user is satisfied with choice
  do{
    Serial.print("Which One To Edit?");
    which_device = how_many(1, stored_devices);
    Serial.println(which_device);
   
    address = 20 * which_device -16;
    
    Serial.print("\nCurrent description: "); //Print current description
    character = 0;
    for (int i = 8; (i < 20 && character != 255); i++){
      character = eeprom.EEPROM_read(address +i);
      if (character != 255){
        Serial.print(char(character));
      }
    }
    Serial.println("");
    
    address = 20 * which_device - 8;
  
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
  Serial.println("Done");
}
/*---------------------------------------------remove_from_EEPROM-------------------------------------*/
void remove_from_EEPROM(int stored_devices){
  int address, character;
  long which_device;  
  boolean make_run = false;
  int bytes_to_move;
  int i, j;
  
  //Interfce with user. Keep in loop until user is satisfied with choice
  do{
    Serial.print("Which Device To Remove? ");
    which_device = how_many(1, stored_devices);
    Serial.println(which_device);
    
    Serial.println("\nOK To Remove Device? ");
    make_run = yes_or_no();
    if (!make_run){
      Serial.println("");
    }   
  }while (!make_run);
  Serial.println("");
 
  //Find address of first byte to be overwritten, and address of first byte to be moved
  address = 20 * which_device - 16;
  //Calculate number of bytes to moce
  bytes_to_move = 20 * (stored_devices - which_device) + 20;
  //Loop to read byte 20 positions away and write to byte to be wirtten over 
  for (i = 0; i < bytes_to_move; i++){
      character = eeprom.EEPROM_read(address + 20 +i);
      eeprom.EEPROM_write((address + i), character);
  }
  //Reduce the number of devices in EEPROM address 3 (the number of stored devices)
  eeprom.EEPROM_write(3, (stored_devices - 1));
  // Say operation is complete
  Serial.println("Operation complete");  
}
/*---------------------------------------------setup-------------------------------------*/
void setup() {
  Serial.begin(9600);
  int choice;
  int stored_devices;
  boolean done = false;

  do{
    stored_devices = find_stored_devices();
    if (stored_devices){
      Serial.println("What Would You Like To Do? Press 1 to 6.");
      Serial.println("  1.  Add New Device To 1-Wire Bus");
      Serial.println("  2.  Display Devices On The 1-Wire Bus");
      Serial.println("  3.  Display Devices In The EEPROM");
      Serial.println("  4.  Edit Device Descriptipn In The EEPROM");
      Serial.println("  5.  Remove Device From The EEPROM");   
      Serial.print("Enter 1 to 5: ");
      choice = how_many(1, 5);
      Serial.println(choice);
      Serial.println("");
      if (choice == 1){
        new_device_to_EEPROM();
      }
      else if (choice == 2){
        devices_on_bus();
      }
      else if (choice == 3){
        devices_in_EEPROM();
      }
      else if (choice == 4){
        find_stored_devices();
        edit_description(stored_devices);
      }
      else if (choice == 5){
        find_stored_devices();
        remove_from_EEPROM(stored_devices);
      }
    }
    else{
      Serial.println("Add New Device To 1-Wire Bus");
      choice = yes_or_no();
      if (yes_or_no){
        new_device_to_EEPROM(); 
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

void loop() {

}
