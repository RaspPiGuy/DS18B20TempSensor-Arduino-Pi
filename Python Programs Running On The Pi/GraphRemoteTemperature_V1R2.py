#!/usr/bin/python

"""
Graphs temperature results from up to 12 sensors. Sensors are connected to Arduino

Description of this program is on my blog.

MJL - thepiandi.blogspot.com - 02/21/2015
"""

# MJL -- www.thepiandi.blogspot.com --  10/31/2014

import os
from datetime import datetime
import time
import subprocess
import sys
from GUI4GraphTemperature_V2R2 import *
from pyrrd.rrd import DataSource, RRA, RRD
from pyrrd.graph import DEF, CDEF, VDEF, LINE, AREA, GPRINT, COMMENT
from pyrrd.graph import ColorAttributes
from pyrrd.graph import Graph
import serial

    
ser = serial.Serial('/dev/ttyAMA0', 9600, timeout = 5)
frame_length = 20  # One frame is all the data for one device.
global start_time

def retrieve_serial():
    """
    Called by number_of_sensors()
    First we flush anything that might be on the serial port
    We write the character "s" to the serial port.
    The Gertboard checks to see if this character is present
    If so it will write the last series of measurements to the serial port
    This function reads the data on the serial port every second
      until it finds the number of bytes to match a multiple of the
      number of bytes for each device.
    If the serial data is not found in 10 seconds, the function exits
    Returns the length of the message it has received.
    """
    ser.flushInput() # Make sure no lingering bytes on serial port
    ser.write("s")  # send "s" to trigger Gertboard to transmit data
    start_time = time.time()
    
    while time.time() - start_time < 10:
        time.sleep(1)
        recv_length = ser.inWaiting()  # finds how many bytes on serial port
        if recv_length > 0 and recv_length % frame_length == 0:
           return recv_length   # good result

    return 0  # failure
        
        
def get_order(num_sensors):
    """
    Called by get_stored_data() and get_measurements()
    Function finds the device number for each device and puts
      these numbers in orted order.  
    """
    orig_order = []  # Device numbers as received
    sorted_order = [] # Device numbers put in numerical order
    for i in range(num_sensors):
        orig_order.append(recv_data[20 * i + 18])

    sorted_order = sorted(orig_order)
    return sorted_order

def number_of_sensors():
    """
    Called from get_stored_data() and get_measurements()
    This calls retrieve_serial()
    retrieve_serial() returns the length of the receive data
    This function calculates the number of sensors from the
      received data and populates recv_data[] with all the
      data retrieved from the Gertboard.
    Returns the number of sensors
    """
    global recv_data
    recv_data = [] # Where Gertboard data will go
    sensors = 0
    recv_length = retrieve_serial()  # will be  0 if failed

    if recv_length:
        sensors = recv_length / frame_length        
        in_coming = ser.read(size = recv_length)  # get the serial data here

        for i in range(recv_length):
            recv_data.append(ord(in_coming[i]))        

    return sensors

def get_stored_data():    
    """
     Called to retrieve number of sensors, device numbers, and resolution
     Reads Gertboard data twice.  Number of sensors and sorted device
        numbers must match to pass
     Makes three tries with 1 munute between tries
     If we pass, returns nuber of sensors and sorted device numbers.  This
        data becomes constant.  Subsequent reads are compared to these values
     If we fail, returns 0 for number of sensors and empty string for sorted
        device numbers.  Program will subsequently halt.
    """
    trials = 0
    global missed_attempts
    while trials < 3:
        first_sensors = number_of_sensors()
        if first_sensors > 0:
            first_stored_order = get_order(first_sensors)

            second_sensors = number_of_sensors()
            if second_sensors == first_sensors:
                second_stored_order = get_order(second_sensors)

                if first_stored_order == second_stored_order:
                    return first_sensors, first_stored_order # good result

        trials += 1
        time.sleep(10)
        print "Glitch retrieving stored data, trying again"
        
    print "\nThree missed attempts. No more tries!"
    return 0, []  # if failed
        
