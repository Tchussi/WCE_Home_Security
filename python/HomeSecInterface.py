#-----------------------------------------------------------------------------
# Filename: HomeSecInterface.py
# 
# Description: Python code to interface with Arduino HTTP Server
#
# Notes: 
#-----------------------------------------------------------------------------

from tkinter import *
from tkinter import ttk
from PIL import Image, ImageTk
import json
import re

try:
    import urllib.request as urllib2
except ImportError:
    import urllib2


#------------------------------------------------------------------------------
# Build GUI
root = Tk()
root.title("Home Security")
root.option_add('*tearOff', FALSE)

#------------------------------------------------------------------------------
# Create the main frame (Home)
mainframe = Frame(root)
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
mainframe.columnconfigure(0, weight=1)
mainframe.rowconfigure(0, weight=1)
mainframe["background"] = "black"

homeSecLabel = Label(mainframe, text="Home Security")
homeSecLabel.grid(column=0, row=0, sticky=(N, W), padx=5)
homeSecLabel.configure(font=("times", 38))
homeSecLabel.configure(foreground="red")
homeSecLabel["background"] = "black"

# Home Security Image
homeSecLogo = ImageTk.PhotoImage(Image.open("logo.jpg"))
homeSecImage = Label(mainframe, image=homeSecLogo)
homeSecImage.grid(column=1, row=0, sticky=E)
homeSecImage["background"] = "black"

#------------------------------------------------------------------------------
# Create the top frame (Home)
topframe = Frame(root)
topframe.grid(column=0, row=1, sticky=(N, W, E, S))
topframe.columnconfigure(0, weight=1)
topframe.rowconfigure(0, weight=1)
topframe["background"] = "black"

# Whether the Home frame is active
homeAlive = 1

doorSecLogo = ImageTk.PhotoImage(Image.open("door.jpg"))

#------------------------------------------------------------------------------
# Create the top frame (Home)
topframeOptions = Frame(root)
topframeOptions.columnconfigure(0, weight=1)
topframeOptions.rowconfigure(0, weight=1)
topframeOptions["background"] = "black"

