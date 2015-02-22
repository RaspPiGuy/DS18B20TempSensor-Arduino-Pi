#!/usr/bin/python

""" This module supports the GraphTemperatureWithGUI.py with a GUI support

Martin Lyon  9/20/2014"""

from Tkinter import *
import os
import subprocess
from datetime import datetime
import cPickle as pickle


def guiwindow():

    def getdirectory():   
        """ Prepares new directory name based upon year, month,and day like: 2013 07 27.  This is added
        to the base directory /home/pi/Documents/PythonProjects/TempProbe.  We check to see if it exists
        and creates it if not. """
        
        basedirectory = '/home/pi/Documents/PythonProjects/TempProbe/TempResults/'  
        newdirectory = datetime.now().strftime('%Y_%m_%d')
        os.chdir(basedirectory)
        err = 'error'
        while err != '':
            directorylist = subprocess.Popen('ls', stdout = subprocess.PIPE, stderr = subprocess.PIPE)
            out, err = directorylist.communicate()
        filedirectory = basedirectory + newdirectory + '/'
        if newdirectory not in out:   
            os.system('mkdir ' + filedirectory)
        return filedirectory        
    
    def check_escaped(intext, char):
        """Makes sure that all of the space or colon characters are escaped with the backspace character"""
        cnt_chars = intext.count(char)
        start_pos = 0
        while cnt_chars:
            index_char = intext.find(char, start_pos)
            if intext[index_char -1] != '\\':
                intext = intext[:index_char] + '\\' + intext[index_char:]
                index_char +=1
            start_pos = index_char + 1
            cnt_chars -= 1
        return intext
        
    def removemessage():
        
        badEntry0.grid_remove()
        badEntry1.grid_remove()
        badEntry2.grid_remove()
        badEntry3.grid_remove()
        badEntry4.grid_remove()

    def loaddefaults():

        #removemessage()
        
        graphtitle.set("Temperature\ Profile")
        graphcomments.set("")
        whichcolor.set(0)
        graphheight.set(400)
        maxmeasurements.set("1")
        interval_days.set("0")
        interval_hours.set("0")
        interval_minutes.set("1")
        fileoverwrite.set(0)
        filenamebase.set("")
        
    def proceed():

        removemessage()        
        
        ok = True


        # Check Title for nonescaped spaces
        graphtitle.set(check_escaped(graphtitle.get(), ' '))
                       
        # Check Comments for nonescaped colons
        graphcomments.set(check_escaped(graphcomments.get(), ':'))

        # Check Max Measurements.  Must be digits and not 0
        if not maxmeasurements.get().isdigit():
            badEntry0.grid()
            ok = False
        else:
            if int(maxmeasurements.get()) == 0:
                badEntry0.grid()
                ok = False
                
        # Check Measurement Interval.  Must be digits and not 0
        if not (interval_days.get().isdigit() and interval_hours.get().isdigit() and interval_minutes.get().isdigit()):
            badEntry1.grid()
            ok = False
        else:
            if int(interval_days.get()) + int(interval_hours.get()) + int(interval_minutes.get()) == 0:
                badEntry1.grid()
                ok = False

        # Check that File Name is Alpha Numeric, not "", and Overwrite Status

        if filenamebase.get() == '':
            badEntry4.grid()
            ok = False
            return

        if not '_' in filenamebase.get():           
            if not filenamebase.get().isalnum():
                badEntry2.grid()
                ok = False
                return
            
        else:
            for i in range(len(filenamebase.get())):
                if not filenamebase.get()[i].isalnum() and not filenamebase.get()[i] == '_':
                    badEntry2.grid()
                    ok = False
                    return
                
        if fileoverwrite.get() == 0:
            file_name = directoryname.get() + filenamebase.get() + ".txt"
            try:
                f=open(file_name)
                f.close
                badEntry3.grid()
                ok = False
            except:
                pass

        # If everything is OK we close the window            
        if ok:
            root.destroy()
        return

    def quitprogram():
        quitall.set(True)
        root.destroy()
        return
    
    
#---------------------------------------------------------------------

# Main Script

# Initialize Variables

    root = Tk()
    root.title('PI Temperature Measurements')
    root.geometry('700x330')

    picklefile = "/home/pi/Documents/PythonProjects/TempProbe/configNEW.pickle"

    graphtitle=StringVar()
    graphcomments=StringVar()
    whichcolor=IntVar()
    graphheight=IntVar()
    maxmeasurements=StringVar()
    interval_days=StringVar()
    interval_hours=StringVar()
    interval_minutes=StringVar()
    fileoverwrite=IntVar()
    directoryname=StringVar()
    directoryname.set(getdirectory())
    filenamebase=StringVar()
    quitall=BooleanVar()
    quitall.set(False)


    try:
        f = open(picklefile, "r")
        configvars = pickle.load(f)
        f.close()
            
        graphtitle.set(configvars[0])
        graphcomments.set(configvars[1])
        whichcolor.set(configvars[2])
        graphheight.set(configvars[3])
        maxmeasurements.set(configvars[4])
        interval_days.set(configvars[5])
        interval_hours.set(configvars[6])
        interval_minutes.set(configvars[7])
        fileoverwrite.set(configvars[8])
        filenamebase.set(configvars[9])
        
    except:
        loaddefaults()
            
