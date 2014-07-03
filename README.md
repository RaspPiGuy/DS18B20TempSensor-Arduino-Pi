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

