/*  User can input a text string up to 100 characters.  The text is
parsed into segments such that only complete words are displayed on
the 16 character LCD display.  

Inputting words of 16 characters long will make a mess,
thepiandi.blogspot.com  MJL 072014
*/
#include "LCD_2X16.h"

LCD_2X16 lcd;

char text[100];
int no_of_chars;

void parse_and_display_message(char text[], int no_of_chars){
  char display_text[16];
  int start;
  int line_end;
  int count;
  int line_begin;
  int i, j;
  int line_length;
  int line_number;
  
  start = 0;
  count = 0;
  line_number = 1;
  do {
    while (text[count] != 32){
      count++;
    }
    if ((count - start) < 18){
      line_end = count;
    }
    else{
      line_begin = start + (text[start] == 32);
      line_length = line_end - line_begin;
      
      for (i = line_begin, j = 0; i < line_end; i++, j++){
        display_text[j] = text[i];
      } 
      
      if (line_length < 16){
        display_text[line_length] = 10;
      }
      line_number = (line_number + 1) % 2;

      if (!line_number){
        lcd.clear_display();
      }
      lcd.lcd_message(display_text, line_number + 1);
      delay(1000);
      
      start = line_end;
      
    }
    count++;
  }while (count < no_of_chars + 1);
  
  
  line_begin = start + (text[start] == 32);
  line_length = count - line_begin -1;
  
  for (i = line_begin, j = 0; i < (count -1); i++, j++){
    display_text[j] = text[i];
  } 
  
  if (line_length < 16){
    display_text[line_length] = 10;
  }
  
  line_number = (line_number + 1) %2;

  if (!line_number){
    lcd.clear_display();
  }
  lcd.lcd_message(display_text, line_number + 1);
  delay(1000);
}
void setup() {  
  Serial.begin(9600);
  
  lcd.lcd_init();
  
  Serial.println("Please enter your text (100 chars max)");
  while (!Serial.available() > 0);
  no_of_chars = Serial.readBytesUntil('\n', text, 100); 
  text[no_of_chars] = 32;  //tack a space on to the end
  
  Serial.print("\nText: ");
  for (int i = 0; i < no_of_chars; i++){
    Serial.print(text[i]);
  }
  Serial.println("");
}

void loop() {
  parse_and_display_message(text, no_of_chars);
}
