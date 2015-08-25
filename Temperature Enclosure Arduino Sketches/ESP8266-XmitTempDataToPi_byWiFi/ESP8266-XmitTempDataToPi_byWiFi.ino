/*------------------------------------------------------------------------------------
Collects Manchester Encoded data from Temperature Sensor board within Temperature
Sensor Box, decodes the data and tranmits the data to the Raspberry Pi

We synchronize with the data from the Sensor board and decode the data from one
sensor to find out how many sensors are connected to the temperature box.  If there
is more than one sensor, the data from the other sensors are then collected.  

All the data is put into one string.

We check to see if the Pi wants data by listening to see if the Pi, which acts as a 
server is on-line.  If not, the recently acquired data is simply lost.

MJL - thepiandi.blogspot.com

August 2, 2015

-------------------------------------------------------------------------------------*/

int Pin = 13;

#include <ESP8266WiFi.h>

// Use WiFiClient class to create TCP connections
WiFiClient client;

/*-------------------------------Global Variables-------------------------------*/
const char* ssid     = "squark741";
const char* password = "=5Qu@29xwa<EKP2%";
const char* host = "192.168.1.12";  //url of Raspberry Pi server
const int httpPort = 50007;  //Raspberry Pi port

volatile  boolean found_transistion;
const int bit_time = 833;  //833 usec. = 1200 baud (600 for data)
const int frame_len = 20;
byte frame[frame_len];  // Data for one sensor from transmitting device

/*-----------------------------------interrupt service routine-------------------------------*/
void findTransistion(){
  found_transistion = true;
}


/*-----------------------------------synchronize()-------------------------------*/
void synchronize(){
  // Routine to find synchronization
	
  unsigned long time, duration;
  boolean val;    // logic level of ESP pin 13
  boolean sync;   // will be true if synchronization has been accomplished
  int upper_threshhold = 4 * bit_time + 4 * bit_time / 10;  // time of 4 bits + 10%
  int lower_threshhold = 4 * bit_time - 4 * bit_time / 10;  // time of 4 bits - 10%

  sync = false;
  do{
    delay(0);
    attachInterrupt(Pin, findTransistion, RISING);  
    found_transistion = false;
    while (!found_transistion);   // We stay here until interrupt occurs
    time = micros();  
    attachInterrupt(Pin, findTransistion, FALLING);  
    found_transistion = false;
    while (!found_transistion);  
    duration = micros() - time;
    
    if ((duration > lower_threshhold) & (duration < upper_threshhold)){  //time of 4 bits +/= 10%
      attachInterrupt(Pin, findTransistion, RISING);  
      found_transistion = false;
      while (!found_transistion);
      duration = micros() - time - duration;
      
      if ((duration > lower_threshhold) & (duration < upper_threshhold)){  //time of 4 bits +/= 10%     
        sync = true;
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
  boolean looking4Rising = true;
  delay(0);
  	
  // Finding the single positive pulse after synchronization
  
  attachInterrupt(Pin, findTransistion, FALLING);  
  found_transistion = false;
  while (!found_transistion); 


  // Now we are into the Manchester encoded data  
  no_bytes = 0;
  present_bit = false; // equals 0
  do{   
    no_clocks = 0;
    do{
      if (looking4Rising){
        attachInterrupt(Pin, findTransistion, RISING);  
        looking4Rising = false;
      }
      else{
        attachInterrupt(Pin, findTransistion, FALLING);  
        looking4Rising = true;
      }
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
boolean transmit_data(String dataToPi){

  String dataFromPi;
  boolean success = false;
      
  //Connect to server on Pi and send all data
  if(client.connect(host, httpPort)){
      client.print(dataToPi); 
      success = true;
  }
        
  return success;
}

/*_______________________________Main Program____________________________________*/
/*-----------------------------------setup()-------------------------------*/

void setup() {
  pinMode(Pin, INPUT);
  Serial.begin(115200);  
  Serial.println("\n");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected\n\n");  
  
}

/*------------------------------------loop()------------------------------------*/
void loop() {
  boolean CRCs_good, successful = false;
  int i, j;
  int number_of_sensors;
  int data_len;      //DEBUG
  CRCs_good = true;
  String dataToXmit;

  //Get one frame and find out how many sensors and put the first sensor in xmit_data
  synchronize();  //find synchronization
  manchester_data();  //find one frame worth of data, get frame[]

  if (!calculateCRC_byte(frame, frame_len)){
    number_of_sensors = frame[0];

    //char xmit_data[number_of_sensors * frame_len];  //define xmit_data here
    for (j = 0; j < frame_len; j++){
      //xmit_data[j] = frame[j];
      dataToXmit.concat(frame[j]);
      dataToXmit.concat(',');
    }
    
    //this will add the rest of the sensors    
    for (i = 1; i < number_of_sensors; i++){      
      synchronize();  //find synchronization
      manchester_data();  //find one frame worth of data, get frame[]
      if (calculateCRC_byte(frame, frame_len)){
        CRCs_good = false;
      }
      else{
        for (j = 0; j < frame_len; j++){
          //xmit_data[frame_len * i + j] = frame[j];
          dataToXmit.concat(frame[j]);          
          dataToXmit.concat(',');
        }
      }	
    }
  }
    
  //Serial.println(dataToXmit);

  if (CRCs_good){
    successful = transmit_data(dataToXmit);
  }
  
  if (successful){
    Serial.println("Transfered Data To Pi");
  }
  else{
    Serial.println("Pi Not Requesting Data");
  }
    
}
