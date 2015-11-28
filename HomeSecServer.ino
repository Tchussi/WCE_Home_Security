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
#define MAX_CAMERAS 10

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

int numDoorNodes = 4;
int numCameraNodes = 2;
int doorNodes[MAX_DOORS] = {1, 2, 3, 4};
String cameraNodes[MAX_CAMERAS] = {"1", "2"};

DynamicJsonBuffer jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

//----------------------------------------------------------------------------------------
// setup
// Description: Setup the CC3000 and begin HTTP Server
// Parameters: none
// Return: none
//----------------------------------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);
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

  // Measure from DHT
  temperature = (uint8_t)dht.readTemperature();
  humidity = (uint8_t)dht.readHumidity();

  // Build json object
  buildStatus();

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
        char jsonStatus[256];
        root.printTo(jsonStatus, 256);
        client.fastrprintln(jsonStatus);
        client.fastrprint(F("You accessed path: ")); client.fastrprintln(path);
      }
      else {
        // Unsupported action, respond with an HTTP 405 method not allowed error.
        client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
        client.fastrprintln(F(""));
      }
    }

    // Wait a short period to make sure the response had time to send before
    // the connection is closed (the CC3000 sends data asyncronously).
    delay(100);

    // Close the connection when done.
    Serial.println(F("Client disconnected"));
    client.close();
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

  root.printTo(Serial);
  Serial.println("\n");
  delay(1000);
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

