
#include "Arduino.h"
#include "LCD_2X16.h"

LCD_2X16::LCD_2X16(){
}

void LCD_2X16::set_datadirection(){
  RS_DDR |= (1 << RS_PIN);
  E_DDR |= (1 << E_PIN);
  D4_DDR |= (1 << D4_PIN);
  D5_DDR |= (1 << D5_PIN);
  D6_DDR |= (1 << D6_PIN);
  D7_DDR |= (1 << D7_PIN);  
}

void LCD_2X16::toggle_enable(){
  delayMicroseconds(5 / CLOCK_FACTOR);
  E_PORT |= (1 << E_PIN);
  delayMicroseconds(5 / CLOCK_FACTOR);
  E_PORT &= ~(1 << E_PIN);
  delay(2);
}

void LCD_2X16::send_nibble(byte lcd_data, int mode){
  if (mode){
    RS_PORT |= (1 << RS_PIN);
  }
  else{
    RS_PORT &= ~(1 << RS_PIN);
  }
  
  if (lcd_data > 127){
    D7_PORT |= (1 << D7_PIN);
  }
  else{
    D7_PORT &= ~(1 << D7_PIN);
  }
  lcd_data <<= 1;
  
  if (lcd_data > 127){
    D6_PORT |= (1 << D6_PIN);
  }
  else{
    D6_PORT &= ~(1 << D6_PIN);
  }
  lcd_data <<= 1;
  
  if (lcd_data > 127){
    D5_PORT |= (1 << D5_PIN);
  }
  else{
    D5_PORT &= ~(1 << D5_PIN);
  }
  lcd_data <<= 1;

  if (lcd_data > 127){
    D4_PORT |= (1 << D4_PIN);
  }
  else{
    D4_PORT &= ~(1 << D4_PIN);
  }

  toggle_enable(); 
}

void LCD_2X16::send_byte(int lcd_data, int mode){
  send_nibble(lcd_data, mode);
  lcd_data *= 0x10;
  send_nibble(lcd_data, mode); 
}

void LCD_2X16::lcd_message(char message[], int line_no){
  if (line_no == 1){
    send_byte(LCD_LINE_1, LCD_CMD);
  }
  else if (line_no == 2){
    send_byte(LCD_LINE_2, LCD_CMD);
  }
  for (int i = 0; i < 16; i++){

    if (int(message[i]) == 10){
      break;
    }
    send_byte(message[i], LCD_CHR);
  }
}

void LCD_2X16::lcd_init(){
	set_datadirection();
	delay(15);
	
  E_PORT &= (255 - (1 << E_PIN)); //Make sure E starts out as 0
  delay(2);
  
  send_nibble(0x30, LCD_CMD); //See page 46 of HD 44780U document
  delay(5);
  send_nibble(0x30, LCD_CMD); //See page 46 of HD 44780U document
  delayMicroseconds(100 / CLOCK_FACTOR);
  send_byte(0x32, LCD_CMD); //  for these first three commands
  send_byte(0x28, LCD_CMD); //4 bit commands, 2 line display, 5x8 character format
  send_byte(0x0C, LCD_CMD); //Display ON, curser OFF, blink OFF
  send_byte(0x06, LCD_CMD); //Increment Address, no shift
  send_byte(0x01, LCD_CMD); //Clear Display  
	
	_entryMode = 0x06;  //I/D = 1, S = 0; Increment Address, no shift
	_displayOnOff = 0x0C;  //D = 1, C = 0, B = 0; Display ON, Cursor OFF, Blink OFF
	_cursorDisplay_shift = 0x10;  //S/C = 0, R/L = 0; Cursor Move, Shift Left 

}

void LCD_2X16::clear_display(){
  send_byte(0x01, LCD_CMD);
}

void LCD_2X16::DDRAM_address(byte address){
	address |= (0x80);
	send_byte(address, LCD_CMD);
}

void LCD_2X16::display_on_off(boolean on_off){
	if (on_off){
		_displayOnOff |= 0x04;
	}
	else{
		_displayOnOff &= ~(0x04);
	}
  send_byte(_displayOnOff, LCD_CMD);	
}

void LCD_2X16::cursor_on_off(boolean on_off){
	if (on_off){
		_displayOnOff |= 0x02;
	}
	else{
		_displayOnOff &= ~(0x02);
	}
  send_byte(_displayOnOff, LCD_CMD);	
}

void LCD_2X16::blink_on_off(boolean on_off){
	if (on_off){
		_displayOnOff |= 0x01;
	}
	else{
		_displayOnOff &= ~(0x01);
	}
  send_byte(_displayOnOff, LCD_CMD);		
}

void LCD_2X16::cursor_shift_right(){
	_cursorDisplay_shift |= (0x04);
	send_byte(_cursorDisplay_shift, LCD_CMD);
}

void LCD_2X16::cursor_shift_left(){
	_cursorDisplay_shift &= ~(0x04);
	send_byte(_cursorDisplay_shift, LCD_CMD);
}

void LCD_2X16::display_shift_right(){
	_cursorDisplay_shift |= (0x0C);
	send_byte(_cursorDisplay_shift, LCD_CMD);
}

void LCD_2X16::display_shift_left(){
	_cursorDisplay_shift |= (0x08);
	_cursorDisplay_shift &= ~(0x04);
	send_byte(_cursorDisplay_shift, LCD_CMD);
}

void LCD_2X16::increment_address(){
	_entryMode |= (0x02);
	send_byte(_entryMode, LCD_CMD);
}

void LCD_2X16::decrement_address(){
	_entryMode &= ~(0x02);
	send_byte(_entryMode, LCD_CMD);
}

void LCD_2X16::s_bit(boolean on_off){
	if (on_off){
		_entryMode |= (0x01);
	}
	else{
		_entryMode &= ~(0x01);
	}
}
