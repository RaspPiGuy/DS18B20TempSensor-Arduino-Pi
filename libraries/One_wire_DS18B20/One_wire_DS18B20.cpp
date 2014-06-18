
/*  
Please see Maxim's datasheet for this device to understand the functionality of 
the 1-wire interface and the device itself.

http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf

To understand the functions in this file, it important that you know how 
ATmega328 register programming is done.  

Please visit my blog entry of January 19, 2014, "Gertboard - Programming With 
ATmega Registers", subtitled "Arduino IDE Built-in Functions Vrs. Direct Register
Programming"

http://thepiandi.blogspot.com/2014/01/gertboard-programming-with-atmega.html

MJL - thepiandi.blogspot.com  05/09/14
*/

#include "One_wire_DS18B20.h"
#include "Arduino.h"
      
DS18B20_INTERFACE::DS18B20_INTERFACE(){
}

//-----------------------calculateCRC_byte()------------------------------------

/* The CRC is checked after reading the ROM code and reading the scrathpad.  
The DS18B20 device appends its internal calculation of the CRC as the last byte 
of the ROM code and the scratchpad. If the result of the CRC is "0", after 
passing in all the bits the CRC passes. */

boolean DS18B20_INTERFACE::calculateCRC_byte(byte inByte[], int num_bytes){
  byte workingByte[num_bytes];
  boolean failure = false;
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
  if (crc) failure = true;
  return failure;
}
//-----------------------initialize()-----------------------------------------

/* Function called by user.

Initialization precedes all command sequences.  The data pin is pulled low for 
500us. then the pin switched to input. After 75usec. the data pin is read.
If "0" is read, initialization passes.  The entire initialization process is 
programmed to take 1ms. */

boolean DS18B20_INTERFACE::initialize(){
  boolean failure = false;  
  int ret_val;  
  PORT &= !(1 << PORT_PIN);  //Data pin = 0    
  DDR |= (1 << PORT_PIN);  //Data pin is OUTPUT  
  delayMicroseconds(500 / CLOCK_FACTOR);  
  DDR &= !(1 << PORT_PIN);  //Data pin is INPUT  
  delayMicroseconds(75 / CLOCK_FACTOR);  //Making this 600 will cause failure.GOOD  
  ret_val = PIN & DATAMASK; //Read input register and mask for your pin
  delayMicroseconds(425 / CLOCK_FACTOR); 
  if (ret_val == 1) failure = true; //if true device did not pull data pin down
  return failure;
}

//-----------------------master_write0()-----------------------------------

/* A "0" is written to the DS18B20 by the pulling the data pin low for 60usec. 
and then releasing it (make data pin an input). An additional 7usec. makes 
this operation one time slot long. */
 
void DS18B20_INTERFACE::master_write0(){  
  PORT &= !(1 << PORT_PIN);  //Data pin = 0    
  DDR |= (1 << PORT_PIN);  //Data pin is OUTPUT  
  delayMicroseconds(60 / CLOCK_FACTOR);  
  DDR &= !(1 << PORT_PIN);  //Data pin is INPUT  
  delayMicroseconds(7 / CLOCK_FACTOR);  
}

//-------------------------master_write1()------------------------------------

/* A "1" is written to the DS18B20 by the pulling the data pin low for 13usec. 
and then releasing it (make data pin an input). An additional 54usec. makes 
this operation one time slot long. */

void DS18B20_INTERFACE::master_write1(){  
  PORT &= !(1 << PORT_PIN);  //Data pin = 0     
  DDR |= (1 << PORT_PIN);  //Data pin is OURPUT  
  delayMicroseconds(13 / CLOCK_FACTOR);  
  DDR &= !(1 << PORT_PIN);  //Data pin is INPUT 
  delayMicroseconds(54 / CLOCK_FACTOR);   
}  
//-------------------------write_byte()-----------------------------------------

/* Writing a byte is done least significant bit first.  The least significant bit
 is tested by seeing if the byte is even or odd using modulo 2.  If odd, 
master_write1 is called, if not, master_write0 is called.  A right shift is done 
to move all the bits down one position. This is repeated until all 8 bits are handled. */