def buildRefresh():

    global topframe
    global topframeOptions
    global homeAlive
    global doorSecLogo

    #Request the status
    response = urllib2.urlopen("http://192.168.1.144")
    json_input = response.readall().decode('utf-8')

    print(json_input)

    parsed_json = json.loads(json_input)

    email = StringVar()
    email = parsed_json['email']
    phone = StringVar()
    phone = parsed_json['phone']
    temperature = StringVar()
    temperature = parsed_json['temperature']
    humidity = StringVar()
    humidity = parsed_json['humidity']


    #Rebuild options frame
    if homeAlive == 0:

        topframeOptions = Frame(root)
        topframeOptions.columnconfigure(0, weight=1)
        topframeOptions.rowconfigure(0, weight=1)
        topframeOptions["background"] = "black"

        #------------------------------------------------------------------------------
        # Option Page

        #------------------------------------------------------------------------------
        # Email Frame
        emailframe = Frame(topframeOptions)
        emailframe.grid(column=0, row=0, sticky=(N, W, E, S), columnspan=2)
        emailframe.columnconfigure(0, weight=1)
        emailframe.rowconfigure(0, weight=1)
        emailframe["background"] = "black"
        emailframe["relief"] = "groove"
        emailframe["bd"] = 5

        emaillabel = Label(emailframe, text="Email:")
        emaillabel.grid(column=0, row=0)
        emaillabel.configure(font=("times", 20))
        emaillabel.configure(foreground="red")
        emaillabel["background"] = "black"

        emailentry = Entry(emailframe, textvariable=email)
        emailentry.grid(column=1, row=0)

        emailbutton = Button(emailframe, text="Edit Email")
        emailbutton.grid(column=2, row=0, padx=5)
        emailbutton.configure(font=("times", 20))
        emailbutton.configure(foreground="white")
        emailbutton["background"] = "grey"

        #------------------------------------------------------------------------------
        # Phone Frame
        phoneframe = Frame(topframeOptions)
        phoneframe.grid(column=0, row=1, sticky=(N, W, E, S), columnspan=2)
        phoneframe.columnconfigure(0, weight=1)
        phoneframe.rowconfigure(0, weight=1)
        phoneframe["background"] = "black"
        phoneframe["relief"] = "groove"
        phoneframe["bd"] = 5

        phonelabel = Label(phoneframe, text="Phone:")
        phonelabel.grid(column=0, row=0)
        phonelabel.configure(font=("times", 20))
        phonelabel.configure(foreground="red")
        phonelabel["background"] = "black"

        phoneentry = Entry(phoneframe, textvariable=phone)
        phoneentry.grid(column=1, row=0)

        phonebutton = Button(phoneframe, text="Edit Phone:")
        phonebutton.grid(column=2, row=0, padx=5)
        phonebutton.configure(font=("times", 20))
        phonebutton.configure(foreground="white")
        phonebutton["background"] = "grey"

        #------------------------------------------------------------------------------
        # Reset Frame
        resetframe = Frame(topframeOptions)
        resetframe.grid(column=0, row=2, sticky=(N, W, E, S), columnspan=2)
        resetframe.columnconfigure(0, weight=1)
        resetframe.rowconfigure(0, weight=1)
        resetframe["background"] = "black"
        resetframe["relief"] = "groove"
        resetframe["bd"] = 5

        resetbutton = Button(resetframe, text="Reset System")
        resetbutton.grid(column=0, row=0, padx=5)
        resetbutton.configure(font=("times", 20))
        resetbutton.configure(foreground="white")
        resetbutton["background"] = "grey"

    #Rebuild home frame
    else:

        topframe.destroy()
        topframe = Frame(root)
        topframe.grid(column=0, row=1, sticky=(N, W, E, S))
        topframe.columnconfigure(0, weight=1)
        topframe.rowconfigure(0, weight=1)
        topframe["background"] = "black"

        #------------------------------------------------------------------------------
        # Node Frame
        nodeframe = Frame(topframe)
        nodeframe.grid(column=0, row=0, sticky=(N, W, E, S), columnspan=2)
        nodeframe.columnconfigure(0, weight=1)
        nodeframe.rowconfigure(0, weight=1)
        nodeframe["background"] = "black"

        #------------------------------------------------------------------------------
        # Temp & Humidity Frame
        tempframe = Frame(nodeframe)
        tempframe.grid(column=0, row=1, stick=(N, W, E, S), columnspan=2)
        tempframe.columnconfigure(0, weight=1)
        tempframe.rowconfigure(0, weight=1)
        tempframe["background"] = "black"
        tempframe["relief"] = "groove"
        tempframe["bd"] = 5

        # Temperature Label (Name)
        tempLabel = Label(tempframe, text="Temperature: ")
        tempLabel.grid(column=0, row=0, sticky=W, padx=(0, 48))
        tempLabel.configure(font=("times", 20))
        tempLabel.configure(foreground="white")
        tempLabel["background"] = "black"

        # Temperature Value (Name)
        tempValue = Label(tempframe, text=temperature)
        tempValue.grid(column=1, row=0, sticky=W, padx=(0, 48))
        tempValue.configure(font=("times", 20))
        tempValue.configure(foreground="white")
        tempValue["background"] = "black"

        # Humidity Label (Name)
        humidLabel = Label(tempframe, text="Humidity: ")
        humidLabel.grid(column=2, row=0, sticky=W, padx=(0, 48))
        humidLabel.configure(font=("times", 20))
        humidLabel.configure(foreground="white")
        humidLabel["background"] = "black"

        # Humidity Value (Name)
        humidValue = Label(tempframe, text=humidity)
        humidValue.grid(column=3, row=0, sticky=W, padx=(0, 48))
        humidValue.configure(font=("times", 20))
        humidValue.configure(foreground="white")
        humidValue["background"] = "black"

        currRow = 2

        for node in parsed_json['door']:

            name = "Door"
            status = node

            #------------------------------------------------------------------------------
            # Door Frame Secure
            doorframe = Frame(nodeframe)
            doorframe.grid(column=0, row=currRow, sticky=(N, W, E, S), columnspan=2)
            doorframe.columnconfigure(0, weight=1)
            doorframe.rowconfigure(0, weight=1)
            doorframe["background"] = "black"
            doorframe["relief"] = "groove"
            doorframe["bd"] = 5

            # Door Logo (find a different one)
            doorSecImage = Label(doorframe, image=doorSecLogo)
            doorSecImage.grid(column=0, row=0, sticky=W)
            doorSecImage["background"] = "black"

            # Door Label (Name)
            doorLabel = Label(doorframe, text=name)
            doorLabel.grid(column=1, row=0, sticky=W, padx=(0, 48))
            doorLabel.configure(font=("times", 20))
            doorLabel.configure(foreground="white")
            doorLabel["background"] = "black"

            if status == '1':
                # Door Label (Secure)
                doorSecLabel = Label(doorframe, text="Secure")
                doorSecLabel.grid(column=2, row=0, padx=5)
                doorSecLabel.configure(font=("times", 20))
                doorSecLabel.configure(foreground="green")
                doorSecLabel["background"] = "black"

            else:
                # Door Label (Not-Secure)
                doorSecLabel = Label(doorframe, text="Not-Secure")
                doorSecLabel.grid(column=2, row=0, padx=5)
                doorSecLabel.configure(font=("times", 20))
                doorSecLabel.configure(foreground="red")
                doorSecLabel["background"] = "black"

            # Door Button (Test)
            doorButton = Button(doorframe, text="Test")
            doorButton.grid(column=3, row=0, padx=5)
            doorButton.configure(font=("times", 20))
            doorButton.configure(foreground="white")
            doorButton["background"] = "grey"

            currRow = currRow + 1

        for node in parsed_json['camera']:

            name = Camera
            image = node

            #------------------------------------------------------------------------------
            # Camera Frame
            cameraframe = Frame(nodeframe)
            cameraframe.grid(column=0, row=currRow, sticky=(N, W, E, S))
            cameraframe.columnconfigure(0, weight=1)
            cameraframe.rowconfigure(0, weight=1)
            cameraframe["background"] = "black"
            cameraframe["relief"] = "groove"
            cameraframe["bd"] = 5

            # Camera Logo (find a different one)
            cameraSecLogo = ImageTk.PhotoImage(Image.open("camera.jpg"))
            cameraSecImage = Label(cameraframe, image=cameraSecLogo)
            cameraSecImage.grid(column=0, row=0, sticky=W)
            cameraSecImage["background"] = "black"

            # Door Label (Name)
            cameraLabel = Label(cameraframe, text=name)
            cameraLabel.grid(column=1, row=0, sticky=W)
            cameraLabel.configure(font=("times", 20))
            cameraLabel.configure(foreground="white")
            cameraLabel["background"] = "black"

            # Door Button (Show Image)
            doorButton = Button(cameraframe, text="Show Image")
            doorButton.grid(column=2, row=0, padx=5)
            doorButton.configure(font=("times", 20))
            doorButton.configure(foreground="white")
            doorButton["background"] = "grey"

            # Camera Button (Test)
            cameraButton = Button(cameraframe, text="Test")
            cameraButton.grid(column=3, row=0, padx=5)
            cameraButton.configure(font=("times", 20))
            cameraButton.configure(foreground="white")
            cameraButton["background"] = "grey"

            currRow = currRow + 1

#------------------------------------------------------------------------------
# Builds the home options
def buildHome():
    try:
        global topframe
        global homeAlive
        topframeOptions.grid_forget()
        topframe.grid(column=0, row=1, sticky=(N, W, E, S))
        homeAlive = 1
        buildRefresh()
    except ValueError:
        pass

#------------------------------------------------------------------------------
# Builds the home options
def buildOption():
    try:
        global topframe
        global homeAlive
        topframe.grid_forget()
        topframeOptions.grid(column=0, row=1, sticky=(N, W, E, S))
        homeAlive = 0
        buildRefresh()
    except ValueError:
        pass

# create a toplevel menu
menubar = Menu(root)
menubar.add_command(label="Home", command=buildHome)  # Generate Home
menubar.add_command(label="Options", command=buildOption) # Generate Options
menubar.add_command(label="Refresh", command=buildRefresh) # Generate Options

# display the menu
root.config(menu=menubar)
root["background"] = "black"
root.minsize(594, 200)
root.maxsize(594, 800)

buildRefresh()


root.mainloop()