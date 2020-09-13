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

/* need to check
 *  1) wifi
 *  2) connection to master and communication back and forth
 *  3) research Wifi strings to check that buttons work correctly - want a stop when it's moving and the open/close when it's stopped with respective feedback. Where does this need to go???? Currently putting this "feedback" in the LED section
 *  

*/


#include <WiFi.h>

boolean debugOn = true;                            // debugging flag
boolean superDebugOn = true;                        // verbose debugging

// blinking timer
unsigned long currentMillis = 0;                // reference
unsigned long splitSecond = 0;                  // 2 second reference
const long interval = 500;                      // 50 ms
int blinky = LOW;                               // blinking status

// Replace with your network credentials
const char* ssid     = "OurCoop";
const char* password = "4TheChickens!";

// Set web server port number to 80
WiFiServer server(80);
/*
// Set IP address
//IPAddress local_IP(192, 168, 151, 248); // production IP address
IPAddress local_IP(10, 4, 20, 132); // development IP address
IPAddress gateway(10, 4, 20, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
*/
// Variable to store the HTTP request
String header;

// Auxiliary variables to store the current output state
String doorState = "";
String wifiSays = "";

// debug monitor on or off
const boolean SerialDisplay = false;   // switch to false when code ready for primetime!

// reed sensors
const int bottomSwitchPin = 35;       // bottom reed sensor
const int topSwitchPin = 34;          // top reed sensor
long lastDebounceTime = 0;            // storage for timing debounces
long debounceDelay = 100;             // 10 milliseconds
int bottomSwitchVal;                  // first reading for bottom reed switch
int bottomSwitchVal2;                 // comparison reading for debounce
int bottomSwitchState;                // variable to hold status of switch
int topSwitchVal;                     // first reading for bottom reed switch
int topSwitchVal2;                    // comparison reading for debounce
int topSwitchState;                   // variable to hold status of switch

// motor connections and monitoring
const int doorMotorDown = 13;         // connection to L298N H-Bridge, pin 3
const int doorMotorUp = 12;           // connection to L298N H-Bridge, pin 4
int doorGoingDown = 0;              // tracking door direction
int doorGoingUp = 0;                // tracking door direction

// LED connections and monitoring
const int coopDoorClosedLed = 15;     // green LED
const int coopDoorOpenLed = 2;        // red LED
const int coopDoorMovingLed = 4;      // yellow LED

// local buttons
const int localUpButton = 23;         // up button
const int localStopButton = 22;       // stop button
const int localDownButton = 21;       // down button
int localUpButtonPressed = 0;       // tracking up button press
int localStopButtonPressed = 0;     // tracking stop button press      
int localDownButtonPressed = 0;     // tracking down button press
int lastUpState = 0;                // comparing up button press
int lastStopState = 0;              // comparing stop button press      
int lastDownState= 0;               // comparing down button press
int buttonSaysUp = 0;
int buttonSaysStop = 0;
int buttonSaysDown = 0;

// master communication variables
String masterSays = "";
boolean newData = false;
String messageToMaster = "";
String slaveSaid = "";

// debug variables
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
  pinMode (5, OUTPUT);
  
  Serial.begin(115200);
  Serial2.begin(9600);
  Serial.println("Coop_Door_Control_v2.01 - 8/20/2019");
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

  lastDebounceTime = millis();
  delay(debounceDelay + 50);
  debounceBottomReed();
  debounceTopReed();
  debugStatus(fromSetup);
}

// reed debouncing
void debounceBottomReed() {
  bottomSwitchVal = digitalRead(bottomSwitchPin);         // reading 1
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay to allow for consistent readings
    bottomSwitchVal2 = digitalRead(bottomSwitchPin);      // comparison reading
    if (bottomSwitchVal == bottomSwitchVal2) {            // looking for consistent readings
      if(bottomSwitchVal != bottomSwitchState) {          // the door changed state
        bottomSwitchState = bottomSwitchVal;              // reset the door state
        Serial.println("bottomSwitchState changed");
        lastDebounceTime = currentMillis;
      }
    }
  }
  else {
    lastDebounceTime = currentMillis;
  }
}
void debounceTopReed() {
  topSwitchVal = digitalRead(topSwitchPin);               // reading 1
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay to allow for consistent readings
    topSwitchVal2 = digitalRead(topSwitchPin);            // comparison reading
    if (topSwitchVal == topSwitchVal2) {                  // looking for consistent readings
      if(topSwitchVal != topSwitchState) {                // the door changed state
        topSwitchState = topSwitchVal;                    // reset the door state
        Serial.println("topSwitchState changed");
        lastDebounceTime = currentMillis;
      }
    }
  }
  else {
    lastDebounceTime = currentMillis;
  }
}