def get_measurement(orig_sorted_order):
    """
    Called to obtain the data from Gertboard for the measuremsnts.
    Calls number_of_sensors() and get_order()
    Checks that the number of sensors and the sorted order of device numbers
      match what was obtained at the start of the program.
    If a match does not occur, we try twice more.  If a match fails, we
      return False.
    In the process, recv_data[] is updated with current data
    """

    trials = 0
    global error_sensor_number
    global error_sensor_order
    
    while trials < 3:
        if number_of_sensors() != no_sensors:
            error_sensor_number += 1
        else:
            if  get_order(no_sensors) != original_sorted_order:
                error_sensor_order += 1
            else:
                return True    
    
        trials += 1
        time.sleep(1.2 * no_sensors)
        
    return False
            

# ----------------------------------------------------------------------------

# Main Program

variable_list = ["", "", "", 0, 0, 0, "", True]
error_no_data = 0
error_sensor_number = 0
error_sensor_order = 0

print
print "     Retrieving Transmitted Data - This will take a few seconds"
print

try:

    # Retrieve number of sensors, descriptions, and device number    
    original_sorted_order = []
    no_sensors, original_sorted_order = get_stored_data()

    if not no_sensors:  # Failed to get stored data after 3 tries
        print "\nData failure. Exiting program"
        raise(KeyboardInterrupt)

    #Check that there are not more than 12 sensors
    if no_sensors > 12:
        print "     You have selected %d sensors.  The maximum is 12." % no_sensors
        raise(KeyboardInterrupt)

    sensor_name = []   # description entered by user at sending unit
    device_number = [] # device number given by sending unit
    resolution = []    # resolutuon (9 - 12) entered by user at sending unit
    
    # search by sensor number get sensor descriptions, resolutions, and device
    #    numbers
    for i in range(no_sensors):
        for j in range(no_sensors):
            if recv_data[20 * j + 18] == original_sorted_order[i]:
        
                # get description
                found_end = False

                if recv_data[20 * j + 6] == 10: # found end of line
                     description = "No Descrip."
                else:
                    description = chr(recv_data[20 * j + 6])
                for k in range(1, 12):
                    if recv_data[20 * j + 6 + k] == 10: #found end of line
                        break
                    else:
                        description = (description +
                            chr(recv_data[20 * j + 6 + k]))
                        
                # escape colons in the description
                description = description.replace(":", "\:")

                # get resolution
                resolution.append((recv_data[20 * j + 5] >> 5) + 9)

                # get device number
                device_number.append(recv_data[20 * j + 18])

        sensor_name.append(description)

    #Retrieve data from GUI    
    variable_list = guiwindow()
    
    if variable_list[7]:
        raise(KeyboardInterrupt)

    title_it = variable_list[0]
    comment = variable_list[1]
    background = str(variable_list[2])
    how_high = variable_list[3]
    max_measurements = int(variable_list[4])
    measurement_interval = variable_list[5]
    filename = variable_list[6]
    
    print
    for i in range(no_sensors):
        print "name for sensor %d: %s" % (device_number[i], sensor_name[i])
    print "graph title: ", title_it
    print "comment: ", comment
    print "background color: ", background
    print "graph height: ", how_high
    print "maximum number of measurements: ", max_measurements
    print "measurement interval: ", measurement_interval
    print "base file name: ", filename
    print

    start_time = int(time.time() / measurement_interval) * measurement_interval
    next_meas_time= start_time + measurement_interval


    # File names
    
    graphfile_blk = filename + '_black.png'
    graphfile_wht = filename + '_white.png'
    resultsfile = filename + '.txt'

    rrdfile = []
    for i in range(no_sensors):
       rrdfile.append(filename + '_sensor' +  str(i + 1) + '.rrd')
                      
        

    # Setup results file
    try:
        f = open(resultsfile, 'w')
        try:
            f.write("Graph Title: %s \n\n" %title_it)
            for i in range(no_sensors):
                f.write("Measuring: %s, Sensor number: %d, Resolution: %d bits\n"
                        %(sensor_name[i], device_number[i], resolution[i]))            
            if comment != "":
                f.write("\nGraph Comments: %s \n" %comment)
            f.write("\n\n")
        finally:
            f.close()
    except:
        print '\nCould not write to the file\n'                  


    # Graph colors:
    colors = []
    colors.append('#FF0000') # Red
    colors.append('#0000FF') # Blue
    colors.append('#00FF00') # Green
    colors.append('#00FFFF') # Cyan
    colors.append('#FFFF00') # Yellow
    colors.append('#FF00FF') # Magenta
    colors.append('#FF8000') # Orange
    colors.append('#804000') # Brown
    colors.append('#408080') # Gray Green
    colors.append('#FF80FF') # Pink
    colors.append('#408040') # Dark Green
    colors.append('#0040C0') # Dark Blue

    # Setup RRD Files and Graph
    dsName = []
    device = []
    device_def = []
    device_line = []
    device_aver = []
    device_val = []
    vname = []
    
    
    for i in range(no_sensors):        
        #  RRD Setup
        dsName.append("Sensor" + str(i))

        dataSources = []
        roundRobinArchives = []
        dataSource = (DataSource(dsName=dsName[i], dsType='GAUGE',
                heartbeat=int(1.5 * measurement_interval)))
        dataSources.append(dataSource)

        roundRobinArchives.append(RRA(cf='LAST', xff=0.5, steps=1, \
                rows=max_measurements))
      
        device.append(RRD(rrdfile[i], step=measurement_interval,
                ds=dataSources, rra=roundRobinArchives, start=start_time))
        device[i].create(debug=False)

        #  Graph Setup
        device_def.append(DEF(rrdfile=rrdfile[i], vname= dsName[i] + '_data',
                dsName=dsName[i], cdef='LAST'))
        device_line.append(LINE(defObj=device_def[i], color=colors[i],
                legend=sensor_name[i] + ' Temperature'))
        device_aver.append(VDEF(vname= dsName[i] + '_aver',
                rpn='%s,AVERAGE' % device_def[i].vname))
        device_val.append(GPRINT(device_aver[i], 'Average ' +
                sensor_name[i] + ' Temperature: %6.2lf Degrees F'))
                        

    # Graph Comment    

    cmt = COMMENT(comment)

    # Define Graph Colors
    #    black background:
    black_bkgnd = ColorAttributes()
    black_bkgnd.back = '#000000'
    black_bkgnd.canvas = '#333333'
    black_bkgnd.shadea = '#000000'
    black_bkgnd.shadeb = '#111111'
    black_bkgnd.mgrid = '#CCCCCC'
    black_bkgnd.axis = '#FFFFFF'
    black_bkgnd.frame = '#0000AA'
    black_bkgnd.font = '#FFFFFF'
    black_bkgnd.arrow = '#FFFFFF'

    #    white background:
    white_bkgnd = ColorAttributes()
    white_bkgnd.back = '#FFFFFF'
    white_bkgnd.canvas = '#EEEEEE'
    white_bkgnd.shadea = '#000000'
    white_bkgnd.shadeb = '#111111'
    white_bkgnd.mgrid = '#444444'
    white_bkgnd.axis = '#000000'
    white_bkgnd.frame = '#0000AA'
    white_bkgnd.font = '#000000'   
    white_bkgnd.arrow = '#000000'
    

    
    # Let's make some measurements and graph them
    print ('First Measurement Will Be Made At: ' +
        time.asctime(time.localtime(next_meas_time)))
    print
    
    measurement = 1
    while max_measurements:
        # waiting for next measurement
        time_now = time.time()
        while time_now < next_meas_time:
            time.sleep(0.5)
            time_now = time.time()
            
        timenow = datetime.now()
    
        # Putting the measurment time into the results text file
        try:
            f=open(resultsfile, 'a')
            try:
                f.write("Measurement: %d. %d to go\n" % (measurement, (max_measurements -1)))
                f.write(timenow.strftime("%A, %B %d, %I:%M:%S %p:\n"))
            finally:
                f.close()
        except(IOError):
            print '\nCould not write to the file\n'
                                
        # Now we get the data and retrieve the temperature
        temp_f = []
        
        print "Measurement: %d. %d to go" % (measurement, (max_measurements -1))
        print timenow.strftime("%A, %B %d, %I:%M:%S %p:")

        if get_measurement(original_sorted_order):  # retrieve the data
            for i in range(no_sensors):
                for j in range(no_sensors):
                    if recv_data[20 * j + 18] == original_sorted_order[i]:
                        temp_c = (256 * recv_data[20 * j + 2] +
                            recv_data[20 * j + 1])
                        if recv_data[20 * j + 2] > 127:
                            temp_c -= 65536

                        temp_c /= 16.0
                        temp_f.append(round(1.8 * temp_c + 32.0, 1))

                        # print result to the terminal
                        print (" The %s sensor temperature is %3.1f" %
                            (sensor_name[i], temp_f[i]) + u"\xB0" +"F")

                        # temperature result to the graph
                        device[i].bufferValue(next_meas_time, str(temp_f[i]))
                        device[i].update(debug = False)

            #append temperature results to the results text file
            try:
                f = open(resultsfile, 'a')                
                for i in range(no_sensors):                    
                    try:
                        f.write((" The %s sensor temperature is %3.1f")
                            %(sensor_name[i], temp_f[i]) + " degF\n")                        
                    except(IOError):
                        print '\nCould not write to the file\n'
            except(IOError):
                print '\nCould not open the file\n'
            f.close()

            #skip a line in the file after last sensor
            try:
                f = open(resultsfile, 'a')
                try:
                    f.write("\n")
                finally:
                    f.close()
            except(IOError):
                print '\nCould not write to the file\n'

            # write to the graph files
            gb = Graph(graphfile_blk, start = start_time,
                end = next_meas_time - measurement_interval, color = black_bkgnd,
                vertical_label='Degrees\ F', width=600, height=how_high,
                title=title_it)
            gw = Graph(graphfile_wht, start = start_time,
                end = next_meas_time - measurement_interval, color = white_bkgnd,
                vertical_label='Degrees\ F', width=600, height=how_high,
                title=title_it)

            for i in range(no_sensors):
                gb.data.extend([device_def[i], device_line[i],
                    device_aver[i], device_val[i]])
                gw.data.extend([device_def[i], device_line[i],
                    device_aver[i], device_val[i]])
            
            if comment:
                gb.data.extend([cmt])
                gw.data.extend([cmt])
            
            if background == '0' or background == '2':          
                gb.write()
            if background == '1' or background == '2':          
                gw.write()

        # what to do if a measurement fails
        else:
            error_no_data += 1
            print " Failed to retrieve data"
            
            # print failure message to results text file
            try:
                f = open(resultsfile, 'a')                
                try:
                    f.write(" Failed to retrieve data\n\n")
                except(IOError):
                    print '\nCould not write to the file\n'
            except(IOError):
                print '\nCould not open the file\n'
            f.close()
                            
        print # skip a line after the last sensor                                              

        # setup for next measurement
        next_meas_time += measurement_interval
        measurement += 1
        max_measurements -= 1
    
