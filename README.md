DS18B20-Pi-Arduino
==================

Industrial Strength Temperature Measurements With Arduino and Raspberry Pi

Incremental Project to use Maxim/Dallas DS18B20 Temperature Sensors

Goals:
*  Write my own 1-wire library including all 1-wire functions

*  Use ATmega328P to interface to as many as 50 devices

*  Store ROM code and up to 12 characters in ATmega's EEPROM (reason for limit of 50 devices).

Use Raspbery Pi to control functionality of ATmega.

ATmega device to be pulled from Arduino and installed on PCB and in an enclosure

Enclosure will also contain batteries, couple of switches, and 2 line by 16 character LCD display

ATmega and Raspberry Pi to communicate with each other by undetermined means - probably RF

Status:
June 12, 2014:
*  Arduino sketches written to interface with multiple devices, make temperature readings, scan for new devices,

*  Store ROM code and descriptions in EEPROM, remove a device from EEPROM, and edit descriptions.

*  Auxiliary sketches for writing to and reading from EEPROM

June 19, 2014:
*  Parasitic operation added

June 21, 2014
*  Changed Find_New_Device to ask for upper and lower alarm temperature and resolution.  Stores these values  in the devices EEPROM.

*  Rewrote Edit_Description now as Edit_Stored_Information to not only allow user to change a device's  description, but also to change the upper and lower alarm temperature and resolution.

July 2, 2014
*  In the library .h and ,cpp files, all ROM Commands as well as all DS18B20 Functions Commands are implemented per the Maxim/Dallas data sheet.

*  Library files to handle storing and retrieving device ROM codes and descriptions in the ATmega EEPROM are implemented.

*  Library functions to handle imputing numbers in floating point and integer formats are implemented.

*  All Arduino application sketches for the DS18B20 are complete.  These include the following sketches:

**  Find_New_Device - Scans the 1-wire bus to see if any device has been added.  If so, asks use for a description which is stored in ATmega EEPROM along with the ROM code.  Asks user to input upper and lower temperature limits and resolution.  These are stored within the device's scratchpad and the device's EEPROM.

**  Read_Temperature - Asks user what devices to run, number of measurements, and time between measurements.  Function determines if devices are connected using parasitic power.  Measurements are made accordingly. User has to option to print out the temperature limits as well as the resolution along with the temperature.  Once the run is made, displays the average of all measurements for each device.  If there were any CRC errors or initialization errors, the number of these are reported.  If a device is connected using parasitic power, when the device is undergoing the convert temperature process, a transistor pulls the 1-wire data line up to Vcc.  The time for this "hard pullup: is dictated by data sheet for the resolution programmed into the device's scratchpad. For devices not connected by parasitic power, when undergoing temperature conversion, the device, itself, informs the bus when the conversion is done.

**  Scan_for_Alarms - Scans the 1-wire bus to see if any device's temperature is out of limits.  If so, displays that information to the user.  This function must determine if the device is connected using parasitic power.

**  Devices_in_EEPROM - Displays the ROM code and description for each device as stored in the ATmega EEPROM.

**  Devices_On_The_Bus - Scans the 1-wire bus and reports ROM code and CRC results.

**  Edit_Stored_Information - Reports to user ROM code, description, upper and lower temperature limits and resolution.  Asks user which device to change and what to change.  Changes are made accordingly and stored.

**  Remove_Device_From_EEPROM - Allows user to remove ROM code and description for any device.  This does not remove information from the device's scratchpad or the device's EEPROM.

July 20, 2014:
*   Starting with python sketches, written for the Raspberry Pi, wrote a library for the 2 line by 16 character LCD display.  

*   Using the LCD display library, wrote sketch "Input_to_TwoLineDisplay". User can input a text string up to 100 characters.  The text is parsed into segments such that only complete words are displayed on the 16 character LCD display.

*   Using the LCD display library, wrote sketch called "Times_Square_Scroll".  User can input a text string up to 100 characters.  Text is displayed starting on the top line at the far right of the screen. Text proceeds right to left as new text is added to the right of the screen. Once the full text has been displayed, it starts over at the same position on the screen but on the other line.  This goes on indefinitely with the text alternating from line to line.

*   At this time, LCD has not been incorporated into DS18B20 temperature measurements.

August 19, 2014
*   Added sketch,: "DS18B20_Suite".  This combines the sketches:
* Read_Temperature
* Scan_for_Alarms
* Find_New_Device
* Devices_On_The_Bus
* Devices_in_EEPROM
* Edit_Stored_Information
* Remove_Device_From_EEPROM
into one sketch with a menu for the user to select the desired operation.

August 10, 2015:
	In the year since this has been updated, much has been added:

*	Constructed an enclosure that contains:
* Protoboard with an ATmega328P to interface the temperature sensors with devices to send data to the Raspberry Pi.  
* The enclosure circuitry is powered by a 3.7V 1200maH Lithium Ion Polymer battery 
* The LiPo battery feeds an Adafruit 500mA. 5V power boost. 
* A SparkFun basic LiPo battery charger with mini-USB.
* 2 line by 16 character LCD display that displays one of the sensor's description and current temperature measurement.
* A momentary switch that allows the operator to cycle the display between the sensors.
* A permanently mounted DS18B20 temperature sensor
* Three connectors for additional DS18B20 temperature sensors installed in cables.  A large number of the sensors can be daisy-chained to one connector.
* The data from the ATmega328P is manchester encoded and is connected (via a DPDT switch) to either a 434MHz RF transmitter or to a ESP8266 WiFi board.
* The data consists of 20 bytes for each sensor connected to the enclosure.  This data consists of:
o Byte 0:  The number of sensors connected to the enclosure
o Bytes 1, and 2:  DS18B20 scratchpad data - the measured temperature
o Byte 3: DS18B20 scratchpad data - upper temperature alarm
o Byte 4: DS18B20 scratchpad data - lower temperature alarm
o Byte 5: DS18B20 scratchpad data - resolution
o Bytes 6 through 17: from the ATmega EEPROM - device description
o Byte 18: from the ATmega EEPROM - the sensor number
o Byte 19: CRC (cyclic redundancy check)
* The WiFi ESP8266 board contains an ESP-12 module, a 3.3V regu7lator, and 5V to 3.3V conversion circuitry for the incoming data from the ATmega module and for the FTDI received data.  
* The WiFi ESP8266 board receives the manchester encoded data from the ATmega board, decodes it and checks the CRC.  This board connects to a WiFi network and acts as a client.  When requested by the Raspberry Pi (the server) the board will send the last collected data from all of the attached sensors to the Raspberry Pi.
* Both the ATmega board and the WiFi board have connectors assessable through an opening in the side of the enclosure for an FTDI basic.  This allows the firmware to be changed at any time.  For the ATmega board, if a new sensor were to be added, or if description, resolution, or upper or lower temperature alarm limits needed to be changed, it would be necessary to load the DS18B20 suite (described above).  If a different WiFi network was used, the firmware for the WiFi board would need to be altered and reloaded for a new SSID and password.
*	As an alternative to the WiFi connection between the enclosure and the Pi, a board was constructed with a 434MHz receiver.  The receiver demodulates the 434MHz signal from the enclosure.  This demodulated signal is the manchester encoded data containing the sensor data.  The data output from the receiver connects to a Gertboard.  The ATmega328P on the Gertboard decodes the manchester data and checks the CRC.  The ATmnega328P on the Gertboard connects to the Raspberry Pi's UART port.  When the Pi requests data, the serial data is sent via the UART port.
*	
