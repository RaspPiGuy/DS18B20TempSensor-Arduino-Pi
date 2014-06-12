/*  Edit_Description.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM
2.  For each device, the description is displayed
3.  Asks the user which device to change
4.  Asks the user for the new description
5.  If the user is satisfied with the new description, it is written to the EEPROM

The EEPROM_Functions library is used to read thE` device's description from EEPROM.
InputFromTerminal library is used to enter integer values from the keyboard.
 
*/
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"

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


void setup() {
  Serial.begin(9600);
  int stored_devices = find_stored_devices();
  if (stored_devices){
    edit_description(stored_devices);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
