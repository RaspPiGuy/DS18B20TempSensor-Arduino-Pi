
/*  Receives Manchester Encoded Data From Transmitting Arduino.

This is a script mainly to troubleshoot RF connection between transmitter and 
receiver. It will not transmit data to the RaspberryPi.  If it see the 
synhchronization pulses from the transmitter it will receive and display
the data mfollowing the synchronization pulses.  The data is displayed in
hex format.  Finally, the CRC is calculated and either "CRC OK" or "CRC Bad"
will be displayed at the end of the line.

Data starts low then a 101010 pattern, then two synchronization pulses 11110000
followed by a 10. Raw data is transmitted at 600 bits/sec.  Encoded data is 
twice that rate of 1200 baud.  Manchester encoding ensures there will never be 
more than two 1's or two 0's in a row.  Hence, the synchronization pattern is four 
1's followed by four 0's, a pattern that can not exist within the 
Manchester encoded data.  Within the Manchester encoded data, there will only 
be two 1's or two 0's, in a row, if there is a bit change in the raw data.  A 1 
is represented as a transition of a 0 to a 1 while a 0 is a transition of a 1 to a 0.

Data received from Arduino is 9 bytes of DS18B20 scratchpad data.  
Sketch performs manchester decoding, checks CRC, and upon request, sends
scratchpad data to python program on the Raspberry Pi.


MJL - thepiandi.blogspot.com    2/21/2015
*/

#include <avr/io.h>

/*-------------------------------Global Variables-------------------------------*/
volatile  boolean found_transistion;
const int frame_len = 20;
byte frame[frame_len];  // Data for one sensor from transmitting device
const int bit_time = 833;  //833 usec. = 1200 baud (600 for data)
#define PIN PIND //Arduino Input is on port D
#define DATAMASK 0x04  //Arduino pin 2 or PD2

/*--------------------------Interrupt Service Routine----------------------------*/
ISR(INT0_vect){
  found_transistion = true;
}

/*-----------------------------------synchronize()-------------------------------*/
void synchronize(){
  // Routine to find synchronization
	
  unsigned long time, duration;
  boolean val;    // logic level of PD2 pin - Arduino pin 2
  boolean sync;   // will be true if synchronization has been accomplished
  int upper_threshhold = 4 * bit_time + 4 * bit_time / 10;  // time of 4 bits + 10%
  int lower_threshhold = 4 * bit_time - 4 * bit_time / 10;  // time of 4 bits - 10%

  sync = false;
  do{
    found_transistion = false;
    time = micros();  
    while (!found_transistion);   // We stay here until interrupt occurs
    duration = micros() - time;
    val = PIN & DATAMASK;
    if (!val){
      if ((duration > lower_threshhold) & (duration < upper_threshhold)){  //time of 4 bits +/= 10%
        found_transistion = false;
        time = micros();  
        while (!found_transistion);
        duration = micros() - time;
        val = PIN & DATAMASK;
        if (val){
          if ((duration > lower_threshhold) & (duration < upper_threshhold)){
            sync = true;
          }
        }
      }    
    }    
  } while(!sync);
}

/*-------------------------------manchester_data()-------------------------------*/
//after synchrozination, receives one frame from transmitting device
void manchester_data(){
  unsigned long time, duration;
  int no_bytes, no_clocks;
  boolean present_bit;  // keeps track of last bit of Manchester encoded data
  byte hold_byte = 0;   // recovered data flows into this variable
  int threshhold = bit_time + bit_time / 2;  // bit_time + 50%
  	
  // Finding the single positive pulse after synchronization
  found_transistion = false;
  while (!found_transistion);

  // Now we are into the Manchester encoded data  
  no_bytes = 0;
  present_bit = false; // equals 0
  do{
    no_clocks = 0;
    do{
      found_transistion = false;
      time = micros();  
      while (!found_transistion);  // waiting for interrupt
      duration = micros() - time;

      if (duration > threshhold){ //found bit change if true
        present_bit = !present_bit;
        no_clocks += 2;  // a bit change will take up two clock pulses
      }
      else{
        no_clocks += 1;   // if there was no bit change
      }
      if (!(no_clocks % 2)){  // we look at every other clock time
        hold_byte <<= 1;  // last bit is shifted left one position
        hold_byte += present_bit;  // new bit goes into the LSB position
      }
    }while (no_clocks < 16);  // we have received one byte with 16 clocks
    frame[no_bytes] = hold_byte;  // transfer that byte to array
    no_bytes++;    
  } while(no_bytes < frame_len);  // raw data stream is mess_len bytes long
}

/*-----------------------------calculateCRC_byte()-----------------------------*/
boolean calculateCRC_byte(byte inByte[], int num_bytes){
  byte workingByte[num_bytes];
  boolean failure = false;
  int crc = 0;
  for (int i = 0; i < num_bytes; i++){
    workingByte[i] = inByte[i];
    for (int j = 0; j < 8; j++){
      if ((crc % 2) ^ (workingByte[i] %2)){
        crc ^= 0b100011000;  // corresponds to polynomial X8 + X5 + X4 +1
      }
      crc >>= 1;
      workingByte[i] >>= 1;
    }   
  }
  if (crc) failure = true;  //will be 0 if CRC passes
  return failure;
}

/*-------------------------------transmit_data()-------------------------------*/
void transmit_data(char xmit_data[], int xmit_length){
  char buff[2];  
 
  if (Serial.available()){  //check to see if python put start character on serial port
    Serial.readBytes(buff, 1);  //get character from serial port.  
    if (buff[0] == 's'){  // s for start
      for (int i = 0; i < xmit_length; i++){
        Serial.print(xmit_data[i]);
      }
    } //if not "s" go back to top of loop
  }  //if no serial data available, go back to top of loop
/*
  for (int i = 0; i < xmit_length; i++){
    Serial.print(xmit_data[i]);
    Serial.print(" ");
  }
  Serial.println("");
*/  
}

/*_______________________________Main Program____________________________________*/
/*-----------------------------------setup()-------------------------------------*/
void setup() {
  
  DDRD &= ~(1 << 2 );   // PD2 is input  - Arduino pin 2
  
  EICRA = B00000001;   // Any change will trigger interrupt
  EIMSK = B00000001;   // Enable INT0 interrupt
  
  Serial.begin(9600);

}
/*------------------------------------loop()------------------------------------*/
void loop() {
  boolean CRCs_good;
  int i, j;
  int number_of_sensors;
  int data_len;      //DEBUG
  CRCs_good = true;

  //Get one frame and find out how many sensors and put the first sensor in xmit_data
  synchronize();  //find synchronization
  manchester_data();  //find one frame worth of data, get frame[]
  
  for (i = 0; i < frame_len; i++){
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
    
  if (!calculateCRC_byte(frame, frame_len)){
    Serial.println("CRC OK");
  }
  else{
    Serial.println("CRC Bad");
  }
}


