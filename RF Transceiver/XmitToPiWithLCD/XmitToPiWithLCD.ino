/* Transmits Manchester encoded data to my Gertboard

This sketch is to be used with my temperature measuring enclosure.  This
device has one permanently mounted DS18B20 temperature sensor and three
connectors for adding more sensors.  The hardware and software supports parasitic
operation where power and ground are connected to ground and the sensor
is powered via the data pin.

The enclosure has a 2 line by 16 character LCD display for displaying the sensor's
description and the measured temperature.  There is also a momentary switch
to switch between sensors to display and to blank the display. 

Measurements are made from each connected sensor, in turn, and the temperature
result, along with other data is transmitted via a 434MHz transmitter to my
Gertboard. 

Before transmitting any data, the user is presented with the device number
and description for each sensor in the system (whether or not it is actually
connected to the enclosure). the user is asked if all sensors are to be 
included in the run.  If the answer is negative, the user is asked which 
sensors are to be included. For those sensors that are included,
we determine if any are connected using parasitic power.  

For each connected sensor, the temperature is measured and 20 bytes of data are 
assembled to be transmitted.  When the data for the last sensor has been 
transmitted, the sketch starts over with the first sensor.

An LCD display will read out the description and temperature of one of the sensors.
Every time the temperature for that sensor is measured, the display is updated.
A switch is used to change the display to the next sensor.  If the switch is pressed
while displaying the last sensor, the display blanks.  The next switch press displays
the first sensor again.

The 20 bytes, for each sensor, are as follows:

byte 0: The number of sensors in the run
byte 1, and 2: scratchpad data - the measured temperature
byte 3: scratchpad data - upper temperature alarm
byte 4: scratchpad data - lower temperature alarm
byte 5: scratchpad data - resolution
byte 6 through 17: from the EEPROM - device description
byte 18: from the EEPROM - device number
byte 19: CRC

Note there are a suite of sketches for the user to add sensors to the system, input a sensor 
description, and to select the sensor's upper and lower temperature alarm settings, and the
sensor's resolution.  Refer to thepiandi.blogspot.com.

Foe each sensor to be run:
1.  The ROM code is retrieved from the EEPROM
2.  Sensor description is read from the EEPROM
3.  Check for Parasitic operation.  If so, retrieve the sensors resolution from the scratchpad
4.  Command the sensor to make the temperature conversion

Once the temperature is converted the following occurs:
1.  The scratchpad is downloaded
2.  Switch is checked to see if it is pressed.
3.  If the LCD selection matches the sensor that is being transmitted, the sensor's
description and current temperature is displayed.
4.	Data is transmitted as described next:

The transmission sequence is as follows: data starts with a 101010 pattern followed by 
eight synchronization bits: 11110000, and the two bits: 10.  This is followed by
the 20 bytes of data as described above.  The 20 bytes are transmitted at 600 bits/sec 
using Manchester encoding.  Encoded data is transmitted at twice that rate at 1200 baud.  
Manchester encoding ensures there will never be more than two 1's or two 0's 
in a row.  A 1 is represented as a transition of a 0 to a 1, while a 0 is a transition 
of a 1 to a 0.   Within the Manchester encoded data, there will only be two 1's or two 0's, 
in a row, if there is a bit change in the raw data.  Hence, the synchronization pattern 
is four 1's followed by four 0's, a pattern that can not exist within the Manchester encoded 
data.

When the last bit of the last byte has been transmitted, the line is pulled low, where
it stays until the next temperature conversion is completed.  The data transmission
starts anew with the next sensor.


MJL - thepiandi.blogspot.com   02/15/15
*/
#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"
#include "InputFromTerminal.h"
#include "LCD_2X16.h"
#include <avr/io.h>

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;
TERM_INPUT term_input;
LCD_2X16 lcd;

/*---------------------------------------------Global Variables-------------------------------------*/

byte rom[8];
byte scratchpad[9];
const int bit_time = 833;  //833 usec.  - 1200baud
int description[12];
int number_of_devices_to_run;
boolean parasitic = false;
int stored_devices;
boolean devices2run[50];
byte xmit_data[20];
char disp_temp[9];
int device_displayed = 1;

//Outout to 454MHz Transmitter Is On PB4
#define PORT_1 PORTB
#define PORT_PIN_1 4
#define DDR_1 DDRB

//Switch is on PB2
#define PORT_2 PORTB
#define PORT_PIN_2 2
#define DDR_2 DDRB
#define PIN_2 PINB
#define DATAMASK_2 0b00000100


/*---------------------------------------------yes_or_no------------------------------------------*/
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

/*---------------------------------------------how_many------------------------------------------*/
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

/*---------------------------------------------find_stored_devices----------------------------------*/
void find_stored_devices(){
  int address, character;
  
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
}

