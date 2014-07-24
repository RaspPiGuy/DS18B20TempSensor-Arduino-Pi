
#ifndef LCD_2X16_H
#define LCD_2X16_H

#include "Arduino.h"
#include <avr/io.h>

#define RS_PORT PORTD //LCD Register Select (RS) pin
#define RS_PIN 2
#define RS_DDR DDRD

#define E_PORT PORTD  //LCD Enable (E) pin
#define E_PIN 3
#define E_DDR DDRD

#define D4_PORT PORTD    //LCD Data Port D4 pin
#define D4_PIN 4
#define D4_DDR DDRD

#define D5_PORT PORTD    //LCD Data Port D5 pin
#define D5_PIN 5
#define D5_DDR DDRD

#define D6_PORT PORTD    //LCD Data Port D6 pin
#define D6_PIN 6
#define D6_DDR DDRD

#define D7_PORT PORTD    //LCD Data Port D7 pin
#define D7_PIN 7
#define D7_DDR DDRD

#define LCD_WIDTH 16 //For 16x2 line display
#define LCD_LINE_1 0x80 //LCD DDRAM left hand address for first line
#define LCD_LINE_2 0xC0 //LCD DDRAM Left hand address for second line

#define LCD_CHR 1  //Mode for Character Data Register
#define LCD_CMD 0 //Mode for Command Data Register

#define CLOCK_FACTOR 1.0 //For Arduino 0.67 for Gertboard

class LCD_2X16 {
	public:
		LCD_2X16();
		void lcd_message(char message[], int line_no);
		void lcd_init();
		void clear_display();
		void DDRAM_address(byte address);
		void display_on_off(boolean on_off);
		void cursor_on_off(boolean on_off);
		void blink_on_off(boolean on_off);
		void cursor_shift_right();
		void cursor_shift_left();
		void display_shift_right();
		void display_shift_left();
		void increment_address();
		void decrement_address();
		void s_bit(boolean on_off);
	private:
		void set_datadirection();
    void toggle_enable();
		void send_nibble(byte lcd_data, int mode);
		void send_byte(int lcd_data, int mode);	
		
		byte _entryMode;
		byte _displayOnOff;
		byte _cursorDisplay_shift;
	
};

#endif