except(KeyboardInterrupt):
    print

if not variable_list[7]:

    print
    print "See you later"
    print
    print "Instances of No data Received: %d" % error_no_data
    print "Instances of Wrong Number of Sensors: %d" % error_sensor_number
    print "Instances of Wrong Sensor Order: %d" % error_sensor_order
    
    print           
    print "To look at the .rrd flies you need:"
    print "  start time: ", start_time
    print "  last measurement: ", next_meas_time


    run_time = next_meas_time - start_time - measurement_interval
    run_days = run_time / 86400
    run_hours = run_time % 86400 / 3600
    run_minutes = run_time % 86400 % 3600 / 60
    total_measurements = run_time / measurement_interval

    print
    print ('Total Run Time: %2d days, %2d hours, %2d minutes'
        %(run_days, run_hours, run_minutes))
    print 'Total Number of Measurements Per Device: ' + str(total_measurements)
    print

    try:
        f = open(resultsfile, 'a')                
        try:
            f.write("Instances of No Data Received: %d\n" % error_no_data)
            f.write("Instances of Wrong Number of Sensors: %d\n" % error_sensor_number)
            f.write("Instances of Wrong Sensor Order: %d\n\n" % error_sensor_order)
            f.write("To look at the .rrd flies you need:\n")
            f.write("  Start time: %d\n" % start_time)
            f.write("  Last measurement: %d\n\n" % next_meas_time)
            f.write("Total Run Time: %2d days, %2d hours, %2d minutes\n"
                % (run_days, run_hours, run_minutes))
            f.write("Total Number of Measurements Per Device: %d\n" %
                total_measurements)
        except(IOError):
            print '\nCould not write to the file\n'
    except(IOError):
        print '\nCould not open the file\n'
    f.close()
                            
 









