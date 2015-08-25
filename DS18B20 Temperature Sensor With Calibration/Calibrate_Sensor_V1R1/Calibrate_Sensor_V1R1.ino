/*
Obtains Calibration Factors for the DS18B20 Sensors

Only one sensor can be attached to the temperature endlosure at a time.

Operator first puts sensor into ice water.  Temperature will read out on the LCD display in
degrees centigrade to four decimal places.

When operator is satisfied that the temperature has stabilized he presses the switch.

If the temperature has reached 0 deg. C +/- 4.0 deg. C, operator is asked if if he wants to
accept value.  If so he presses the switch again.  If he does not press the switch, we move
on to the next part.  

If the operator has pressed the switch to accept the calibration, the calibration factor is
written to the EEPROM at location 21.

The operator next puts the sensor into boiling water and the above procedure is repeated.
The high temperature factor is stored in location 22.

*/


#include "One_wire_DS18B20.h"
#include "EEPROM_Functions.h"
#include "LCD_2X16.h"
#include <avr/io.h>

DS18B20_INTERFACE ds18b20;
EEPROM_FUNCTIONS eeprom;
LCD_2X16 lcd;

/*---------------------------------------------Global Variables-------------------------------------*/

byte rom[8];
byte scratchpad[9];
const int bit_time = 833;  //833 usec.  - 1200baud
int description[12];
boolean parasitic = false;
int stored_devices;
boolean devices2run[50];
byte xmit_data[20];
char disp_temp[16];
int device_displayed = 1;
int selected_sensor;

//Switch is on PB2
#define PORT_2 PORTB
#define PORT_PIN_2 2
#define DDR_2 DDRB
#define PIN_2 PINB
#define DATAMASK_2 0b00000100

