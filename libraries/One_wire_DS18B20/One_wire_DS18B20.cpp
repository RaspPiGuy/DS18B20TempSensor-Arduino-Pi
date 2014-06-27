
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

/*  THESE FUNCTIONS ARE PRIVATE AND ARE CALLED BY THE PUBLIC FUNCTIONS BELOW*/

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
//-------------------------------search_device()-----------------------------

/* 
Function called by search_rom() and alarm_search(). This does the actual work 
required of these functions.  The only difference between search_rom() and 
alarm_search() is they pass different command codes, or what Maxim calls protocol.
 
This  function runs the basic algorithm as outlined in Maxim's Application 
Note 187, "1-Wire Search Algorithm".  Function finds one device and stores 
it's code in the array New_Rom_no. It returns the variable LastDescrepanvy.  

The function is passed the array Rom_no and the variable LastDiscrepancy.  
If this is the first time the function is run, Rom_no will contain eight 
zeros and LastDiscrepancy will be 0.  If this function finds a device, it
is called again with the last values of Rom_no and LastDiscrepancy.  If the
function finds the last device connected to 1-wire, LastDiscrepancy will be
zero.  The ROM code of the newly discovered device goes into the array New_Rom_no.  
*/

int DS18B20_INTERFACE::search_device(byte Rom_no[], byte New_Rom_no[], int LastDiscrepancy, byte code){
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
  write_byte(code);  //send code for search command
  
  do {                       //Do Until All 64 Bits Are Accessed
    id_bit = master_read(); 
    cmp_id_bit = master_read();      
    if (id_bit == cmp_id_bit){   //There is a discrepancy
      if (bit_number == LastDiscrepancy){
        search_direction = 1;
      }
      else if (bit_number > LastDiscrepancy){
        search_direction = 0;
        last_zero = bit_number;
      }
      else{   // bit_number must be less that LastDiscrepancy
				//Take on the value of same bit in Rom_no
        if (Rom_no[Rom_byte_mask] & (1 << Rom_bit_mask)){ 
          search_direction = 1;
        }
        else{
          search_direction = 0;
          last_zero = bit_number;
        }
      }
    }
    else{                         //There is no discrepancy
      search_direction = id_bit;
    }
		//We are at the compare stage, write to the device, one bit.  Some devices may drop out
    if (search_direction){      
      master_write1();
    }
    else{
      master_write0();
    }
		//Now write that same bit into ROM ID at the present byte and bit position
    if (search_direction){        
      New_Rom_no[Rom_byte_mask] |= (1 << Rom_bit_mask);   //Set bit to 1
    }
     
    Rom_bit_mask++;      //Shift to the next bit position in the ROM ID
		//If we have gone beyond the byte boundry, move to the next byte
    if (Rom_bit_mask > 7){  
      Rom_byte_mask++;
      Rom_bit_mask = 0;      
    }  
    bit_number++;
    
   }while (bit_number < 65);
 
  LastDiscrepancy = last_zero;
  return LastDiscrepancy;
}
/* THE FOLLOWING FUNCTIONS ARE PUBLIC MEANING THEY CAN BE CALLED BY THE USER'S CODE*/  

//-----------------------calculateCRC_byte()------------------------------------

/* This function can be called by the user but  is not a ROM Command or a 
Function Command as outlined in the DS18B20 datasheet.

The CRC is checked after reading the ROM code and reading the scrathpad.  
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
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ROM Commands >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

//-----------------------------initialize()-----------------------------------------

/* ROM Command called by user.

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
//--------------------------read_rom()-----------------------------------------

/* ROM Command called by user.  Preceded by initialize(). 

WARNING: Do not use this function if more than one device is connected to the 
1-wire bus.

The ROM code is 8 bytes long including the CRC byte.  Eight bytes are read from
 the DS18B20. Each byte becomes an element in an array.  The CRC should be checked 
 after this function.
*/

void DS18B20_INTERFACE::read_rom(byte rom_value[]){
  write_byte(0x33);
  for (int i=0; i<8; i++){
    rom_value[i] = read_byte();
  }
}
//--------------------------skip_rom()-----------------------------------------

