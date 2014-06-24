DS18B20-Pi-Arduino
==================

Industrial Strength Temperature Measurements With Arduino and Raspberry Pi

Incremental Project to use Maxim/Dallas DS18B20 Temperature Sensors

Goals:
* Write my own 1-wire library including all 1-wire functions
* Use ATmega328P to interface to as many as 50 devices
* Store ROM code and up to 12 chgaracters in ATmega's EEPROM (reason for limit of 50 devices)
* Use Raspbery Pi to control functionality of ATmega.
* ATmega device to be pullled from Arduino and installed on PCB and in an enclosure
* Enclosure will also contail batteries, couple of switches, and 2 line by 16 character LCD display
* ATmega and Raspberry Pi to communicate with each other by undetermined means - probably RF

Status:
  June 12, 2014:  
  *  One-wire library written for Arduino with all functions except, parasitic operation and alarms.
  *  Arduino sketches written to interface with multiple devices, make temperature readings, scan for new devices,
  Store ROM code and descriptions in EEPROM, remove a device from EEPROM, and edit descriptions.
  *  Auxiliary sketches for writing to and reading from EEPROM

  June 19, 2014:
  *  Parasitic operation added

  June 21, 2014
  *  Changed Find_New_Device to ask for upper and lower alarm temperature and resolution.  Stores these values
  in the devices EEPROM.
  *  Rewrote Edit_Description now as Edit_Stored_Information to not only allow user to change a devices's
  description, but also to change the upper and lower alarm temperature and resolution.