// door controls
void stopCoopDoor(){
  Serial.println("stopCoopDoor");
  digitalWrite (doorMotorUp, LOW);    // turn off up motor
  digitalWrite (doorMotorDown, LOW);  // turn off down motor
  doorGoingUp = 0;                    // door not going up
  doorGoingDown = 0;                  // door not going down
  debounceBottomReed();
  debounceTopReed();
  debugStatus(fromStopCoopDoor);
}
void closeCoopDoor() {
  debounceBottomReed();
  debounceTopReed();
  if (bottomSwitchVal != 1) {
    Serial.println("closeCoopDoor");
    digitalWrite (doorMotorUp, LOW);    // turn off up motor
    digitalWrite (doorMotorDown, HIGH); // turn on down motor
    doorGoingUp = 0;                    // door not going up
    doorGoingDown = 1;                  // door going down
    if (bottomSwitchVal == 1) {
      stopCoopDoor();
    }
    debugStatus(fromCloseCoopDoor);
  }
}
void openCoopDoor() {
  debounceBottomReed();
  debounceTopReed();
  if (topSwitchVal != 1) {
    Serial.println("openCoopDoor");
    digitalWrite (doorMotorUp, HIGH);   // turn off up motor
    digitalWrite (doorMotorDown, LOW);  // turn on down motor
    doorGoingUp = 1;                    // door going up
    doorGoingDown = 0;                  // door not going down
    if (topSwitchVal == 1) {
      stopCoopDoor();
    }
    debugStatus(fromOpenCoopDoor);
  }
}

// get ready to stop
void doorMoving() {
  if((doorGoingUp == 1) || (doorGoingDown == 1)) {
    Serial.println("doorMoving");
    debounceBottomReed();
    debounceTopReed();
    if (doorGoingUp == 1) {
      if (topSwitchVal == 1) {
        stopCoopDoor();
      }
    }
    if (doorGoingDown == 1) {
      if (bottomSwitchVal == 1) {
        stopCoopDoor();
      }
    }
    // debugStatus(fromDoorMoving);
  }
}

// LED controls & feedback to Master and Wifi
void coopDoorLed() {
  if(bottomSwitchState == 1) {                                // bottom read switch closed
    digitalWrite (coopDoorClosedLed, HIGH);                   // turn on closed led
    digitalWrite (coopDoorOpenLed, LOW);                      // turn off open led
    digitalWrite (coopDoorMovingLed, LOW);                    // turn off moving led
    doorState = "closed";
    messageToMaster = "door down>";
  }
  if(topSwitchState == 1) {                                   // top read switch closed
    digitalWrite (coopDoorClosedLed, LOW);                    // turn off closed led
    digitalWrite (coopDoorOpenLed, HIGH);                     // turn on open led
    digitalWrite (coopDoorMovingLed, LOW);                    // turn off moving led
    doorState = "open";
    messageToMaster = "door up>";
  }
  if(((doorGoingDown == 1) || (doorGoingUp == 1)) && ((bottomSwitchState == 0) && (topSwitchState == 0))) {  // neither read switch closed
    digitalWrite (coopDoorClosedLed, LOW);                    // turn off closed led
    digitalWrite (coopDoorOpenLed, LOW);                      // turn off open led
    digitalWrite (coopDoorMovingLed, HIGH);                   // turn on moving led
    doorState = "moving";
    messageToMaster = "door moving>";
  }
  else if ((doorGoingDown == 0) && (doorGoingUp == 0) && (bottomSwitchState == 0) &&( topSwitchState == 0)) {
    digitalWrite (coopDoorClosedLed, LOW);                    // turn off closed led
    digitalWrite (coopDoorOpenLed, LOW);                      // turn off open led
    if(currentMillis - splitSecond >= interval) {
      splitSecond = currentMillis;
      blinky = !blinky;
      digitalWrite (coopDoorMovingLed, blinky);
      doorState = "stopped in the middle";
      messageToMaster = "door stopped>";
      debounceBottomReed();
      debounceTopReed();
    }
  }
}

