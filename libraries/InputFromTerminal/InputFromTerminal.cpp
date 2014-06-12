#include "Arduino.h"
#include "InputFromTerminal.h"

TERM_INPUT::TERM_INPUT(){
}

float TERM_INPUT::termFloat(){
  
  float returningFloat;
  boolean decimal_found;
  boolean negative;
  float leftOfDecimal;
  float rightOfDecimal;
  int digits2right;
  boolean firstchar;
  boolean good_number;
  
  int inChar;

  do { 
    good_number = true;
    
    decimal_found = false;
    negative = false;
    leftOfDecimal = 0;
    rightOfDecimal = 0;
    digits2right = 0;
    returningFloat = 0.0;
    firstchar = false;
    
    do {
      while (!Serial.available() > 0);  //wait for monitor
      inChar = Serial.read();
        
      if (inChar == 10);           //do nothing
  
      else if (inChar == 46){ 	        //dedimal point
        if (!decimal_found){
          decimal_found = true;
          firstchar = true;
        }
        else{
          good_number = false;
        }        
      }
      else if (inChar == 45){ 	//minus sign
        if (!firstchar){
          negative = true;
          firstchar = true;
        }
        else{
          good_number = false;
        }
      }
      else if (inChar > 47 && inChar < 58){  //digits 0 - 9
         if (!decimal_found){
           leftOfDecimal *= 10;
           leftOfDecimal += (inChar - 48);
         }
         else{
           rightOfDecimal *= 10;
           rightOfDecimal += (inChar - 48);
           digits2right++;
         }
         firstchar = true;       
      }
      else{
        good_number = false;
      }
 
    }while(inChar != 10); 
    
    if (!good_number){
      Serial.println("Try again");
    }
    
  }while (!good_number);

  while(digits2right > 0){
    rightOfDecimal /= 10.0;
    digits2right--;   
  }
  
  returningFloat = leftOfDecimal + rightOfDecimal;
    
  if (negative){
    returningFloat = -returningFloat;
  }
      
  return returningFloat;
}



long TERM_INPUT::termInt(){
  
  long returningInt; 
  boolean good_number;
  int inChar;

  do { 
    good_number = true;
    returningInt = 0;
    
    do {
      while (!Serial.available() > 0);  //wait for monitor
      inChar = Serial.read();
        
      if (inChar == 10);           //do nothing
      else if (inChar > 47 && inChar < 58){  //digits 0 - 9
        returningInt *= 10;
        returningInt += (inChar - 48);    
      }
      else{
        good_number = false;
      }
 
    }while(inChar != 10); 
    
    if (!good_number){
      Serial.println("Try again");
    }
    
  }while (!good_number);

      
  return returningInt;
}