/*---------------------------------------------synchronize-------------------------------------*/
void synchronize(){
  // send 101010 as beginning pattern  
  for (int i = 0; i < 3; i++){
    PORT_1 |= (1 << PORT_PIN_1);
    delayMicroseconds (bit_time);
    PORT_1 &= ~(1 << PORT_PIN_1);
    delayMicroseconds (bit_time);
  }
  
  //  send 11110000 as sync pattern
  PORT_1 |= (1 << PORT_PIN_1);
  delayMicroseconds(bit_time * 4);
  PORT_1 &= ~(1 << PORT_PIN_1);
  delayMicroseconds(bit_time * 4);
  
  // send 10
  PORT_1 |= (1 << PORT_PIN_1);
  delayMicroseconds (bit_time);
  PORT_1 &= ~(1 << PORT_PIN_1);
  delayMicroseconds (bit_time);
}

/*---------------------------------------------send_data-------------------------------------*/
void send_data(byte xmit_byte){
  for (int i = 0; i < 8; i++){
		// Data transmitted using Manchester Encoding
    // we start with MSB first
    // each raw bit becomes two transmitted bits.
    // a 1 becomes a 01, a 0 becomes a 10
    if (xmit_byte > 127){  // true if MSB is a 1
      PORT_1 &= ~(1 << PORT_PIN_1);  //xmit 0
      delayMicroseconds (bit_time);
      PORT_1 |= (1 << PORT_PIN_1);    //xmit 1
      delayMicroseconds (bit_time);
    }
    else{  // MSB is 0
      PORT_1 |= (1 << PORT_PIN_1);  //xmit 1
      delayMicroseconds (bit_time);
      PORT_1 &= ~(1 << PORT_PIN_1);  //xmit 0
      delayMicroseconds (bit_time);
    }
    xmit_byte <<= 1; // left shift i position    
  }
}

/*---------------------------------------calculate_CRC_byte-------------------------------------*/
int calculateCRC_byte(byte inByte[], int num_bytes){
  byte workingByte[num_bytes];
  int crc = 0;
  
  for (int i = 0; i < num_bytes; i++){
    workingByte[i] = inByte[i];
    for (int j = 0; j < 8; j++){
      if ((crc % 2) ^ (workingByte[i] %2)){
        crc ^= 0b100011000;
      }
      crc >>= 1;
      workingByte[i] >>= 1;
    }   
  }
  return crc;
}

/*---------------------------------------query_user-------------------------------------*/
void query_user(){
  int i, j, k;
  int address;
  boolean make_run = false;
  int character;

  find_stored_devices();
  number_of_devices_to_run = stored_devices;
  
  //Interface with user. Keep in loop until user is satisfied with choices
  do{
    //Set up array for devices to run as all true  
    for (i = 0; i < stored_devices; i++){
      devices2run[i] = true;
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
      Serial.print(" Run this device? ");
      if (devices2run[i]){
        Serial.println("Yes");
      }
      else{
        Serial.println("No");
      }
      address += 20;
    }
    Serial.println("");
     
    Serial.println("OK To Make Run? ");
    make_run = yes_or_no();
    if (!make_run){
      Serial.println("");
    }
      
  }while (!make_run);
  Serial.println("");
  
  if (number_of_devices_to_run){
    Serial.println("Transmitting Temperature Data To Raspberry Pi");
  }
  else{
    Serial.println("Goodby");
  }
}

/*---------------------------------------check_switch_closed------------------------------*/
boolean check_switch_closed(){
  byte switch_input;
  boolean switch_closed = false;
  
  switch_input = PIN_2 & DATAMASK_2;  //returns 4 if not pressed, 0 if pressed
  if (!switch_input){
    switch_closed = true;   
  }
  return switch_closed;
}
/*---------------------------------------convert_temp_to_array---------------------------*/
//Required to properly display the temperature on the LCD display.
void convert_temp_to_array(float temp_f){
  if (temp_f < 0){
    if (abs(temp_f) > 100.0){
      disp_temp[0] = '-';
    }
    else if (abs(temp_f) > 10.0){
      disp_temp[0] = ' ';
      disp_temp[1] = '-';  
    }
    else{
      disp_temp[0] = ' ';
      disp_temp[1] = ' ';
      disp_temp[2] = '-';
    }
    temp_f *= -1.0;
  }
  else{
    if (temp_f > 100.0){
      disp_temp[0] = ' ';
    }
    else if (temp_f > 10.0){
      disp_temp[0] = ' ';
      disp_temp[1] = ' ';  
    }
    else{
      disp_temp[0] = ' ';
      disp_temp[1] = ' ';
      disp_temp[2] = ' ';
    }
  }
  temp_f += 0.05;
    
  if (temp_f >= 100.0){
    disp_temp[1] = char(temp_f/100 + 48); 
    temp_f = temp_f - (int(temp_f / 100) * 100);

    disp_temp[2] = char(temp_f/10 + 48);
    temp_f = temp_f - (int(temp_f / 10) * 10);
  }

  if (temp_f >= 10.0){
    disp_temp[2] = char(temp_f/10 + 48);
    temp_f = temp_f - (int(temp_f / 10) * 10);
  }
  
  disp_temp[3] = char(temp_f/1 + 48);
  temp_f = temp_f - int(temp_f);

  disp_temp[4] = '.';

  disp_temp[5] = char(temp_f/.1 + 48);  
  
  disp_temp[6] = 223;
  
  disp_temp[7] = 'F';
    
  disp_temp[8] = '\n';
}


