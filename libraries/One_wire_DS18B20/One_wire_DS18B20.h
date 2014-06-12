
/* 
Please see Maxim's datasheet for this device to understand the 
functionality of the 1-wire interface and the device itself.

http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 
MJL - thepiandi.blogspot.com  05/09/14
*/

#ifndef DS18B20_INTERFACE_H
#define DS18B20_INTERFACE_H

#include "Arduino.h"

/* Clock factor is to correct delayMicrosecond.  For Gertboard use .67.
For Arduion use 1 */

#define CLOCK_FACTOR 1.0

/* We need to define which ATmega328 pin to use to control the 
DS18B20. See Atmega328 datasheet pages 91 and 92 register names.
This controls the following 5 selections: */

/*PORT_PIN is the number of the ATmega328 pin within the port you have
selected.  The choices are 0 - 7. PB5 would be 5 */

#define PORT_PIN 0

/* DDR is the data direction register for ATmega328 pin you select.
Choices are DDRB, DDRC, and DDRD */

#define DDR DDRB

/* PORT is the port data register controlling the ATmega328
pin you have connected to the DS18B20. Choices are PORTB, PORTC,
and PORTD */

#define PORT PORTB

/* PIN is the input register for the ATmega328 pin you select. 
Choices are PINB, PINC, and PIND */

#define PIN PINB

/*DATAMASK selects the ATmega328 pin you have selected to connect to the 
DS18B20.  For example, if you are using pin PB3, use 0b00000100 */

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
    int convert_t();
    void write_scratchpad(byte alarm_and_configuration[]);
    int search_rom(byte Rom_no[], byte New_Rom_no[], int LastDescrepancy);
 
  private:
    void master_write0();
    void master_write1();
    void write_byte(byte dataword);
    byte master_read();
    byte read_byte();

};

#endif
      

