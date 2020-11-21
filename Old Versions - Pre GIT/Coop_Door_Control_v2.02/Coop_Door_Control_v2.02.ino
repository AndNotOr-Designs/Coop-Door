// Board: "DOIT ESP32 DEVKIT V1"
// Upload Speed: "921600"
// Flash Frequency: "80MHz"
// Core Debug Level: "NONE"
// Programmer: "AVRISP mkll"

/* Loading note:
- If you get the "Failed to connect to ESP32: Timed out... Connecting..." error when trying to upload code, it means that your ESP32 is not in flashing/uploading mode.
- Hold-down the “BOOT” button in your ESP32 board
- After you see the  “Connecting….” message in your Arduino IDE, release the finger from the “BOOT” button:
*/

/* NOTE NOTE NOTE NOTE NOTE
 * check the IP address for production vs. Development
 * END Note */

// Coop_Door_Control_v0.01 = not implemented, but the control from master is working along with the sensors
// Coop_Door_Control_v0.02 = adds web control, lost control from master - need to go 232... - not fully implemented yet
// Coop_Door_Control_v0.03 = adds communication with Mega Master
// Coop_Door_Control_V1.00 = implemented still need to test communication from master (haven't implemented Master code yet)
// Coop_Door_Control_V2.01 = completly cleared out all coop control stuff except wifi and communication with Master start again!
// Coop_Door_Control_V2.02 = 10/6/19 implemented. Still need to complete master code, but all other is working.

/* need to check
 *  1) wifi
 *  2) connection to master and communication back and forth
 *  3) research Wifi strings to check that buttons work correctly - want a stop when it's moving and the open/close when it's stopped with respective feedback. Where does this need to go???? Currently putting this "feedback" in the LED section
 *  

*/


#include <WiFi.h>                                         // for webserver

boolean debugOn = true;                                   // debugging flag
boolean superDebugOn = false;                             // verbose debugging

// blinking timer
unsigned long currentMillis = 0;                          // reference
unsigned long splitSecond = 0;                            // 2 second reference
const long interval = 500;                                // 50 ms
int blinky = LOW;                                         // blinking status

// network information
const char* ssid     = "OurCoop";
const char* password = "4TheChickens!";

WiFiServer server(80);                                    // Set web server port number to 80
/*
// Set IP address
//IPAddress local_IP(192, 168, 151, 248);                 // production IP address
IPAddress local_IP(192, 168, 151, 247);                   // development IP address
IPAddress gateway(192, 168, 151, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
*/
String header;                                            // Variable to store the HTTP request

// Auxiliary variables to store the current output state
String doorState = "";                                    // status of door for webpage launching
String wifiSays = "";                                     // information from webpage

// reed sensors
const int bottomSwitchPin = 35;                           // bottom reed sensor
const int topSwitchPin = 34;                              // top reed sensor
long lastDebounceTime = 0;                                // storage for timing debounces
long debounceDelay = 100;                                 // 10 milliseconds
int bottomSwitchVal;                                      // first reading for bottom reed switch
int bottomSwitchVal2;                                     // comparison reading for debounce
int bottomSwitchState;                                    // variable to hold status of switch
int topSwitchVal;                                         // first reading for bottom reed switch
int topSwitchVal2;                                        // comparison reading for debounce
int topSwitchState;                                       // variable to hold status of switch

// motor connections and monitoring
const int doorMotorDown = 13;                             // connection to L298N H-Bridge, pin 3
const int doorMotorUp = 12;                               // connection to L298N H-Bridge, pin 4
int doorGoingDown = 0;                                    // tracking door direction
int doorGoingUp = 0;                                      // tracking door direction

// LED connections and monitoring
const int coopDoorClosedLed = 15;                         // door closed: green LED
const int coopDoorOpenLed = 2;                            // door open: red LED
const int coopDoorMovingLed = 4;                          // door moving/stopped: yellow LED
const int commandToCoopLed = 5;                           // outside command coming in: blue LED