# Setup All the frames

    frame1 = Frame(root)
    frame1.grid(row = 0, column = 0, sticky = W)
    frame2 = Frame(root)
    frame2.grid(row = 1, column = 0, sticky = W)
    frame3 = Frame(root)
    frame3.grid(row = 2, column = 0, sticky = W)
    frame4 = Frame(root)
    frame4.grid(row = 3, column = 0, sticky = W)
    frame5 = Frame(root)
    frame5.grid(row = 4, column = 0, sticky = W)
    frame6 = Frame(root)
    frame6.grid(row = 5, column = 0, sticky = W)
    frame7 = Frame(root)
    frame7.grid(row = 6, column = 0, sticky = W)

# Set up the font for labels

    labelfont = ('times', 12, 'bold')

# Set up the widgets

    # Graph Title entry widget:
    graphtitleLabel = Label(frame1, text = ' Graph Title: ', font = labelfont)
    graphtitleLabel.grid(row = 1, column = 0, columnspan = 2, pady = 5, sticky = W)
    graphtitleText = Entry(frame1, textvariable = graphtitle, width = 40)
    graphtitleText.grid(row = 1, column = 2, padx = 10, sticky = W)

    # Graph comment entry widget:
    graphcommentLabel = Label(frame1, text = ' Graph Comment: ', font = labelfont)
    graphcommentLabel.grid(row = 2, column = 0, columnspan = 2, pady = 5, sticky = W)
    graphcommentText = Entry(frame1, textvariable = graphcomments, width = 40)
    graphcommentText.grid(row = 2, column = 2, padx = 10, sticky = W)

    # Background color radiobutton widget:
    whichcolorLabel = Label(frame2, text = ' Graph Background Color: ', font = labelfont)
    whichcolorLabel.grid(row = 0, column = 0, pady = 5, sticky = W)
    whichcolor0 = Radiobutton(frame2, text = 'Black', variable = whichcolor, value = 0)
    whichcolor0.grid(row = 0, column = 1)
    whichcolor1 = Radiobutton(frame2, text = 'White', variable = whichcolor, value = 1)
    whichcolor1.grid(row = 0, column = 2)
    whichcolor2 = Radiobutton(frame2, text = 'Both', variable = whichcolor, value = 2)
    whichcolor2.grid(row = 0, column = 3)

    # Graph height scale widget:
    graphHiteLable = Label(frame3, text = ' Height of Graph: ', font = labelfont)
    graphHiteLable.grid(row = 0, column = 0, sticky = W)
    graphHite = Scale(frame3, orient = HORIZONTAL, from_ = 100, to = 400, variable = graphheight)
    graphHite.configure(resolution = 100, length = 200, tickinterval = 100)
    graphHite.grid(row = 0, column = 1, pady = 5, sticky = W)

    # Maximum measurements entry widget:
    maxmeasLabel = Label(frame4, text =  ' Number of Measurements: ', font = labelfont)
    maxmeasLabel.grid(row = 0, column = 0, pady = 5, sticky = W)
    maxmeasText = Entry(frame4, textvariable = maxmeasurements, width = 4, justify = RIGHT)
    maxmeasText.grid(row = 0, column = 1, padx = 10)
    badEntry0 = Label(frame4, text = 'Non-nteger or less than 1 measurement', fg = 'red')
    badEntry0.grid(row = 0, column = 2, sticky = W)
    badEntry0.grid_remove()

    # Measurement interval entry widgets:
    intervalLabel0 = Label(frame5, text =  ' Measurement Interval: ', font = labelfont)
    intervalLabel0.grid(row = 0, column = 0, pady = 5, sticky = W)
    interval_daysText = Entry(frame5, textvariable = interval_days, width = 4, justify = RIGHT)
    interval_daysText.grid(row = 0, column = 1)
    intervalLabel1 = Label(frame5, text = 'day(s)  ', font = labelfont)
    intervalLabel1.grid(row = 0, column = 2)
    interval_hoursText = Entry(frame5, textvariable = interval_hours, width = 3, justify = RIGHT)
    interval_hoursText.grid(row = 0, column = 3)
    intervalLabel2 = Label(frame5, text = 'hour(s)  ', font = labelfont)
    intervalLabel2.grid(row = 0, column = 4)
    interval_minutesText = Entry(frame5, textvariable = interval_minutes, width = 3, justify = RIGHT)
    interval_minutesText.grid(row = 0, column = 5)
    intervalLabel3 = Label(frame5, text = 'minute(s)', font = labelfont)
    intervalLabel3.grid(row = 0, column = 6)
    badEntry1 = Label(frame5, text = '   A non-nteger or less than 1 minute', fg = 'red')
    badEntry1.grid(row = 0, column = 7)
    badEntry1.grid_remove()

    # Check for file overwrite check widget:
    filecheckLabel = Label(frame6, text = ' Allow File Overwrite: ', font = labelfont)
    filecheckLabel.grid(row = 0, column = 0, pady = 5, sticky = W)
    filecheckBox = Checkbutton(frame6, variable = fileoverwrite)
    filecheckBox.grid(row = 0, column = 1)

    # File name entry widget
    filenameLabel = Label(frame6, text = '     Base Filename: ', font = labelfont)
    filenameLabel.grid(row = 0, column = 2)
    filenameText = Entry(frame6, textvariable = filenamebase, width = 15)
    filenameText.grid(row = 0, column = 3)
    badEntry2 = Label(frame6, text = ' Only numbers, letters, and underscore', fg = 'red')
    badEntry2.grid(row = 0, column = 4)
    badEntry2.grid_remove()
    badEntry3 = Label(frame6, text = ' Will overwrite existing file', fg = 'red')
    badEntry3.grid(row = 0, column = 4)
    badEntry3.grid_remove()
    badEntry4 = Label(frame6, text = ' Must enter a file name', fg = 'red')
    badEntry4.grid(row = 0, column = 4)
    badEntry4.grid_remove()

    # Continue button widget:
    proceedButton = Button(frame7, text = "Continue", command = proceed, font = labelfont, bg = 'green')
    proceedButton.configure(activebackground = 'green', width = 8, height = 2)
    proceedButton.grid(row = 0, column = 1, padx = 70, pady = 10)

    # Quit button widget:
    quitButton = Button(frame7, text = 'Quit', command = quitprogram, font = labelfont, bg = 'red')
    quitButton.configure(activebackground = 'red', width = 8)
    quitButton.grid(row = 0, column = 0, padx = 70)

    # Load defaults widget:
    defaultsButton = Button(frame7, text = 'Defaults', command = loaddefaults, font = labelfont, bg = 'yellow')
    defaultsButton.configure(activebackground = 'yellow', width = 8)
    defaultsButton.grid(row = 0, column = 2, padx = 70)


    root.mainloop()

    hours = int(interval_hours.get()) + 24 * int(interval_days.get())

    minutes = int(interval_minutes.get()) + 60 * hours
    total_interval = minutes * 60

    passout = []
    passout.append(graphtitle.get())
    passout.append(graphcomments.get())
    passout.append(whichcolor.get())
    passout.append(graphheight.get())
    passout.append(maxmeasurements.get())
    passout.append(total_interval)
    passout.append(directoryname.get() + filenamebase.get())
    passout.append(quitall.get())

    configout = []
    configout.append(graphtitle.get())
    configout.append(graphcomments.get())
    configout.append(whichcolor.get())
    configout.append(graphheight.get())
    configout.append(maxmeasurements.get())
    configout.append(interval_days.get())
    configout.append(interval_hours.get())
    configout.append(interval_minutes.get())
    configout.append(fileoverwrite.get())
    configout.append(filenamebase.get())
    
    try:
        f = open(picklefile, "w")
        pickle.dump(configout, f)
        f.close()
    except(IOError):
        pass
    
    return passout
#---------------------------------------------------------------------

# Test Code

if __name__ == '__main__':

    from time import sleep

    try:

        variable_list = guiwindow()

        
        if variable_list[7]:
            raise(KeyboardInterrupt)

        title_it = variable_list[0]
        comment = variable_list[1]
        background = variable_list[2]
        how_high = variable_list[3]
        max_measurements = variable_list[4]
        measurement_interval = variable_list[5]
        filename = variable_list[6]

        print "\ngraph title: ", title_it
        print "comment: ", comment
        print "background color: ", background
        print "graph height: ", how_high
        print "maximum number of measurements: ", max_measurements
        print "measurement interval: ", measurement_interval
        print "base file name: ", filename
        
# Create keyboard interrupt to test last line of code

        print "\nHit CTRL C"
        sleep(100)
        
    except(KeyboardInterrupt):
        if not variable_list[7]:
            print "\nStuff at end of program"
            


    