/* ROM command called by user.  Preceded by initialize().  

Skip ROM allows commands to be sent without sending the ROM contents. Useful 
for commanding all devices perform temperature conversion if followed by 
convert_t().  */

void DS18B20_INTERFACE::skip_rom(){
  write_byte(0xCC);
}
//--------------------------match_rom()-----------------------------------------

/* ROM command called by user.  Preceded by initialize(). 

Match rom precedes most Function Commands.  It is the method that selects the 
device the user wishes to address.  The only alternative ROM commands that can 
precede a Function Command are skip_rom(), if you want to address multiple
devices or read_rom(), used if there is no more than one device on the bus. */

void DS18B20_INTERFACE::match_rom(byte rom_value[]){
  write_byte(0x55);
  for (int i=0; i<8; i++){
    write_byte(rom_value[i]);
  }
}

//--------------------------search_rom()-----------------------------------------
/*  ROM command called by user.  Preceded by initialize().

Real work of this function is done by search_device.
*/

int DS18B20_INTERFACE::search_rom(byte Rom_no[], byte New_Rom_no[], int LastDiscrepancy){
	LastDiscrepancy = search_device(Rom_no, New_Rom_no, LastDiscrepancy, 0xF0);
	return LastDiscrepancy;
}

//--------------------------alarm_search()---------------------------------------
/* ROM command called by user.  Preceded by initialize().  

Real work of this function is done by search_device.
*/