// local buttons
const int localUpButton = 23;                             // up button
const int localStopButton = 22;                           // stop button
const int localDownButton = 21;                           // down button
int localUpButtonPressed = 0;                             // tracking up button press
int localStopButtonPressed = 0;                           // tracking stop button press      
int localDownButtonPressed = 0;                           // tracking down button press
int lastUpState = 0;                                      // comparing up button press
int lastStopState = 0;                                    // comparing stop button press      
int lastDownState= 0;                                     // comparing down button press
int buttonSaysUp = 0;                                     // button tracking for up
int buttonSaysStop = 0;                                   // button tracking for stop
int buttonSaysDown = 0;                                   // button tracking for down

// master communication variables
String masterSays = "";                                   // instructions from master
boolean newData = false;                                  // flag for processing new data
String messageToMaster = "";                              // response back to master
String slaveSaid = "";                                    // watching what to send to master

// debug constants
const int fromSetup = 1;
const int fromMasterSaysNewData = 2;
const int fromSendToMaster = 3;
const int fromOpenCoopDoor = 4;
const int fromCloseCoopDoor= 5;
const int fromStopCoopDoor = 6;
const int fromOperateCoopDoorDown = 7;
const int fromOperateCoopDoorUp = 8;
const int fromOperateCoopDoorStop = 9;
const int fromDoorMoving = 10;
const int fromLocalUpButtonPressed = 11;
const int fromLocalStopButtonPressed = 12;
const int fromLocalDownButtonPressed = 13;
const int fromDebounceBottomReed = 14;
const int fromDebounceTopReed = 15;

// setup
void setup() {
  pinMode (bottomSwitchPin, INPUT);
  pinMode (topSwitchPin, INPUT);
  pinMode (doorMotorDown, OUTPUT);
  pinMode (doorMotorUp, OUTPUT);
  pinMode (coopDoorClosedLed, OUTPUT);
  pinMode (coopDoorOpenLed, OUTPUT);
  pinMode (coopDoorMovingLed, OUTPUT);
  pinMode (localUpButton, INPUT);
  pinMode (localStopButton, INPUT);
  pinMode (localDownButton, INPUT);
  pinMode (commandToCoopLed, OUTPUT);
  
  Serial.begin(115200);                                     // serial monitor
  Serial2.begin(9600);                                    // master connection
  Serial.println("Coop_Door_Control_v2.02 - 10/06/2019");
  Serial.println("Serial Monitor open @ 9600");
  Serial.println("Master Communication open on Serial2 @ 9600");
/*
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }*/
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  server.begin();

  lastDebounceTime = millis();                            // start debounce tracking
  delay(debounceDelay + 50);                              // wait for delay to execute debounce
  debounceBottomReed();                                   // door closed status
  debounceTopReed();                                      // door open status
  debugStatus(fromSetup);                                 // debugging
}

// reed debouncing
void debounceBottomReed() {                               // door closed status
  bottomSwitchVal = digitalRead(bottomSwitchPin);         // reading 1
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay to allow for consistent readings
    bottomSwitchVal2 = digitalRead(bottomSwitchPin);      // comparison reading
    if (bottomSwitchVal == bottomSwitchVal2) {            // looking for consistent readings
      if(bottomSwitchVal != bottomSwitchState) {          // the door changed state
        bottomSwitchState = bottomSwitchVal;              // reset the door state
        debugStatus(fromDebounceBottomReed);              // debugging
        lastDebounceTime = currentMillis;                 // reset reference
      }
    }
  }
  else {
    lastDebounceTime = currentMillis;                     // reset reference
  }
}
void debounceTopReed() {                                  // door open status
  topSwitchVal = digitalRead(topSwitchPin);               // reading 1
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay to allow for consistent readings
    topSwitchVal2 = digitalRead(topSwitchPin);            // comparison reading
    if (topSwitchVal == topSwitchVal2) {                  // looking for consistent readings
      if(topSwitchVal != topSwitchState) {                // the door changed state
        topSwitchState = topSwitchVal;                    // reset the door state
        debugStatus(fromDebounceTopReed);                 // debugging
        lastDebounceTime = currentMillis;                 // reset reference
      }
    }
  }
  else {
    lastDebounceTime = currentMillis;                     // reset reference
  } 
}

