/*  User can input a text string up to 100 characters.  
Text is displayed starting on the top line at the far right of the screen.
Text proceeds right to left as new text is added to the right of the screen.
Once the full text has been displayed, it starts over at the same position
on the screen but on the other liue.  This goes on indefinately with the
texte alternating from line to line.

thepiandi.blogspot.com  MJL 072014
*/
#include "LCD_2X16.h"

LCD_2X16 lcd;

char text[100];
int no_of_chars;

byte scroll(char text[], int no_of_chars, byte address_counter){
  char character[2];
  
  for (int i = 0; (i < no_of_chars + 1) && (text[i] != 10) ; i++){
    
    lcd.DDRAM_address(address_counter ^ 0x40);  //switch to other line
    lcd.lcd_message(" \n", 0);  //print space to erase character
    
    lcd.DDRAM_address(address_counter);  //back to line we are writing to
    
    character[0] = text[i];            //character to write to LCD
    character[1] = 10;                 //apopend new line
    lcd.lcd_message(character, 0);     //character goes to address 1 past right end of display.
    lcd.display_shift_left();          //shift left to bring character into view
    address_counter++;                 //for next character
    if (((address_counter & 0x3F) % 0x28) == 0){   //se if we have reached end of line space
      address_counter -= 0x28;                     //if so get to beginning of line
    }
    delay(200);
  }
  return address_counter;
}

void times_square_scroll(char text[], int no_of_chars){
  byte address_counter;

  address_counter = 0x10; //right end of display + 1
  
  while (1){
    address_counter = scroll(text, no_of_chars, address_counter);  //print text once
    address_counter ^= 0x40;  //shift to other line
  }
}
void setup() {  
  Serial.begin(9600);
  
  lcd.lcd_init();
  
  Serial.println("Please enter your text (100 chars max)");
  while (!Serial.available() > 0);
  no_of_chars = Serial.readBytesUntil('\n', text, 100); 
  text[no_of_chars] = 10;  //tack a line feed on to the end
  
  Serial.print("\nText: ");
  for (int i = 0; i < no_of_chars; i++){
    Serial.print(text[i]);
  }
  Serial.println("");
   
  times_square_scroll(text, no_of_chars);
}


void loop() {
}