/*---------------------------------------detect_devices-------------------------------------*/
int detect_devices(){
  boolean done = false;
  int i, j;
  int address;
  int connected_devices;

  //How Many devices stored in the EEPROM?
  stored_devices = eeprom.EEPROM_read(3); // 3 = address location with number of devices
  connected_devices = stored_devices;

  for (i = 0; i < stored_devices; i++){
    devices2run[i] = true;
  }

  for (i = 0; i < stored_devices; i++){
    address = 22 * i + 4;   //Get ROM code from EEPROM
    for (j = 0; j < 8; j++){
      rom[j] = eeprom.EEPROM_read(address + j);
    } 
    ds18b20.initialize();
    ds18b20.match_rom(rom);      
    ds18b20.read_scratchpad(scratchpad); 
 
    if (ds18b20.calculateCRC_byte(scratchpad, 9)){  //CRC failed if this is true.  Device not present
        devices2run[i] = false;
        connected_devices--;
    }
    else{
      selected_sensor = i;
    }
  }
  return connected_devices;
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
/*---------------------------------------check_switch_open------------------------------*/
boolean check_switch_open(){
  byte switch_input;
  boolean switch_open = false;
    
  switch_input = PIN_2 & DATAMASK_2;  //returns 4 if not pressed, 0 if pressed
  if (switch_input){
    switch_open = true;
  }
  return switch_open;
}
/*---------------------------------------convert_temp_to_array---------------------------*/
//Required to properly display the temperature on the LCD display.
void convert_temp_to_array(float temp_c){
  if (temp_c < 0){
    if (abs(temp_c) >= 100.0){
      disp_temp[5] = '-';
    }
    else if (abs(temp_c) > 10.0){
      disp_temp[5] = ' ';
      disp_temp[6] = '-';  
    }
    else{
      disp_temp[5] = ' ';
      disp_temp[6] = ' ';
      disp_temp[7] = '-';
    }
    temp_c *= -1.0;
  }
  else{
    if (temp_c > 100.0){
      disp_temp[5] = ' ';
    }
    else if (temp_c > 10.0){
      disp_temp[5] = ' ';
      disp_temp[6] = ' ';  
    }
    else{
      disp_temp[5] = ' ';
      disp_temp[6] = ' ';
      disp_temp[7] = ' ';
    }
  }
    
  if (temp_c >= 100.0){
    disp_temp[6] = '1'; 
    temp_c = temp_c - 100;

    if (temp_c < 10){
      disp_temp[7] = '0';
    }
  }

  if (temp_c >= 10.0){
    disp_temp[7] = char(temp_c/10 + 48);
    temp_c = temp_c - (int(temp_c / 10) * 10);
  }
  
  disp_temp[8] = char(temp_c/1 + 48);
  temp_c = 10000 * (temp_c - int(temp_c));

  disp_temp[9] = '.';
  
  disp_temp[10] = char(temp_c/1000 + 48);  
  temp_c = temp_c - int(temp_c / 1000) * 1000;
  
  disp_temp[11] = char(temp_c/100 + 48);
  temp_c = temp_c - int(temp_c / 100) * 100;
  
  disp_temp[12] = char(temp_c/10 + 48);  
  temp_c = temp_c - int(temp_c / 10) * 10;
  
  disp_temp[13] = char(temp_c/1 + 48);
}
/*-----------------------------------------measure temperature-------------------------------------*/
float measure_temperature(){
  float temp_c;
  
  ds18b20.initialize();
  ds18b20.match_rom(rom);
  ds18b20.convert_t(parasitic, 12);
      
  //Get the result from the scratchpad
  ds18b20.initialize();
  ds18b20.match_rom(rom);
  ds18b20.read_scratchpad(scratchpad);
  
  //Get temperature and prepare for LCD display
  temp_c = (256 * scratchpad[1] + scratchpad[0]);
  if (scratchpad[1] > 127){  //Temperature below zero
    temp_c -= 65536;          //calculates 2's complement
  }
  temp_c /= 16.0;   
  return temp_c;  
}

/*_________________________________________MAIN PROGRAM__________________________________*/
/*---------------------------------------------setup-------------------------------------*/
void setup() {
  int i;
  int address;
  char display_description[13];
  float temp_c;
  int stored_resolution = 0x7F;
  byte alarm_and_config[3];
  byte calibration_ice = 0;
  byte calibration_boil = 0;
  float temp_ice;  //debug

  Serial.begin(9600);
  
  //Make the pin to the switch an input
  DDR_2 &= ~(1 << PORT_PIN_2);

  //initialize the LCD display
  lcd.lcd_init();
  
  
  if (detect_devices() == 0){
    lcd.lcd_message("No Sensors Found\n", 1);
    lcd.lcd_message("                \n",2);  
  }
  else if (detect_devices() > 1){
    lcd.lcd_message("Only One Sensor \n", 1);
    lcd.lcd_message("Connected Please\n", 2);
  }
  else{
    //check for parasitic power     
    if (!ds18b20.read_power_supply()){  //A zero means parasitic
      parasitic = true;
    }
    //find selected sensor in EEPROM
    address = 22 * selected_sensor + 4;
    
    //get ROM code from EEPROM
     for (i = 0; i < 8; i++){
       rom[i] = eeprom.EEPROM_read(address + i);
     } 
     
     //get description from EEPROM
     address += 8;
     for (i = 0; i < 12; i++){
       description[i] = eeprom.EEPROM_read(address + i);
       if (description[i] == 255){
         description[i] = 10;
       }
     }
     
     //display description
     for (i = 0; i < 12; i++){
       if (description[i] == 10){
         display_description[i] = ' ';
       }
       else{
         display_description[i] = char(description[i]);
       }
     }
     display_description[i] = '\n';
     
     lcd.lcd_message(display_description, 1);
     
     //get resolution and store it.  We will be replacing it with 12 bits for calibration
     ds18b20.initialize();
     ds18b20.match_rom(rom); 
     ds18b20.read_scratchpad(scratchpad); 
     
     if (scratchpad[4] != 0x7F){
       stored_resolution = scratchpad[4];
       alarm_and_config[0] = scratchpad[2];  //upper temperature alarm
       alarm_and_config[1] = scratchpad[3];  //lower temperature alarm
       alarm_and_config[2] = 0x7F;  //12 bit resolution
       
       //write the three bytes to the scratchpad
       ds18b20.initialize();
       ds18b20.match_rom(rom); 
       ds18b20.write_scratchpad(alarm_and_config);      
     }

     disp_temp[0] = 'I';
     disp_temp[1] = 'c';
     disp_temp[2] = 'e';
     disp_temp[3] = ' ';
     disp_temp[4] = ' ';
     disp_temp[14] = 223;
     disp_temp[15] = 'C';    
     disp_temp[16] = '\n';
   
     //Sensor should be in ice water
     while(1){
       //measure and display temperature
       temp_c = measure_temperature();
       convert_temp_to_array(temp_c);
       lcd.lcd_message(disp_temp, 2);
      
       if(check_switch_closed()){
         break;
       }
     }
     
     //wait until switch is open   
     while(1){
       if (check_switch_open()){
         break;
       }
     }

     //temp_c = 0.5;   //debug 
     if (temp_c > -4.0 && temp_c < 4.0){
       lcd.lcd_message("Press to Accept \n", 2);
       //if switch is pressed within 3 seconds. low calibration factor is stored
       for (i = 0; i < 12; i++){
         if (check_switch_closed()){
           if (temp_c < 0){
             calibration_ice = byte(16.0 * temp_c + 256);           
           } 
           else{
             calibration_ice = byte(16.0 * temp_c);           
           }
           
           //wait for switch to be open again
           while(1){
             if (check_switch_open()){
               break;
             }
           }
           
           //Write the calibration factor to the EEPROM
           address = 22 * selected_sensor + 24;  
           eeprom.EEPROM_write(address, calibration_ice);  
           
           lcd.lcd_message("Low Cal Accepted\n", 2);
           delay(1000);
           break;
         }
         else{
           delay(250);
         }
       }
     }
     else{
       lcd.lcd_message("Out of Range    \n", 2);
       delay(1000);
     }
     
     temp_ice = temp_c; //debug
     
     disp_temp[0] = 'B';
     disp_temp[1] = 'o';
     disp_temp[2] = 'i';
     disp_temp[3] = 'l';
     disp_temp[4] = ' ';
    
    //Sensor should be in boiling water
     while(1){
       //measure and display temperature
       temp_c = measure_temperature();
       convert_temp_to_array(temp_c);
       lcd.lcd_message(disp_temp, 2);
      
       if(check_switch_closed()){
         break;
       }
     }

     //wait until switch is open   
     while(1){
       if (check_switch_open()){
         break;
       }
     }
     
     //temp_c = 99.5; //debug
     if (temp_c > 96.0 && temp_c < 104.0){
       lcd.lcd_message("Press to Accept \n", 2);
       //if switch is pressed within 3 seconds. low calibration factor is stored
       for (i = 0; i < 12; i++){
         if (check_switch_closed()){
           if (temp_c < 100){
             calibration_boil = byte(16.0 * (temp_c - 100.0) + 256);           
           } 
           else{
             calibration_boil = byte(16.0 * (temp_c - 100.0));           
           }
           
           //wait for switch to be open again
           while(1){
             if (check_switch_open()){
               break;
             }
           }
           //Write the calibration factor to the EEPROM
           address = 22 * selected_sensor + 25;  
           eeprom.EEPROM_write(address, calibration_boil);                   
           
           lcd.lcd_message("Hi Cal Accepted \n", 2);
           delay(1000);
           break;
         }
         else{
           delay(250);
         }
       }
     }
     else{
       lcd.lcd_message("Out of Range    \n", 2);
       delay(1000);
     }

     //restore original resolution if it is not 12 bits.
     if (stored_resolution != 0x7F){
       alarm_and_config[2] = stored_resolution;
       
       //write the three bytes to the scratchpad
       ds18b20.initialize();
       ds18b20.match_rom(rom); 
       ds18b20.write_scratchpad(alarm_and_config);      
     }
     
     lcd.lcd_message("Calibration     \n",1);
     lcd.lcd_message("        Complete\n",2);

     //Debug   
     Serial.print("Temperature of Ice Water: ");
     Serial.print(temp_ice); 
     Serial.println(" deg C."); 
     Serial.print("Calibration Factor low: ");
     Serial.println(calibration_ice, HEX);
     Serial.print("Temperature of Boiling Water: ");
     Serial.print(temp_c); 
     Serial.println(" deg C."); 
     Serial.print("Calibration Factor high: ");
     Serial.println(calibration_boil, HEX);
       
   }
}

void loop() {
  // put your main code here, to run repeatedly:

}