// door controls
void stopCoopDoor(){                                      // stop coop door
  digitalWrite (doorMotorUp, LOW);                        // turn off up motor
  digitalWrite (doorMotorDown, LOW);                      // turn off down motor
  doorGoingUp = 0;                                        // door not going up
  doorGoingDown = 0;                                      // door not going down
  debounceBottomReed();                                   // door down status
  debounceTopReed();                                      // door up status
  debugStatus(fromStopCoopDoor);                          // debugging
}
void closeCoopDoor() {                                    // close coop door
  debounceBottomReed();                                   // door down status
  debounceTopReed();                                      // door up status
  if (bottomSwitchVal != 1) {                             // is the door open?
    digitalWrite (doorMotorUp, LOW);                      // turn off up motor
    digitalWrite (doorMotorDown, HIGH);                   // turn on down motor
    doorGoingUp = 0;                                      // door not going up
    doorGoingDown = 1;                                    // door going down
    if (bottomSwitchVal == 1) {                           // is door closed?
      stopCoopDoor();
    }
    debugStatus(fromCloseCoopDoor);                       // debugging
  }
}
void openCoopDoor() {                                     // open coop door
  debounceBottomReed();                                   // door down status
  debounceTopReed();                                      // door up status
  if (topSwitchVal != 1) {                                // is the door closed?
    digitalWrite (doorMotorUp, HIGH);                     // turn off up motor
    digitalWrite (doorMotorDown, LOW);                    // turn on down motor
    doorGoingUp = 1;                                      // door going up
    doorGoingDown = 0;                                    // door not going down
    if (topSwitchVal == 1) {                              // is door open?
      stopCoopDoor();
    }
    debugStatus(fromOpenCoopDoor);                        // debugging
  }
}
void doorMoving() {                                       // door is moving
  if((doorGoingUp == 1) || (doorGoingDown == 1)) {        // door is either going up or going down
    debounceBottomReed();                                 // door down status
    debounceTopReed();                                    // door up status
    if (doorGoingUp == 1) {                               // door is going up, watch for door up
      if (topSwitchVal == 1) {                            // is the door open?
        stopCoopDoor();
      }
    }
    if (doorGoingDown == 1) {                             // door is going down, watch for door down
      if (bottomSwitchVal == 1) {                         // is the door closed?
        stopCoopDoor();
      }
    }
  }
}
void operateCoopDoor() {                                  // time to operate the coop door somehow
  if ((masterSays == "lower coop door") || (buttonSaysDown == 1) || (wifiSays == "lower coop door")) {
    debugStatus(fromOperateCoopDoorDown);                 // debugging
    masterSays = "";                                      // clear what the master says so only execute once
    wifiSays = "";                                        // clear what the webpage says so only execute once
    closeCoopDoor();                                      // close the door
    }
  if ((masterSays == "raise coop door")  || (buttonSaysUp == 1) || (wifiSays == "raise coop door")) {
    debugStatus(fromOperateCoopDoorUp);                   // debugging
    masterSays = "";                                      // clear what the master says so only execute once
    wifiSays = "";                                        // clear what the webpage says so only execute once
    openCoopDoor();                                       // open the door
    }
  if ((masterSays == "stop coop door")  || (buttonSaysStop == 1) || (wifiSays == "stop coop door")) {
    debugStatus(fromOperateCoopDoorStop);                 // debugging
    masterSays = "";                                      // clear what the master says so only execute once
    wifiSays = "";                                        // clear what the webpage says so only execute once
    if((topSwitchVal != 1) || (bottomSwitchVal != 1)){    // only run if door hasn't reached full open or close
      stopCoopDoor();                                     // stop the door
    }
  }
}
void localButtons() {                                     // both sets of buttons are tied in toghether
  localUpButtonPressed = digitalRead(localUpButton);      // read the up button state
  localStopButtonPressed = digitalRead(localStopButton);  // read the stop button state
  localDownButtonPressed = digitalRead(localDownButton);  // read the down button state
  if (localUpButtonPressed != lastUpState) {              // did the last up button state change - execute only once
    lastUpState = localUpButtonPressed;                   // update
    if (localUpButtonPressed == 1) {                      // is up pressed?
      buttonSaysUp = 1;                                   // feedback
      digitalWrite (commandToCoopLed, HIGH);              // turn on external command LED
      debugStatus(fromLocalUpButtonPressed);              // debugging
    }
    else {
      buttonSaysUp = 0;                                   // feedback
      digitalWrite (commandToCoopLed, LOW);               // turn off external command LED
    }
  }
  if (localDownButtonPressed != lastDownState) {          // did the last down button state change - execute only once
    lastDownState = localDownButtonPressed;               // update
    if (localDownButtonPressed == 1) {                    // is down pressed?   
      buttonSaysDown = 1;                                 // feedback
      digitalWrite (commandToCoopLed, HIGH);              // turn on external command LED
      debugStatus(fromLocalDownButtonPressed);            // debugging
    }
    else {
      buttonSaysDown = 0;                                 // feedback
      digitalWrite (commandToCoopLed, LOW);               // turn off external command LED
    }
  }
  if (localStopButtonPressed != lastStopState) {          // did the last stop button state change - execute only once
    lastStopState = localStopButtonPressed;               // update
    if (localStopButtonPressed == 1) {                    // is stop pressed?     
      buttonSaysStop = 1;                                 // feedback
      digitalWrite (commandToCoopLed, HIGH);              // turn on external command LED
      debugStatus(fromLocalStopButtonPressed);            // debugging
    }
    else {
      buttonSaysStop = 0;                                 // feedback
      digitalWrite (commandToCoopLed, LOW);               // turn off external command LED
    }
  }
}