int DS18B20_INTERFACE::alarm_search(byte Rom_no[], byte New_Rom_no[], int LastDiscrepancy){
	LastDiscrepancy = search_device(Rom_no, New_Rom_no, LastDiscrepancy, 0xEC);
	return LastDiscrepancy;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Function Commands >>>>>>>>>>>>>>>>>>>>>>>>*/

//-------------------------convert_t()------------------------------------------

/* Function Command called by user.  Must be preceded by match_rom(), skip_rom(), 
or if there is only one device on the bus, read_rom().  Generally, a read_scratchpad()
is called after this function.

This commands the DS18B20 to make a voltage reading, convert that reading to 
temperature, and store that value in the two most significant bytes of the scratchpad. 
 
For non-parasitic operation, after writing the code to command the convert temperature 
process, a read timeslot is initiated every ms.  Once a "1" is received, the function 
exits with the number of ms. returned to the calling program. If the number of ms. 
is zero (data pin never went to "0"), the conversion never took place.  If it stays "0" 
for more than 2 seconds, the function does not wait any longer and exits.  
The conditions of zero time or time greater than 2000 should be tested
for by the calling program and declared a failed situation. 

For parasitic operation, the DS18B20'S data pin must be held high for the duration of the 
maximum time specified for each resolution.  The resistor pull-up does not provide 
sufficient current for that operation so a transistor is turned on that pulls the DS18B20's 
data pin up to Vcc.  This is called a hard pull-up.  In parasitic mode, the function
does not return anything.
*/
 
int DS18B20_INTERFACE::convert_t(boolean parasitic, int resolution){
	if (parasitic){
		//Data pin for hard pull-up pulled low
  	PORT_1 &= !(1 << PORT_PIN_1); 
		write_byte(0x44);
		//Pin controlling hard pull-up is output turning transistor on
		DDR_1 |= (1 << PORT_PIN_1);   
		delay(94 * pow(2, (resolution - 9)));
		//Pin controlling hard pull-up is input turning transistor off
		DDR_1 &= (1 << PORT_PIN_1);   
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
//---------------------------read_scratchpad()---------------------------------

/*Function Command called by user.  Must be preceded by match_rom(), or if there is 
only one device on the bus, skip_rom(), or read_rom().

read_scratchpad reads 9 bytes from the DS18B20.  The last byte is the CRC byte.  
The 9 bytes become elements in an array.  The CRC should be checked after this 
function. */

void DS18B20_INTERFACE::read_scratchpad(byte scratchpad_value[]){  
  write_byte(0xBE);
  for (int i=0; i<9; i++){
    scratchpad_value[i] = read_byte();
  }
}
//-------------------------------write_scratchpad()-----------------------------

/* Function Command called by user.  Must be preceded by match_rom(), skip_rom(), 
or if there is only one device on the bus, read_rom().  

This command writes three bytes to the scratchpad.  The first byte is the upper 
temperature threshold, the second is the lower temperature threshold, and the third 
is the configuration byte.  The configuration byte has only two useful bits and 
they are used to set the resolution as 9, 10, 11, or 12 bits.  
*/

void DS18B20_INTERFACE::write_scratchpad(byte alarm_and_configuration[]){
  write_byte(0x4E);
  for (int i=0; i<3; i++){
    write_byte(alarm_and_configuration[i]);
  }
}
//-------------------------------copy_scratchpad()-------------------------------
/* Function Command called by user.  Must be preceded by match_rom(), skip_rom(), 
or if there is only one device on the bus, read_rom().  

Copies the contents of bytes 2, 3, and 4 of the scratchpad to the device's eeprom.
Those three bytes represent, respectively, upper alarm temperature, lower alarm
temperature, and the configuration register that contains the resolution.

For non-parasitic mode it is enough to just send the command.  However, for parasitic
mode, it is necessary to use the hard pull-up to pull the data pin to the positive 
rail for 10ms.  The hard pull-up consists of the pnp transistor controlled by the 
ATmega device.  This will be done in the same way as is done in the convert_t() function.
*/
void DS18B20_INTERFACE::copy_scratchpad(boolean parasitic){
	if (parasitic){
		//Data pin for hard pull-up pulled low
  	PORT_1 &= !(1 << PORT_PIN_1); 
		write_byte(0x48);
		//Pin controlling hard pull-up is output turning transistor on
		DDR_1 |= (1 << PORT_PIN_1);   
		delay(10);  //delay 10ms.
		//Pin controlling hard pull-up is input turning transistor off
		DDR_1 &= (1 << PORT_PIN_1);   
	}
	else{
		write_byte(0x48);
	}
}
//-------------------------------recall_eeprom()--------------------------------
/* Function Command called by user.  Must be preceded by match_rom(), skip_rom(), 
or if there is only one device on the bus, read_rom().  

Copies the alarm high, alarm low, and configuration register from the DS18B20's 
eeprom to the device's scratchpad.  

After writing the command, a read timeslot is initiated every ms.  Once a "1" is 
received, the function exits with the number of ms. returned to the calling 
program. If the number of ms. is zero (data pin never went to "0"), the process never 
took place.  If it stays "0" for more than 2 seconds, the function does not wait 
any longer and exits.  

The conditions of zero time or time greater than 2000ms. should be tested
for by the calling program and declared a failed situation. 

It may be rare that it is necessary to call this function as the device preforms this
operation upon power-up.
*/

int DS18B20_INTERFACE::recall_eeprom(){
	byte value;
	int count = -1;
	write_byte(0xB8);
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

//-------------------------------read_power_supply()-----------------------------
/* Function Command called by user.  Must be preceded by match_rom(), skip_rom(), 
or if there is only one device on the bus, read_rom().  

Purpose is to determine if a device is connected to the bus using parasitic power.
User may determine if ANY device is connected using parasitic power by preceding call
to this function by calling skip_rom().  User may also determine if a particular 
device is connected using parasitic power by preceding call to this function with 
match_rom().  If only one device is connected to the bus, read_rom() could preceed the
call to this function.  That has not been tested because it is kind of silly.  User
should know how his one device is connected without this function.  

After issuing command function calls master_read() which is the equivalent of 
read timeslot (datasheet terminology).  Any device that is connected for parasitic 
power will pull the bus low making master_read() return "0" or false.  If all devices
are conventionally powered, master_read() will return a non "0", or true. 

Returns boolean true or false according to the master_read() results.
*/

boolean DS18B20_INTERFACE::read_power_supply(){
	write_byte(0xB4);
	return(master_read());
}


  
  