void DS18B20_INTERFACE::write_byte(byte dataword){  
  for (int i=0; i<8; i++){    
    if (dataword % 2){
       master_write1();
    }
    else{
      master_write0();
    } 
    dataword >>= 1;    
  }
}
//--------------------------master_read()---------------------------------------

/* This constitutes a read timeslot and reads one bit sent by the DS18B20.  The 
data pin is pulled low for 5 usec. then released. Data pin is read 6 usec. later 
to determine if the DS18B20 has pulled it low or left it high.  Time slot continues 
for 50usec. more. */
  
byte DS18B20_INTERFACE::master_read(){  
  byte ret_val;  
  PORT &= !(1 << PORT_PIN);  //Datapin = 0    
  DDR |= (1 << PORT_PIN);  //Datapin is OUTPUT    
  delayMicroseconds(5 / CLOCK_FACTOR); 
  DDR &= !(1 << PORT_PIN);  //Datapin is INPUT  
  delayMicroseconds(6 / CLOCK_FACTOR);   
  ret_val = PIN & DATAMASK; //Read input register and mask for your pin  
  delayMicroseconds(50 / CLOCK_FACTOR);  
  return ret_val;  
}
//-------------------------read_byte()------------------------------------------

/* Since the DS18B20 transmits least significant bit first, each bit is entered 
as a "0" or "1" in the most significant bit position and all the bits are shifted 
right one position.  Repeated until all 8 bits are read.*/
  
byte DS18B20_INTERFACE::read_byte(){
  byte dataword = 0;
  for (int i=0; i<8; i++){
    dataword >>= 1;    
    if (master_read()){
      dataword += 0b10000000;
    }
  }  
  return dataword;  
}
//--------------------------read_rom()-----------------------------------------

/* Function called by user.

WARNING: Do not use this function if more than one device is connected to the 
1-wire bus.

The ROM code is 8 bytes long including the CRC byte.  Eight bytes are read from
 the DS18B20. Each byte becomes an element in an array.  The CRC should be checked after this function.
*/

void DS18B20_INTERFACE::read_rom(byte rom_value[]){
  write_byte(0x33);
  for (int i=0; i<8; i++){
    rom_value[i] = read_byte();
  }
}
//--------------------------skip_rom()-----------------------------------------

/* Function called by user.

Skip ROM allows commands to be sent without sending the ROM contents.
Initialization must be sent before this command.*/

void DS18B20_INTERFACE::skip_rom(){
  write_byte(0xCC);
}
//--------------------------match_rom()-----------------------------------------

/* Function called by user.

Match rom precedes a convert temperature or a read scratchpad command.  Its purpose
is to send the ROM code to the DS18B20. Initialization must be sent before this 
command. */

void DS18B20_INTERFACE::match_rom(byte rom_value[]){
  write_byte(0x55);
  for (int i=0; i<8; i++){
    write_byte(rom_value[i]);
  }
}
//---------------------------read_scratchpad()---------------------------------

/* 
Function called by user.

Reading the scratchpad is done after a skip rom or match rom.  It reads 9 bytes
from the DS18B20.  The last byte is the CRC byte.  The 9 bytes become elements in
an array.  The CRC should be checked after this function. */

void DS18B20_INTERFACE::read_scratchpad(byte scratchpad_value[]){  
  write_byte(0xBE);
  for (int i=0; i<9; i++){
    scratchpad_value[i] = read_byte();
  }
}
//-------------------------convert_t()------------------------------------------

/* Function called by user.

This commands the DS18B20 to make a voltage reading, convert that reading to 
temperature, and store that value in the two most significant bytes of the scratchpad. 
 
For non-parasitic operation, After writing the code to command the convert temperature 
process, a read timeslot is initiated every ms.  Once a "1" is received, the function 
exits with the number of ms. returned to the calling program. If the number of ms. 
is zero (data pin never went to "0"), the conversion never took place.  If it stays "0" 
for more than 2 seconds, the function does not wait any longer and exits.  
The conditions of time of zero or more than 2000 should be tested
for by the calling program and declared a failed situation. 

For parasitic operation, the DS18B20'S data pin must be held high for the duration of the 
maximum time specified for each resolution.  The resistor pull-up does not provide 
sufficient current for that operation so a transistor is turned on that pulls the DS18B20's 
data pin up to Vcc.  This is called a hard pull-up.  In parasitic mode, the function
does not return anything.
*/
 