// do something to the door
void operateCoopDoor() {
  if ((masterSays == "lower coop door") || (buttonSaysDown == 1) || (wifiSays == "lower coop door")) {
    Serial.println("operate down");
    debugStatus(fromOperateCoopDoorDown);
    masterSays = "";
    wifiSays = "";
    closeCoopDoor();
    }
  if ((masterSays == "raise coop door")  || (buttonSaysUp == 1) || (wifiSays == "raise coop door")) {
    Serial.println("operate up");
    debugStatus(fromOperateCoopDoorUp);
    masterSays = "";
    wifiSays = "";
    openCoopDoor();
    }
  if ((masterSays == "stop coop door")  || (buttonSaysStop == 1) || (wifiSays == "stop coop door")) {
    Serial.println("operate stop");
    debugStatus(fromOperateCoopDoorStop);
    masterSays = "";
    wifiSays = "";
    if((topSwitchVal != 1) || (bottomSwitchVal != 1)){
      stopCoopDoor();
    }
  }
}
// local buttons on controller
void localButtons() {
  localUpButtonPressed = digitalRead(localUpButton);
  localStopButtonPressed = digitalRead(localStopButton);
  localDownButtonPressed = digitalRead(localDownButton);
  if (localUpButtonPressed != lastUpState) {
    lastUpState = localUpButtonPressed;
    if (localUpButtonPressed == 1) {
      buttonSaysUp = 1;
      digitalWrite (5, HIGH);
      Serial.println("up button state changed");
    }
    else {
      buttonSaysUp = 0;
      digitalWrite (5, LOW);
    }
  }
  if (localDownButtonPressed != lastDownState) {
    lastDownState = localDownButtonPressed;
    if (localDownButtonPressed == 1) {
      buttonSaysDown = 1;
      digitalWrite (5, HIGH);
      Serial.println("down button state changed");
    }
    else {
      buttonSaysDown = 0;
      digitalWrite (5, LOW);
    }
  }
  if (localStopButtonPressed != lastStopState) {
    lastStopState = localStopButtonPressed;
    if (localStopButtonPressed == 1) {
      buttonSaysStop = 1;
      digitalWrite (5, HIGH);
      Serial.println("stop button state changed");
    }
    else {
      buttonSaysStop = 0;
      digitalWrite (5, LOW);
    }
  }
}

void sendToMaster() {
  if (slaveSaid != messageToMaster) {
    slaveSaid = messageToMaster;
    Serial2.print(messageToMaster);
    debugStatus(fromSendToMaster);
  }
}
void masterSaysNewData() {
  if (newData == true) {
    debugStatus(fromMasterSaysNewData);
    newData = false;
  }
}
void recWithEndMarker() {
  char endMarker = '>';
  char rc;
  while (Serial2.available() >0 && newData == false) {
    rc = Serial2.read();
    if (rc == endMarker) {
      newData = true;
      masterSaysNewData();
    }
    else {
      masterSays += rc;
    }
  }
}
void wifiProcessing() {
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /6/on") >= 0) {
              wifiSays = "lower coop door";
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } else if (header.indexOf("GET /5/on") >= 0) {
              wifiSays = "stop coop door";
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } else if (header.indexOf("GET /4/on") >= 0) {
              wifiSays = "raise coop door";
              client.stop();
              Serial.println("Client stopped connection - disconnected.");
              Serial.println("");
            } else if (header.indexOf("GET /7/on") >=0) {
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
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
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
    }
    if (superDebugOn == true) {
      Serial.println("Debugging: ");
      
      // Variable to store the HTTP request
      Serial.print("HTTP header: ");
      Serial.println(header);
      
      // Auxiliar variables to store the current output state
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
      Serial.println("- - - - - - - - - -");
    }
  }
}

void loop() {
  currentMillis = millis();
  recWithEndMarker();
  masterSaysNewData();
  sendToMaster();
  wifiProcessing();
  coopDoorLed();
  operateCoopDoor();
  doorMoving();
  localButtons();
}