/*_________________________________________MAIN PROGRAM__________________________________*/
/*---------------------------------------------setup-------------------------------------*/
void setup(){
  Serial.begin(9600);

  //Make Output to transmitter an output
  DDR_1 = (1 << PORT_PIN_1); 
  PORT_1 &= ~(1 << PORT_PIN_1);  //Set to 0 orignially
 
  //Make the pin to the switch an input
  DDR_2 &= ~(1 << PORT_PIN_2);

  //initialize the LCD display
  lcd.lcd_init();
  
  query_user();
    
}

/*---------------------------------------------loop-------------------------------------*/
void loop(){
  int i, j, k;
  int address;
  int resolution;
  int sequence = 1;
  char display_description[13];
  float temp_c, temp_f;
  boolean spaces;
  
  for (j = 0; j < stored_devices; j++){  
    if (devices2run[j]){  //check to see if device is to be run
      address = 20 * j +4;   //Get ROM code from EEPROM
      for (k = 0; k < 8; k++){
        rom[k] = eeprom.EEPROM_read(address + k);
      } 

      address += 8;
      for (k = 0; k < 12; k++){
        description[k] = eeprom.EEPROM_read(address + k);
        if (description[k] == 255){
          description[k] = 10;
        }
      }
      
      if (parasitic){
        //If parasitic operation then must find resolution for each device
        //Match ROM followed by read Scratchpad for resolution
        if (ds18b20.initialize()){
          Serial.println("Initialization Failure on Match ROM");
        }
        else{
          ds18b20.match_rom(rom); 
          //Read scratchpad and check CRC
          ds18b20.read_scratchpad(scratchpad); 
          if (ds18b20.calculateCRC_byte(scratchpad, 9)){
            Serial.println("Scratchpad failed CRC");
          }
          else{
            resolution = scratchpad[4];
            resolution = (resolution >> 5) + 9;
          }
        }
      }
      else{
        resolution = 12;  //dummy.  If not parasitic, this will be ignored by convertT
      }
      
      //Match ROM followed by Convert T
      if (ds18b20.initialize()){
        Serial.println("Initialization Failure on Convert Temperature");
      }
      ds18b20.match_rom(rom);
      ds18b20.convert_t(parasitic, resolution);
         
      //Match ROM followed by Read Scratchpad
      if (ds18b20.initialize()){
        Serial.println("Initialization Failure on Read Scratchpad");
      }
      ds18b20.match_rom(rom);
      ds18b20.read_scratchpad(scratchpad);
       
      //Check scratchpad CRC
      if (ds18b20.calculateCRC_byte(scratchpad, 9)){
        Serial.println("Scratchpad CRC Error");
      }
      
      //Display description and temperature
      if (check_switch_closed()){
        while (check_switch_closed());  //looking for release of switch
        device_displayed ++;
        if (device_displayed > number_of_devices_to_run){
          device_displayed = 0;
        }
      }
      
      //Prepare description for LCD display
      spaces = false;
      if (device_displayed == sequence){
        for (k = 0; k < 12; k++){
          if (description[k] == 10){
            spaces = true;
          }
          if (!spaces){
            display_description[k] = char(description[k]);
          }            
          else{
            display_description[k] = ' ';                        
          }
        }
        display_description[k] = '\n';


        //Get temperature and prepare for LCD display
        temp_c = (256 * scratchpad[1] + scratchpad[0]);
        if (scratchpad[1] > 127){  //Temperature below zero
          temp_c -= 65536;          //calculates 2's complement
        }
        temp_c /= 16.0;
        temp_f = 1.8 * temp_c + 32.0;
        
        convert_temp_to_array(temp_f);
            
        lcd.lcd_message(display_description, 1);
        lcd.lcd_message(disp_temp, 2);
      }
      else if(device_displayed == 0){
        lcd.clear_display();
      }
    
      //Assemble xmit_data array and tack on CRC byte

      //first byte is the number of devices to run
      xmit_data[0] = number_of_devices_to_run;

      //next comes 6 scratchpad bytes
      for (i = 0; i < 5; i++){
        xmit_data[i + 1] = scratchpad[i];
      }
      
      //next is the description
      for (i = 0; i < 12; i++){
        xmit_data[i + 6] = description[i];
      }
            
      //device number goes here      
      xmit_data[18] = j + 1;
      
      //last byte is the cRC byte
      xmit_data[19] = calculateCRC_byte(xmit_data, 19);
      
      
      // send synchronization pattern to Pi 
      synchronize();
    
      // send xmit_data data to Pi
      for (i = 0; i < 20; i++){
        send_data(xmit_data[i]);
      }      
          
      //PORTD &= ~(1 << 2);  // after last bit xmit a 0
      PORT_1 &= ~(1 << PORT_PIN_1);  //after last bit xmit 0
      delay(100);
      
      sequence ++;
      if (sequence > number_of_devices_to_run){
        sequence = 1;
      }
    }
  }
}

