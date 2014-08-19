/*  Remove_Device_From_EEPROM.ino    thepiandi@blogspot.com       MJL  060614

1.  Retrieves number of devices stored in the EEPROM by querying location 3 in the EEPROM
2.  For each device, the description is displayed
3.  Asks the user which device to remove
4.  If the user is satisfied, the 20 bytes of the device's location are overwritten.
5.  Reduce the number of devices stored in EEPROM position 3.

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
      Serial.print("  Description: ");
   
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

/*---------------------------------------------remove_from_EEPROM-------------------------------------*/
void remove_from_EEPROM(int stored_devices){
  int address, character;
  long which_device;  
  boolean make_run = false;
  int bytes_to_move;
  int i, j;
  
  //Interfce with user. Keep in loop until user is satisfied with choice
  
  Serial.print("Which Device To Remove, 0 to Exit? ");
  which_device = how_many(0, stored_devices);
  Serial.println(which_device);

  if (which_device){  
    Serial.println("\nOK To Remove Device? ");
    if (yes_or_no()){
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
  }
}

/*---------------------------------------------setup-------------------------------------*/
void setup() {
  boolean done = false;
  Serial.begin(9600);
  
  do{
    int stored_devices = find_stored_devices();
    if (stored_devices){
      remove_from_EEPROM(stored_devices);
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