// LED controls & feedback to Master and Wifi
void coopDoorLed() {
  if(bottomSwitchState == 1) {                            // bottom read switch closed
    digitalWrite (coopDoorClosedLed, HIGH);               // turn on closed led
    digitalWrite (coopDoorOpenLed, LOW);                  // turn off open led
    digitalWrite (coopDoorMovingLed, LOW);                // turn off moving led
    doorState = "closed";                                 // webpage tracking
    messageToMaster = "door down>";                       // tell the master door is down
  }
  if(topSwitchState == 1) {                               // top read switch closed
    digitalWrite (coopDoorClosedLed, LOW);                // turn off closed led
    digitalWrite (coopDoorOpenLed, HIGH);                 // turn on open led
    digitalWrite (coopDoorMovingLed, LOW);                // turn off moving led
    doorState = "open";                                   // webpage tracking
    messageToMaster = "door up>";                         // tell the master door is up
  }
  if(((doorGoingDown == 1) || (doorGoingUp == 1)) && ((bottomSwitchState == 0) && (topSwitchState == 0))) {  // neither read switch closed
    digitalWrite (coopDoorClosedLed, LOW);                // turn off closed led
    digitalWrite (coopDoorOpenLed, LOW);                  // turn off open led
    digitalWrite (coopDoorMovingLed, HIGH);               // turn on moving led
    doorState = "moving";                                 // webpage tracking
    messageToMaster = "door moving>";                     // tell the master door is moving
  }
  else if ((doorGoingDown == 0) && (doorGoingUp == 0) && (bottomSwitchState == 0) &&( topSwitchState == 0)) {
    digitalWrite (coopDoorClosedLed, LOW);                // turn off closed led
    digitalWrite (coopDoorOpenLed, LOW);                  // turn off open led
    if(currentMillis - splitSecond >= interval) {         // 2 second pause for blinking LED
      splitSecond = currentMillis;                        // reset reference
      blinky = !blinky;                                   // blink = not blink
      digitalWrite (coopDoorMovingLed, blinky);           // flash the LED
      doorState = "stopped in the middle";                // webpage tracking
      messageToMaster = "door stopped>";                  // tell the master door is stopped
      debounceBottomReed();                               // check door down status
      debounceTopReed();                                  // check door up status
    }
  }
}

