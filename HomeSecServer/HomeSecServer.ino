//-----------------------------------------------------------------------------
// Filename: HomeSecServer.ino
// 
// Description: Arduino code for the WCE Home Security project
//
// Notes: Uses the Adafruit CC3000 HTTPServer example code as a base.
//        comments below.
//-----------------------------------------------------------------------------

/***************************************************
  Adafruit CC3000 Breakout/Shield Simple HTTP Server

  This is a simple implementation of a bare bones
  HTTP server that can respond to very simple requests.
  Note that this server is not meant to handle high
  load, concurrent connections, SSL, etc.  A 16mhz Arduino
  with 2K of memory can only handle so much complexity!
  This server example is best for very simple status messages
  or REST APIs.

  See the CC3000 tutorial on Adafruit's learning system
  for more information on setting up and using the
  CC3000:
    http://learn.adafruit.com/adafruit-cc3000-wifi

  Requirements:

  This sketch requires the Adafruit CC3000 library.  You can
  download the library from:
    https://github.com/adafruit/Adafruit_CC3000_Library

  For information on installing libraries in the Arduino IDE
  see this page:
    http://arduino.cc/en/Guide/Libraries

  Usage:

  Update the SSID and, if necessary, the CC3000 hardware pin
  information below, then run the sketch and check the
  output of the serial port.  After connecting to the
  wireless network successfully the sketch will output
  the IP address of the server and start listening for
  connections.  Once listening for connections, connect
  to the server IP from a web browser.  For example if your
  server is listening on IP 192.168.1.130 you would access
  http://192.168.1.130/ from your web browser.

  Created by Tony DiCola and adapted from HTTP server code created by Eric Friedrich.

  This code was adapted from Adafruit CC3000 library example
  code which has the following license:

  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "utility/debug.h"
#include "utility/socket.h"
#include "DHT.h"
#include <ArduinoJson.h>
#include <XBee.h>
#include <SD.h>
#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SoftwareSerial.h>  

// DHT11 sensor pins
#define DHTPIN 47
#define DHTTYPE DHT22

// Create DHT instances
DHT dht(DHTPIN, DHTTYPE);

// These are the interrupt and control pins
// MUST be an interrupt pin!
#define ADAFRUIT_CC3000_IRQ   20

// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  48
#define ADAFRUIT_CC3000_CS    53

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "WCE_wifi"   // cannot be longer than 32 characters!
#define WLAN_PASS       "password"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

// What TCP port to listen on for connections.
// The HTTP protocol uses port 80 by default.
#define LISTEN_PORT           80

// Maximum length of the HTTP action that can be parsed.
#define MAX_ACTION            10

// Maximum length of the HTTP request path that can be parsed.
// There isn't much memory available so keep this short!
#define MAX_PATH              64

// Size of buffer for incoming request data.
// Since only the first line is parsed this
// needs to be as large as the maximum action
// and path plus a little for whitespace and
// HTTP version.
#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20

// Amount of time in milliseconds to wait for
// an incoming request to finish.  Don't set this
// too high or your server could be slow to respond.
#define TIMEOUT_MS            500

// Max number of door and camera nodes.
#define MAX_DOORS 10
#define MAX_CAMERAS 1

// SD card chip select
#define chipSelect 45

// Using SoftwareSerial (Arduino 1.0+) or NewSoftSerial (Arduino 0023 & prior):
#if ARDUINO >= 100
// On Uno: camera TX connected to pin 2, camera RX to pin 3:
SoftwareSerial cameraconnection = SoftwareSerial(16, 17);
// On Mega: camera TX connected to pin 69 (A15), camera RX to pin 3:
//SoftwareSerial cameraconnection = SoftwareSerial(69, 3);
#else
NewSoftSerial cameraconnection = NewSoftSerial(16, 17);
#endif
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);

// HTTP Server
Adafruit_CC3000_Server httpServer(LISTEN_PORT);
uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char action[MAX_ACTION+1];
char path[MAX_PATH+1];

// Variables to be exposed to the API
int temperature;
int humidity;
String title = "Home";
String email = "email@email.com";
String phoneNumber = "111-111-1111";

int numDoorNodes;
int numCameraNodes = 0;
int doorNodes[MAX_DOORS];
String cameraNodes[MAX_CAMERAS];
uint64_t serial_nums [MAX_DOORS];

// JSON
DynamicJsonBuffer jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

// XBee Coms
XBee xbee = XBee();
ZBRxIoSampleResponse ioSample = ZBRxIoSampleResponse();
XBeeAddress64 test = XBeeAddress64();

//----------------------------------------------------------------------------------------
// setup
// Description: Setup the CC3000 and begin HTTP Server
// Parameters: none
// Return: none
//----------------------------------------------------------------------------------------

void setup(void)
{
  Serial.begin(9600);
  Serial1.begin(9600);
  xbee.setSerial(Serial1);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);

  // Initialize DHT sensor
  Serial.println(F("\nInitialize DHT Sensor"));
  dht.begin();
  
  // Initialise the module
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  // Display the IP address DNS, Gateway, etc.
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  numDoorNodes = 0;
  
  // When using hardware SPI, the SS pin MUST be set to an
  // output (even if not connected or used).  If left as a
  // floating input w/SPI on, this can cause lockuppage.
#if !defined(SOFTWARE_SPI)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if(chipSelect != 53) pinMode(53, OUTPUT); // SS on Mega
#else
  if(chipSelect != 10) pinMode(10, OUTPUT); // SS on Uno, etc.
#endif
#endif

  Serial.println("VC0706 Camera test");
  pinMode(chipSelect, OUTPUT);
  
  // Set SD card on WiFi off
  digitalWrite(ADAFRUIT_CC3000_CS, HIGH);
  digitalWrite(chipSelect, LOW);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }  
  
  // Set SD card off WiFi on
  digitalWrite(chipSelect, HIGH);
  digitalWrite(ADAFRUIT_CC3000_CS, LOW);
  
  
  // Try to locate the camera
  if (cam.begin()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }
  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }

  // Set the picture size - you can choose one of 640x480, 320x240 or 160x120 
  // Remember that bigger pictures take longer to transmit!
  
  //cam.setImageSize(VC0706_640x480);        // biggest
  cam.setImageSize(VC0706_320x240);        // medium
  //cam.setImageSize(VC0706_160x120);          // small

  // You can read the size back from the camera (optional, but maybe useful?)
  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_640x480) Serial.println("640x480");
  if (imgsize == VC0706_320x240) Serial.println("320x240");
  if (imgsize == VC0706_160x120) Serial.println("160x120");


  //  Motion detection system can alert you when the camera 'sees' motion!
  cam.setMotionDetect(true);           // turn it on
  //cam.setMotionDetect(false);        // turn it off   (default)

  // You can also verify whether motion detection is active!
  Serial.print("Motion detection is ");
  if (cam.getMotionDetect()) 
    Serial.println("ON");
  else 
    Serial.println("OFF");
  
  // Start listening for connections
  httpServer.begin();
  
  Serial.println(F("Listening for connections..."));
}

//----------------------------------------------------------------------------------------
// loop
// Description: Handles HTTP Requests and handles XBee communication
// Parameters: none
// Return: none
//----------------------------------------------------------------------------------------

void loop(void)
{

  // Update data
  XBeeComsLoop();

  // Measure from DHT
  temperature = (uint8_t)dht.readTemperature();
  humidity = (uint8_t)dht.readHumidity();

  // Update the Camera
  CameraComsLoop();
  
  // Try to get a client which is connected.
  Adafruit_CC3000_ClientRef client = httpServer.available();
  if (client) {
    Serial.println(F("Client connected."));
    // Process this request until it completes or times out.
    // Note that this is explicitly limited to handling one request at a time!

    // Clear the incoming data buffer and point to the beginning of it.
    bufindex = 0;
    memset(&buffer, 0, sizeof(buffer));
    
    // Clear action and path strings.
    memset(&action, 0, sizeof(action));
    memset(&path,   0, sizeof(path));

    // Set a timeout for reading all the incoming data.
    unsigned long endtime = millis() + TIMEOUT_MS;
    
    // Read all the incoming data until it can be parsed or the timeout expires.
    bool parsed = false;
    while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE)) {
      if (client.available()) {
        buffer[bufindex++] = client.read();
      }
      parsed = parseRequest(buffer, bufindex, action, path);
    }

    // Handle the request if it was parsed.
    if (parsed) {
      Serial.println(F("Processing request"));
      Serial.print(F("Action: ")); Serial.println(action);
      Serial.print(F("Path: ")); Serial.println(path);
      // Check the action to see if it was a GET request.
      if (strcmp(action, "GET") == 0) {
        // Respond with the path that was accessed.
        // First send the success response code.
        client.fastrprintln(F("HTTP/1.1 200 OK"));
        // Then send a few headers to identify the type of data returned and that
        // the connection will not be held open.
        client.fastrprintln(F("Content-Type: text/plain"));
        client.fastrprintln(F("Connection: close"));
        client.fastrprintln(F("Server: Adafruit CC3000"));
        // Send an empty line to signal start of body.
        client.fastrprintln(F(""));
        // Now send the response data.
        parsePath(path);
        buildStatus();
        root.printTo(client);
      }
      else {
        // Unsupported action, respond with an HTTP 405 method not allowed error.
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F(""));
      }
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(150);

    // Close the connection when done.
    Serial.println(F("Client disconnected"));
    client.close();
  }
}

//----------------------------------------------------------------------------------------
// CameraComsLoop
// Description: Parse the Camera responses
// Parameters: none
// Return: none
//----------------------------------------------------------------------------------------
void CameraComsLoop() {
    if (cam.motionDetected()) {
        Serial.println("Motion!");   
        cam.setMotionDetect(false);
    
        if (! cam.takePicture()) 
            Serial.println("Failed to snap!");
        else 
            Serial.println("Picture taken!");
    
        // Set SD card on WiFi off
        digitalWrite(ADAFRUIT_CC3000_CS, HIGH);
        digitalWrite(chipSelect, LOW);
        
        char filename[13];
        strcpy(filename, "IMAGE00.JPG");
        for (int i = 0; i < 100; i++) {
            filename[5] = '0' + i/10;
            filename[6] = '0' + i%10;
            // create if does not exist, do not open existing, write, sync after write
            if (! SD.exists(filename)) {
            break;
            }
        }
        
        File imgFile = SD.open(filename, FILE_WRITE);
        
        uint16_t jpglen = cam.frameLength();
        Serial.print(jpglen, DEC);
        Serial.println(" byte image");
        
        Serial.print("Writing image to "); Serial.print(filename);
        
        while (jpglen > 0) {
            // read 32 bytes at a time;
            uint8_t *buffer;
            uint8_t bytesToRead = min(32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
            buffer = cam.readPicture(bytesToRead);
            imgFile.write(buffer, bytesToRead);
        
            //Serial.print("Read ");  Serial.print(bytesToRead, DEC); Serial.println(" bytes");
        
            jpglen -= bytesToRead;
        }
        imgFile.close();
        
        // Set SD card off WiFi on
        digitalWrite(chipSelect, HIGH);
        digitalWrite(ADAFRUIT_CC3000_CS, LOW);
        
        Serial.println("...Done!");
        cam.resumeVideo();
        cam.setMotionDetect(true);
    }  
}

//----------------------------------------------------------------------------------------
// XBeeComsLoop
// Description: Parse the Xbee responses
// Parameters: none
// Return: none
//----------------------------------------------------------------------------------------
void XBeeComsLoop() {

  //attempt to read a packet    
  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    // got something

    if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) {
      xbee.getResponse().getZBRxIoSampleResponse(ioSample);

      uint64_t serialMSB = ioSample.getRemoteAddress64().getMsb();
      uint64_t serialLSB = ioSample.getRemoteAddress64().getLsb();
      uint64_t serial_num = (serialMSB << 32) | serialLSB; 

      // read analog inputs
      for (int i = 0; i <= 4; i++) {
        if (ioSample.isAnalogEnabled(i)) {

          int doorStatus = 0;
          if (ioSample.getAnalog(i) > 0) {
            doorStatus = 1;
          }

          // Search for serial_num or a location to use
          uint8_t empty = 0;
          bool filled = 0;
          for (int i = 0; i < MAX_DOORS; i++) {
            if (serial_nums[i] == serial_num) {
              doorNodes[i] = doorStatus;
              filled = 1;
            } else if (serial_nums[i] == 0) {
              empty = i;
            }
         }

          // Didn't find a slot
          if (!filled) {
            serial_nums[empty] = serial_num;
            doorNodes[empty] = doorStatus;
            numDoorNodes++;
            //Serial.print("Inserting New Door Node\n\n\n\n\n");
          }
          //Serial.print("Door Status\n");
          //Serial.println(doorStatus, BIN);
          //Serial.print("\n");
        }
      }
    } 
    else {
      Serial.print("Expected I/O Sample, but got ");
      Serial.print(xbee.getResponse().getApiId(), HEX);
    }    
  } else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());
  }
}

// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path) {
  // Check if the request ends with \r\n to signal end of first line.
  if (bufSize < 2)
    return false;
  if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    parseFirstLine((char*)buf, action, path);
    return true;
  }
  return false;
}

// Parse the action and path from the first line of an HTTP request.
void parseFirstLine(char* line, char* action, char* path) {
  // Parse first word up to whitespace as action.
  char* lineaction = strtok(line, " ");
  if (lineaction != NULL)
    strncpy(action, lineaction, MAX_ACTION);
  // Parse second word up to whitespace as path.
  char* linepath = strtok(NULL, " ");
  if (linepath != NULL)
    strncpy(path, linepath, MAX_PATH);
}

//----------------------------------------------------------------------------------------
// parsePath
// Description: Parse the HTTP request
// Parameters: path the full path
// Return: none
//----------------------------------------------------------------------------------------
void parsePath(char* path){
  String str(path);
  if (str.indexOf("status") >= 0) {
    if (str.indexOf("?") >= 0) {
      String command(path + str.indexOf("?") + 1);
    }
    else {
      
    }
  }
  else if (str.indexOf("testdoornode") >= 0) {
    if (str.indexOf("?") >= 0) {
      String command(path + str.indexOf("?") + 1);
      testDoorNode(command);
    }
    else {
      testDoorNode(String(0));
    }
  }
  else if (str.indexOf("testcameranode") >= 0) {
    if (str.indexOf("?") >= 0) {
      String command(path + str.indexOf("?") + 1);
      testCameraNode(command);
    }
    else {
      testCameraNode(String(0));
    }
  }
  else if (str.indexOf("resetsystem") >= 0) {
    if (str.indexOf("?") >= 0) {
      String command(path + str.indexOf("?") + 1);
      resetSystem(command);
    }
    else {
      resetSystem(String(0));
    }
  }
  else if (str.indexOf("editemail") >= 0) {
    if (str.indexOf("?") >= 0) {
      String command(path + str.indexOf("?") + 1);
      editEmail(command);
    }
    else {
      editEmail(String(0));
    }
  }
  else if (str.indexOf("editphone") >= 0) {
    if (str.indexOf("?") >= 0) {
      String command(path + str.indexOf("?") + 1);
      editPhoneNumber(command);
    }
    else {
      editPhoneNumber(String(0));
    }
  }
}

//----------------------------------------------------------------------------------------
// displayConnectionDetails
// Description: Prints connection details.
// Parameters: none
// Return: bool, connection status
//----------------------------------------------------------------------------------------

bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

//----------------------------------------------------------------------------------------
// buildStatus
// Description: Builds the JSON status object
// Parameters: none
// Return: none
//----------------------------------------------------------------------------------------
void buildStatus(void) {

  root["title"] = title;
  root["humidity"] = humidity;
  root["temperature"] = temperature;
  root["email"] = email;
  root["phone"] = phoneNumber;
  JsonArray& door = root.createNestedArray("door");

  for (int i = 0; i < numDoorNodes; i++) {
    door.add(doorNodes[i]);
  }
  
  JsonArray& camera = root.createNestedArray("camera");

  for (int i = 0; i < numCameraNodes; i++) {
    camera.add(cameraNodes[i]);
  }
}

//----------------------------------------------------------------------------------------
// testDoorNode
// Description: Tests the door nodes.
// Parameters: command arguments
// Return: pass or fail
//----------------------------------------------------------------------------------------
int testDoorNode(String command) {
  Serial.print(F("\nTest Door Node: "));Serial.println(command);
  return 0;
}

//----------------------------------------------------------------------------------------
// testCameraNode
// Description: Tests the camera nodes.
// Parameters: command arguments
// Return: pass or fail
//----------------------------------------------------------------------------------------
int testCameraNode(String command) {
  Serial.print(F("\nTest Camera Node: "));Serial.println(command);
  return 0;
}

//----------------------------------------------------------------------------------------
// resetSystem
// Description: Tells the system to reset.
// Parameters: command arguments
// Return: pass or fail
//----------------------------------------------------------------------------------------
int resetSystem(String command) {
  Serial.print(F("\nReset System: "));Serial.println(command);
  return 0;
}

//----------------------------------------------------------------------------------------
// editEmail
// Description: Edits the local email.
// Parameters: command arguments
// Return: pass or fail
//----------------------------------------------------------------------------------------
int editEmail(String command) {
  Serial.print(F("\nEdit Email: "));Serial.println(command);
  return 0;
}

//----------------------------------------------------------------------------------------
// editPhoneNumber
// Description: Edits the local phone number.
// Parameters: command arguments
// Return: pass or fail
//----------------------------------------------------------------------------------------
int editPhoneNumber(String command) {
  Serial.print(F("\nEdit Phone Number: "));Serial.println(command);
  return 0;
}