int DS18B20_INTERFACE::convert_t(boolean parasitic, int resolution){
	if (parasitic){
  	PORT_1 &= !(1 << PORT_PIN_1); //Data pin for hard pull-up pulled low
		write_byte(0x44);
		DDR_1 |= (1 << PORT_PIN_1);   //Pin controlling hard pull-up is output truning transistor on
		delay(94 * pow(2, (resolution - 9)));
		DDR_1 &= (1 << PORT_PIN_1);   //Pin controlling hard pull-up is input turning transistor off
	}
	else{
		byte value;
		int count = -1;
		write_byte(0x44);
		do{
			count++;
			if (count > 2000){
				break;
			}
			value = master_read();
			delay(1);
		} while (value == 0); 
		return (count); 
	}
}

//-------------------------------write_scratchpad()-----------------------------

/* Function called by user.

This command writes three bytes to the scratchpad.  The first byte is the upper 
temperature threshold, the second is the lower temperature threshold, and the third 
is the configuration byte.  The configuration byte has only two useful bits and 
they are used to set the resolution as 9, 10, 11, or 12 bits.  Skip rom or match 
rom must be sent before this command.
*/

void DS18B20_INTERFACE::write_scratchpad(byte alarm_and_configuration[]){
  write_byte(0x4E);
  for (int i=0; i<3; i++){
    write_byte(alarm_and_configuration[i]);
  }
}


//-------------------------------search_rom()-----------------------------

/* This  function runs the basic algorithm as
outlined in Maxim's Application Note 187, "1-Wire Search Algorithm".  
Function finds one device and stores it's code in the array New_Rom_no.
It returns the variable LastDescrepanvy.  

The function is passed the array Rom_no and the variable LastDescrepancy.  
If this is the first time the function is run, Rom_no will contain eight 
zeros and LastDescrepancy will be 0.  If this function finds a device, it
is called again with the last values of Rom_no and LastDescrepancy.  If the
function finds the last device connected to 1-wire, Lastdescrepancy will be
zero.  The ROM code of the newly discovered device goes into the array New_Rom_no.  
*/

int DS18B20_INTERFACE::search_rom(byte Rom_no[], byte New_Rom_no[], int LastDescrepancy){
  int Rom_byte_mask;
  int Rom_bit_mask;
  boolean LastDeviceFound = false;
  byte search_direction;
  int bit_number;
  int last_zero;
  byte id_bit, cmp_id_bit;
  
  Rom_byte_mask = 0;  //To access the correct byte in Rom_no
  Rom_bit_mask = 0;   //To access correct bit in Rom_no
  last_zero = 0;
  bit_number = 1;
  write_byte(0xF0);  //send code for search command
  
  do {                       //Do Until All 64 Bits Are Accessed
    id_bit = master_read(); 
    cmp_id_bit = master_read();      
    if (id_bit == cmp_id_bit){   //There is a descrepancy
      if (bit_number == LastDescrepancy){
        search_direction = 1;
      }
      else if (bit_number > LastDescrepancy){
        search_direction = 0;
        last_zero = bit_number;
      }
      else{   // bit_number must be less that LastDescrepancy
        if (Rom_no[Rom_byte_mask] & (1 << Rom_bit_mask)){   //Take on the value of same bit in Rom_no
          search_direction = 1;
        }
        else{
          search_direction = 0;
          last_zero = bit_number;
        }
      }
    }
    else{                         //There is no descrepancy
      search_direction = id_bit;
    }
    if (search_direction){      //We are at the compare stage, write to the device, one bit.  Some devices may drop out
      master_write1();
    }
    else{
      master_write0();
    }
     if (search_direction){        //Now write that same bit into ROM ID at the present byte and bit position
      New_Rom_no[Rom_byte_mask] |= (1 << Rom_bit_mask);   //Set bit to 1
    }
     
    Rom_bit_mask++;      //Shift to the next bit position in the ROM ID
    if (Rom_bit_mask > 7){  //If we have gone beyond the byte boundry, move to the next byte
      Rom_byte_mask++;
      Rom_bit_mask = 0;      
    }  
    bit_number++;
    
   }while (bit_number < 65);
 
  LastDescrepancy = last_zero;
  return LastDescrepancy;
}


  
  