// communications
void sendToMaster() {                                     // response back to Master on status
  if (slaveSaid != messageToMaster) {                     // did the message change
    slaveSaid = messageToMaster;                          // update
    Serial2.print(messageToMaster);                       // send response
    debugStatus(fromSendToMaster);                        // debugging
  }
}
void masterSaysNewData() {                                // new instructions from master
  if (newData == true) {
    debugStatus(fromMasterSaysNewData);                   // debugging
    newData = false;                                      // reset reference
  }
}
void recWithEndMarker() {                                 // process strings from master
  char endMarker = '>';                                   // define end of string character
  char rc;                                                // receive character
  while (Serial2.available() >0 && newData == false) {    // data is there
    rc = Serial2.read();                                  // read the data
    if (rc == endMarker) {                                // do we see the end character
      newData = true;                                     // entire string is present, time to process
      masterSaysNewData();                                // process new data
    }
    else {
      masterSays += rc;                                   // no end string yet, continue to build string
    }
  }
}
void wifiProcessing() {
  WiFiClient client = server.available();                 // Listen for incoming clients
  if (client) {                                           // If a new client connects,
    Serial.println("New Client.");                        // print a message out in the serial port
    String currentLine = "";                              // make a String to hold incoming data from the client
    while (client.connected()) {                          // loop while the client's connected
      if (client.available()) {                           // if there's bytes to read from the client,
        char c = client.read();                           // read a byte, then
        Serial.write(c);                                  // print it out the serial monitor
        header += c;
        if (c == '\n') {                                  // if the byte is a newline character
          if (currentLine.length() == 0) {                // if the current line is blank, you got two newline characters in a row. that's the end of the client HTTP request, so send a response:
            client.println("HTTP/1.1 200 OK");            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)and a content-type so the client knows what's coming, then a blank line:
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // process information from webpage button presses
            if (header.indexOf("GET /6/on") >= 0) {       // the lower coop door button was pressed
              wifiSays = "lower coop door";
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } else if (header.indexOf("GET /5/on") >= 0) {// the stop coop door button was pressed
              wifiSays = "stop coop door";                
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } else if (header.indexOf("GET /4/on") >= 0) {// the raise coop door button was pressed
              wifiSays = "raise coop door";               
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } else if (header.indexOf("GET /7/on") >=0) { // the stop connection button was pressed
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } 
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: arial; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button {background-color: #007000; border-radius: 38px; border: 3px solid #4b8f29; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 25px; width: 300px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555; border-radius: 38px; border: 3px solid #4b8f29; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 25px; width: 300px; margin: 2px; cursor: pointer;}");
            client.println(".button3 {background-color: #aa0000; border-radius: 19px; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 15px; width: 200px; margin: 2px; cursor: pointer;}</style></head>");
            
            // Web Page Heading
            client.println("<body bgcolor=\"#000000\"><h1><p style=\"color:white\">Coop Door Control</p></h1>");
            
            if (doorState=="open") {
              client.println("<h2><p style=\"color:purple\">Door State: <span style=\"color: red\">" + doorState + "</span></p></h2>");
              client.println("<p><a href=\"/4/off\"><button class=\"button2\">door is open</button></a></p>");
              client.println("<p><a href=\"/5/off\"><button class=\"button2\">stop not needed</button></a></p>");
              client.println("<p><a href=\"/6/on\"><button class=\"button\">press to close door</button></a></p>");
              client.println("<hr width=\"50%\"><p></p>");
              client.println("<p><img src=\"http://liskfamilycom.ipage.com/files/look_right.jpg\" style=\"width:50px\"  align=\"middle\">");
              client.println("<a href=\"/7/on\"><button class=\"button3\">close connection</button>");
              client.println("<img src=\"http://liskfamilycom.ipage.com/files/look_left.jpg\" style=\"width:50px\" align=\"middle\"></a></p>");
            } else if (doorState=="closed") {
              client.println("<h2><p style=\"color:purple\">Door State: <span style=\"color:green\">"+ doorState + "</span></p></h2>");
              client.println("<p><a href=\"/4/on\"><button class=\"button\">press to open door</button></a></p>");
              client.println("<p><a href=\"/5/off\"><button class=\"button2\">stop not needed</button></a></p>");
              client.println("<p><a href=\"/6/off\"><button class=\"button2\">door is closed</button></a></p>");
              client.println("<hr width=\"50%\"><p></p>");
              client.println("<p><img src=\"http://liskfamilycom.ipage.com/files/look_right.jpg\" style=\"width:50px\"  align=\"middle\">");
              client.println("<a href=\"/7/on\"><button class=\"button3\">close connection</button>");
              client.println("<img src=\"http://liskfamilycom.ipage.com/files/look_left.jpg\" style=\"width:50px\" align=\"middle\"></a></p>");
            } else if (doorState=="moving") {
              client.println("<h2><p style=\"color:purple\">Door State: <span style=\"color:yellow\">"+ doorState + "</span></p></h2>");
              client.println("<p><a href=\"/4/off\"><button class=\"button2\">door is moving</button></a></p>");
              client.println("<p><a href=\"/5/on\"><button class=\"button\">press to stop door</button></a></p>");
              client.println("<p><a href=\"/6/off\"><button class=\"button2\">door is moving</button></a></p>");
              client.println("<hr width=\"50%\"><p></p>");
              client.println("<p><img src=\"http://liskfamilycom.ipage.com/files/look_right.jpg\" style=\"width:50px\"  align=\"middle\">");
              client.println("<a href=\"/7/on\"><button class=\"button3\">close connection</button>");
              client.println("<img src=\"http://liskfamilycom.ipage.com/files/look_left.jpg\" style=\"width:50px\" align=\"middle\"></a></p>");
            } else {
              client.println("<h2><p style=\"color:purple\">Door State: <span style=\"color:orange\">"+ doorState + "</span></p></h2>");
              client.println("<p><a href=\"/4/on\"><button class=\"button\">press to open door</button></a></p>");
              client.println("<p><a href=\"/5/off\"><button class=\"button2\">stop not needed</button></a></p>");
              client.println("<p><a href=\"/6/on\"><button class=\"button\">press to close door</button></a></p>");
              client.println("<hr width=\"50%\"><p></p>");
              client.println("<p><img src=\"http://liskfamilycom.ipage.com/files/look_right.jpg\" style=\"width:50px\"  align=\"middle\">");
              client.println("<a href=\"/7/on\"><button class=\"button3\">close connection</button>");
              client.println("<img src=\"http://liskfamilycom.ipage.com/files/look_left.jpg\" style=\"width:50px\" align=\"middle\"></a></p>");
            }
            client.println("</body></html>");
            client.println();                             // The HTTP response ends with another blank line
            break;                                        // Break out of the while loop
          } else {                                        // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {                           // if you got anything else but a carriage return character,
          currentLine += c;                               // add it to the end of the currentLine
        }
      }
    }
    header = "";                                          // Clear the header variable
    client.stop();                                        // Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void debugStatus(int fromCall) {
  if (debugOn == true) {
    Serial.println("- - - - - - - - - -");
    Serial.print("call: ");
    switch (fromCall) {
      case fromMasterSaysNewData:
        Serial.println("masterSaysNewData");
        break;
      case fromSendToMaster:
        Serial.println("sendToMaster");
        break;
      case fromOpenCoopDoor:
        Serial.println("openCoopDoor");
        break;
      case fromCloseCoopDoor:
        Serial.println("closeCoopDoor");
        break;
      case fromStopCoopDoor:
        Serial.println("stopCoopDoor");
        break;
      case fromSetup:
        Serial.println("setup");
        break;
      case fromOperateCoopDoorDown:
        Serial.println("operateCoopDoor - Down");
        break;
      case fromOperateCoopDoorUp:
        Serial.println("operateCoopDoor - Up");
        break;
      case fromOperateCoopDoorStop:
        Serial.println("operateCoopDoor - Stop");
        break;
      case fromDoorMoving:
        Serial.println("doorMoving");
        break;
      case fromLocalUpButtonPressed:
        Serial.println("localUpButtonPressed");
        break;
      case fromLocalStopButtonPressed:
        Serial.println("localStopButtonPressed");
        break;
      case fromLocalDownButtonPressed:
        Serial.println("localDownButtonPressed");
        break;
      case fromDebounceBottomReed:
        Serial.println("debounceBottomReed");
        break;
      case fromDebounceTopReed:
        Serial.println("debounceTopReed");
        break;
    }
    if (superDebugOn == true) {
      Serial.println("Debugging: ");
    // Variable to store the HTTP request
      Serial.print("HTTP output state: doorState: ");
      Serial.print(doorState);
      Serial.print(" wifiSays: ");
      Serial.println(wifiSays);
    // debouncing data
      Serial.print("debounce bottom: bottomSwitchVal: ");
      Serial.print(bottomSwitchVal);
      Serial.print(" bottomSwitchVal2: ");
      Serial.print(bottomSwitchVal2);
      Serial.print(" bottomSwitchState: ");
      Serial.println(bottomSwitchState);
      Serial.print("debounce top: topSwitchVal: ");
      Serial.print(topSwitchVal);
      Serial.print(" topSwitchVal2: ");
      Serial.print(topSwitchVal2);
      Serial.print(" topSwitchState: ");
      Serial.println(topSwitchState);
    // motor connections and monitoring
      Serial.print("Door Motor: doorGoingDown: ");
      Serial.print(doorGoingDown);
      Serial.print(" doorGoingUp: ");
      Serial.println(doorGoingUp);
    // local buttons
      Serial.print("localUpButtonPressed: ");
      Serial.print(localUpButtonPressed);
      Serial.print(" localStopButtonPressed: ");
      Serial.print(localStopButtonPressed);
      Serial.print(" localDownButtonPressed: ");
      Serial.println(localDownButtonPressed);
      Serial.print("lastUpState: ");
      Serial.print(lastUpState);
      Serial.print(" lastStopState: ");
      Serial.print(lastStopState);
      Serial.print(" lastDownState: ");
      Serial.println(lastDownState);
      Serial.print("buttonSaysUp: ");
      Serial.print(buttonSaysUp);
      Serial.print(" buttonSaysStop: ");
      Serial.print(buttonSaysStop);
      Serial.print(" buttonSaysDown: ");
      Serial.println(buttonSaysDown);
    // master communication variables
      Serial.print("Master Comms: masterSays: ");
      Serial.print(masterSays);
      Serial.print(" newData: ");
      Serial.print(newData);
      Serial.print(" messageToMaster: ");
      Serial.print(messageToMaster);
      Serial.print(" slaveSaid: ");
      Serial.println(slaveSaid);
    // timing
      Serial.print("Timing: lastDebounceTime: ");
      Serial.println(lastDebounceTime);
    }
    Serial.println("- - - - - - - - - -");
  }
}

void loop() {
  currentMillis = millis();                               // reset timing reference
  recWithEndMarker();                                     // look for new instructions from master
  masterSaysNewData();                                    // process new instructions from master
  sendToMaster();                                         // send responses back to master
  wifiProcessing();                                       // look for new connections on webpage
  coopDoorLed();                                          // LED feedback processing
  operateCoopDoor();                                      // instructions to operate door
  doorMoving();                                           // door is moving, look to stop
  localButtons();                                         // local button presses
}