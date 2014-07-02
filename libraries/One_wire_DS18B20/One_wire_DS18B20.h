
/* 
Please see Maxim's datasheet for this device to understand the 
functionality of the 1-wire interface and the device itself.

http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 
MJL - thepiandi.blogspot.com  05/09/14
*/

#ifndef DS18B20_INTERFACE_H
#define DS18B20_INTERFACE_H

#include "Arduino.h"
#include <avr/io.h>

/* Clock factor is to correct delayMicrosecond.  For Gertboard use .67.
For Arduion use 1 */

#define CLOCK_FACTOR 1.0

/* We need to define which ATmega328 pins to use to control the 
DS18B20s and the hard pull-up for parasitic operation. See Atmega328 datasheet 
pages 91 and 92 register names. This controls the following 10 selections: */

/*PORT_PIN and PORT_PIN_1 are the numbers of the ATmega328 pin within the port you have
selected.  The choices are 0 - 7. PB5 would be 5 */

#define PORT_PIN 0     //For DS18B20 data pins
#define PORT_PIN_1 1   //For Hard Pull-up transistor

/* DDR is the data direction register for ATmega328 pins you select.
Choices are DDRB, DDRC, and DDRD */

#define DDR DDRB       //For DS18B20 data pins
#define DDR_1 DDRB     //For Hard Pull-up transistor

/* PORT is the port data register controlling the ATmega328
pinS you have connected to the DS18B20 and the hard pull-up transistor. 
Choices are PORTB, PORTC, and PORTD */

#define PORT PORTB       //For DS18B20 data pins
#define PORT_1 PORTB     //For Hard Pull-up transistor	

/* PIN is the input register for the ATmega328 pin you select for the DS18B20 devices. Not necessary
for hard pull-up transistor. 
Choices are PINB, PINC, and PIND */

#define PIN PINB

/*DATAMASK selects the ATmega328 pin you have selected to connect to the 
DS18B20.  Not necessary for hard pull-up transistor. 
For example, if you are using pin PB3, use 0b00000100 */

#define DATAMASK 0b00000001


class DS18B20_INTERFACE{
  public:
    DS18B20_INTERFACE();
    boolean calculateCRC_byte(byte inByte[], int num_bytes);
    boolean initialize();
    void read_rom(byte rom_value[]);
    void skip_rom();
    void match_rom(byte rom_value[]);
    void read_scratchpad(byte scratchpad_value[]);
    int convert_t(boolean parasitic, int resolution);
    void write_scratchpad(byte alarm_and_configuration[]);
    int search_rom(byte Rom_no[], int LastDiscrepancy);
    int alarm_search(byte Rom_no[], int LastDiscrepancy);
		void copy_scratchpad(boolean parasitic);
		int recall_eeprom();
		boolean read_power_supply();
  private:
    void master_write0();
    void master_write1();
    void write_byte(byte dataword);
    byte master_read();
    byte read_byte();
    int search_device(byte Rom_no[], int LastDiscrepancy, byte code);

};

#endif
      

