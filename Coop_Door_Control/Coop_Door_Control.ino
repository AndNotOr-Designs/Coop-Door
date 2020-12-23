// Board: "DOIT ESP32 DEVKIT V1"
// Upload Speed: "921600"
// Flash Frequency: "80MHz"
// Core Debug Level: "NONE"
// Programmer: "AVRISP mkll"

// version information - reference in README.md
const float Coop_Door_Control_Version = 2.09;       
const String versionDate = "12/--/2020";                  

/* Loading note:
- If you get the "Failed to connect to ESP32: Timed out... Connecting..." error when trying to upload code, it means that your ESP32 is not in flashing/uploading mode.
- Hold-down the “BOOT” button in your ESP32 board
- After you see the  “Connecting….” message in your Arduino IDE, release the finger from the “BOOT” button:
*/

#include <WiFi.h>                                         // for webserver
#include <secureSettings.h>
#include <HTTPClient.h>                                   // for ThingSpeak
#include "TimeLib.h"                                      // NTP

boolean thingSpeakOff = true;                            // turning thingspeak off for testing purposes - no tweeting!
boolean debugOn = true;                                  // debugging flag
boolean superDebugOn = true;                             // verbose debugging does cause a delay in button push response
boolean debugWithDelay = false;                           // adds 10 second delay to verbose debugging - causes issues with debouncing!

volatile boolean autoOpenOn = true;                       // switch for tracking whether to listen to commands from master or not

// blinking timer
unsigned long currentMillis = 0;                          // reference
unsigned long splitSecond = 0;                            // 2 second reference
const long interval = 500;                                // 50 ms
int blinky = LOW;                                         // blinking status

// network information
unsigned long lostWiFi = 0;                               // for timing on when ESP32 loses signal to allow for reboot
const long waitForWiFi = 30000;                           // wait time to see if WiFi gets connected before rebooting
int initialLostWiFi = 0;                                  // flag to enter into wait time to reboot
int counter = 30;                                         // for serial printing of counter till reboot

WiFiServer server(80);                                    // Set web server port number to 80
WiFiClient client(80);                                    // set web client port number to 80

// time
const char* ntpServer = "pool.ntp.org";                   // NTP server
const long  gmtOffset_sec = -18000;                       // Eastern Time Zone
const int   daylightOffset_sec = 3600;                    // one hour
String closeTimeStamp;                                    // thingspeak data
String openTimeStamp;                                     // thingspeak data
int ntpMonth;
int ntpDay;
int ntpYear;
int ntpHour;
int ntpMinute;
int ntpSecond;
int timeHasBeenSet = 0;                                   // flag for only setting time on ESP32 once
String timeStamp;                                         // used to convert time from NTP into string to send to thingspeak

String header;                                            // Variable to store the HTTP request

// Auxiliary variables to store the current output state
String doorState = "";                                    // status of door for webpage launching
String wifiSays = "";                                     // information from webpage

// ThingSpeak
int causeCode = 0;                                        // cause code for door movement
String causeCodeText = "";                                // cause code text
String causeCodeStr = "";                                 // cause code string to send to thingspeak
const char* serverName = "http://api.thingspeak.com/update";

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
unsigned long motorTimer = 0;                             // timer for motor to help prevent it from running too long
int timeTheMotor = 0;                                     // flag for timing motor movement
const long motorThirtySec = 30000;                        // motor run time too long

// LED connections and monitoring
const int coopDoorClosedLed = 15;                         // door closed: green LED
const int coopDoorOpenLed = 2;                            // door open: red LED
const int coopDoorMovingLed = 4;                          // door moving/stopped: yellow LED
const int commandToCoopLed = 25;                          // outside command coming in: blue LED inside box
const int wifiConnected = 5;                              // wifi connected: blue LED by window
const int wifiNotConnected = 26;                          // wifi not connected: red LED inside box

// PWM
const int freq = 5000;                                    // brightness of WiFi connected LED
const int ledChannel = 0;
const int resolution = 8;
int dutyCycle;                                            // variable for how bright to set blue LED
const int ledBrightness = 100;                            // how bright when door open

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

// override buttons
int overrideA = 32;                                       // button for engaging motor in direction 1
int overrideB = 33;                                       // button for engaging motor in direction 2
int overrideAPressed = 0;                                 // button A
int overrideBPressed = 0;                                 // button B
int overrideAOn = 0;                                      // status tracking to turn motor off when button released
int overrideBOn = 0;                                      // status tracking to turn motor off when button released

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
const int fromAutomaticControlOn = 16;
const int fromAutomaticControlOff = 17;
const int doorClosed = 18;
const int doorOpened = 19;

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
  pinMode (overrideA, INPUT);
  pinMode (overrideB, INPUT);
  pinMode (wifiNotConnected, OUTPUT);

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(wifiConnected, ledChannel);

  Serial.begin(115200);                                   // serial monitor
  Serial2.begin(9600);                                    // master connection

// serial monitor header
  Serial.println();
  Serial.println("/- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\\");
  Serial.print("|                                 Coop Door Control v");
  Serial.print(Coop_Door_Control_Version);
  Serial.println("                                   |");
  Serial.print("|                                     --");
  Serial.print(versionDate);
  Serial.println("--                                        |");
  Serial.println("|                                         And!Or                                            |");
  Serial.println("|                                                                                           |");
  Serial.print("| Debug Status: ");
  if (debugOn == true) {
    Serial.print("on, Super Debug Status: ");
    if (superDebugOn == true) {
      Serial.println("on                                                  |");
    }
    else {
      Serial.println("off                                                 |");
    }
  }
  else {
    Serial.print("off, Super Debug Status: ");
    if (superDebugOn == true) {
      Serial.println("on                                                 |");
    }
    else {
      Serial.println("off                                                |");
    }
  }
  Serial.println("| Serial 2 = Coop Door Control connection @9600                                             |");
  Serial.println("|                                                                                           |");

  Serial.print("| Connecting to: ");
  Serial.print(ssid);
  Serial.println("                                                                    |");
  WiFi.begin(ssid, password);
  Serial.print("| ");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite (wifiNotConnected, HIGH);                // not connected LED
    Serial.print(counter);                                // countdown timer
    Serial.print(".");
    counter--;                                            // decrement counter
    delay(1000);
    if(millis() > 30000) {                                // hasn't connected to WiFi for 30 seconds
      Serial.println("         |");
      Serial.println("| No WiFi connection, rebooting                                                             |");
      ESP.restart();                                      // restart
    }
  } 
  Serial.println(" WiFi Connected                                                                        |");
  Serial.print("| IP Address: ");
  Serial.print(WiFi.localIP());
  Serial.println("                                                                   |");
  Serial.print("| MAC address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("                                                            |");
  Serial.println("|                                                                                           |");
  digitalWrite (wifiNotConnected, LOW);                   // not connected LED
  counter = 30;                                           // reset counter for when wifi lost
  Serial.println("\\- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -/");
// end serial monitor header

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // NTP
  Serial.print("  - ");
  printLocalTime();                                       // show current time
  Serial.println();

  server.begin();                                         // open server

  causeCode = 25;                                         // reference the readme for cause code
  causeCodeText = "react: esp32 SETUP run";

  lastDebounceTime = millis();                            // start debounce tracking
  delay(debounceDelay + 50);                              // wait for delay to execute debounce
  debounceBottomReed();                                   // door closed status
  debounceTopReed();                                      // door open status
}

void motorTimerMonitor() {
  if(timeTheMotor == 1) {
    if (debugOn == true) {
    // motor timer
      Serial.print("Motor Timing: motor auto stop in: ");
      Serial.print((motorThirtySec - (millis() - motorTimer)) / 1000);
      Serial.println(" seconds");
    }
    if ((millis() - motorTimer) > motorThirtySec) {
      stopCoopDoor();                                     // stop the door
      causeCode = 300;                                    // reference the readme for cause code
      causeCodeText = "react: motor running more than 30 seconds";
      motorTimer = 0;                                     // clear timer
    }
  }
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
  timeTheMotor = 0;                                       // turn off motor timer
  motorTimer = 0;                                         // clear timer
  debugStatus(fromStopCoopDoor);                          // debugging
}
void closeCoopDoor() {                                    // close coop door
  debounceBottomReed();                                   // door down status
  debounceTopReed();                                      // door up status
  if (bottomSwitchVal != 1) {                             // is the door open?
    timeTheMotor = 1;                                     // turn on the timer
    motorTimer = millis();                                // set timer
    digitalWrite (doorMotorUp, LOW);                      // turn off up motor
    digitalWrite (doorMotorDown, HIGH);                   // turn on down motor
    doorGoingUp = 0;                                      // door not going up
    doorGoingDown = 1;                                    // door going down
    if (bottomSwitchVal == 1) {                           // is door closed?
      stopCoopDoor();
    }
    debugStatus(fromCloseCoopDoor);                       // debugging
    printLocalTime();                                     // timestamp
    closeTimeStamp = timeStamp;                           // set timestamp
    sendToThingSpeak(doorClosed);                         // thingspeak
  }
}
void openCoopDoor() {                                     // open coop door
  debounceBottomReed();                                   // door down status
  debounceTopReed();                                      // door up status
  if (topSwitchVal != 1) {                                // is the door closed?
    timeTheMotor = 1;                                     // turn on the timer
    motorTimer = millis();                                // set timer
    digitalWrite (doorMotorUp, HIGH);                     // turn off up motor
    digitalWrite (doorMotorDown, LOW);                    // turn on down motor
    doorGoingUp = 1;                                      // door going up
    doorGoingDown = 0;                                    // door not going down
    if (topSwitchVal == 1) {                              // is door open?
      stopCoopDoor();
    }
    debugStatus(fromOpenCoopDoor);                        // debugging
    printLocalTime();                                     // timestamp
    openTimeStamp = timeStamp;                            // set timestamp
    sendToThingSpeak(doorOpened);                         // thingspeak
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
  if ((masterSays == "raise coop door") || (buttonSaysUp == 1) || (wifiSays == "raise coop door")) {
    debugStatus(fromOperateCoopDoorUp);                   // debugging
    if (masterSays == "raise coop door") {                // open door cause codes below
      causeCode = 100;
      causeCodeText = "react: masterSays raise coop door";
    } else if (buttonSaysUp == 1) {
      causeCode = 125;
      causeCodeText = "react: buttonSaysUp raise coop door";
    } else if (wifiSays == "raise coop door") {
      causeCode = 150;
      causeCodeText = "react: wifiSays raise coop door";
    } else {
      causeCode = 175;
      causeCodeText = "react: raise door unknown";
    }
    masterSays = "";                                      // clear what the master says so only execute once
    wifiSays = "";                                        // clear what the webpage says so only execute once
    openCoopDoor();                                       // open the door
  }
  if ((masterSays == "lower coop door") || (buttonSaysDown == 1) || (wifiSays == "lower coop door")) {
    debugStatus(fromOperateCoopDoorDown);                 // debugging
    if (masterSays == "lower coop door") {                // close cause codes below
      causeCode = 200;
      causeCodeText = "react: masterSays lower coop door";
    } else if (buttonSaysDown == 1) {
      causeCode = 225;
      causeCodeText = "react: buttonSaysDown lower coop door";
    } else if (wifiSays == "lower coop door") {
      causeCode = 250;
      causeCodeText = "react: wifisays lower coop door";
    } else {
      causeCode = 275;
      causeCodeText = "react: lower door unknown";
    }
    masterSays = "";                                      // clear what the master says so only execute once
    wifiSays = "";                                        // clear what the webpage says so only execute once
    closeCoopDoor();                                      // close the door
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
    digitalWrite (coopDoorOpenLed, HIGH);                 // turn on open led
    digitalWrite (coopDoorClosedLed, LOW);                // turn off closed led
    digitalWrite (coopDoorMovingLed, LOW);                // turn off moving led
    doorState = "open";                                   // webpage tracking
    messageToMaster = "door up>";                         // tell the master door is up
  }
  if(((doorGoingDown == 1) || (doorGoingUp == 1)) && ((bottomSwitchState == 0) && (topSwitchState == 0))) {  // neither read switch closed
    digitalWrite (coopDoorMovingLed, HIGH);               // turn on moving led
    digitalWrite (coopDoorClosedLed, LOW);                // turn off closed led
    digitalWrite (coopDoorOpenLed, LOW);                  // turn off open led
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
  if(WiFi.status() == WL_CONNECTED) {
    if(doorState == "closed") {           
      dutyCycle = 100;                                    // min brightness      
    } else if (doorState == "open") {     
      dutyCycle = 180;                                    // full brightness
    } else  {                             
      dutyCycle = 255;                                    // full brightness
    }
    digitalWrite(wifiNotConnected, LOW);                  // not connected LED
    lostWiFi = 0;                                         // tracking for when lost connection millis counts
    counter = 30;                                         // timer reset
  } else {
    if(initialLostWiFi == 0) {                            // only set lostWiFi tracking to currentMillis once
      lostWiFi = currentMillis;                           // timing for countdown
      initialLostWiFi = 1;                                // only allow lostWiFi counting to set once
      Serial.print("Lost WiFi, rebooting countdown: ");
    } else {
      digitalWrite (wifiNotConnected, HIGH);              // not connected LED
      dutyCycle = 0;                                      // turn LED off
      Serial.print(counter);                              // countdown timer
      Serial.print(".");                                  
      counter--;
      delay(1000);
      if((currentMillis - lostWiFi) >= waitForWiFi) {     // hasn't connected to WiFi for 30 seconds
        Serial.println("No WiFi connection, rebooting");
        ESP.restart();                                    // restart ESP32
      }
    }
  }
}

// automatic control
void automaticControl() {
  if (wifiSays == "turn on automatic door control") {
    debugStatus(fromAutomaticControlOn);                  // debugging
    wifiSays = "";                                        // clear what the webpage says so only execute once
    autoOpenOn = true;                                    // engage automatic door control
  }
  if (wifiSays == "turn off automatic door control") {
    debugStatus(fromAutomaticControlOff);                 // debugging
    wifiSays = "";                                        // clear what the webpage says so only execute once
    autoOpenOn = false;                                   // disengage automatic door control
  }
}

// override buttons
void overrideButtons() {
  overrideAPressed = digitalRead(overrideA);              // read the top button state
  if (overrideAPressed == HIGH) {
    digitalWrite (doorMotorUp, HIGH);                     // turn on up motor
    digitalWrite (doorMotorDown, LOW);                    // turn off down motor
    overrideAOn = HIGH;
    Serial.println("A pressed");
  } else if (overrideAOn == HIGH){
    overrideAOn = LOW;
    digitalWrite (doorMotorUp, LOW);                      // turn off up motor
    digitalWrite (doorMotorDown, LOW);                    // turn off down motor
    Serial.println("A released");
  }
  overrideBPressed = digitalRead(overrideB);              // read the top button state
  if (overrideBPressed == HIGH) {
    digitalWrite (doorMotorUp, LOW);                      // turn on up motor
    digitalWrite (doorMotorDown, HIGH);                   // turn off down motor
    overrideBOn = HIGH;
    Serial.println("B pressed");
  } else if (overrideBOn == HIGH){
    overrideBOn = LOW;
    digitalWrite (doorMotorUp, LOW);                      // turn off up motor
    digitalWrite (doorMotorDown, LOW);                    // turn off down motor
    Serial.println("B released");
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
    Serial.println();
    Serial.print("void recWithEndMarker: ");
    Serial.print(rc);
    if (rc == endMarker) {                                // do we see the end character
      if (autoOpenOn == true) {                           // only listen to the master when automatic door control is engaged
        newData = true;                                   // entire string is present, time to process
        masterSaysNewData();                              // process new data
      } else {                                            // automode off
        masterSays = "";                                  // clear buffer
        if (debugOn == true) {                            // only send if debug on
          Serial.println("autoOpenOn = false - IGNORING MASTER");
          Serial2.print("autoOpenOn = false - IGNORING MASTER>");
        }
        newData = false;                                  // make sure to not execute new command
      }
    } else {
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
                client.stop();                            // Close the connection
            } else if (header.indexOf("GET /5/on") >= 0) {// the stop coop door button was pressed
                wifiSays = "stop coop door";                
                client.stop();                            // Close the connection
            } else if (header.indexOf("GET /9/on") >= 0) {// the raise coop door button was pressed
                wifiSays = "raise coop door";               
                client.stop();                            // Close the connection
            } else if (header.indexOf("GET /7/on") >=0) { // the stop connection button was pressed
                client.stop();                            // Close the connection
                Serial.println("Client closed connection");
            } else if (header.indexOf("GET /8/on") >=0) { // engage automatic door control - listen to the master
                wifiSays = "turn on automatic door control";
            } else if (header.indexOf("GET /8/off") >=0) {// disengage automatic door control - ignore the master
                wifiSays = "turn off automatic door control";
            } 
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            client.println("<style>html {font-family: arial; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("hr.dashed {border-top: 2px dashed #641aa1; boarder-radius: 1px; width: 310px; margin-left: auto; margin-right: auto;}");
            client.println("hr.solid {border-top: 5px solid #641aa1; boarder-radius: 2px; width: 350px; margin-left: auto; margin-right: auto;}");
            client.println(".button {background-color: #EDAA3E; border-radius: 38px; border: none; padding: 14px 40px;"); // yellow
            client.println("text-decoration: none; font-size: 25px; color: #641AA1; width: 300px; margin: 2px; cursor: pointer;}");
            client.println(".button1 {background-color: #0EED63; border-radius: 38px; border: none; padding: 14px 40px;"); // red
            client.println("text-decoration: none; font-size: 25px; color: #641AA1; width: 300px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #12A148; border-radius: 38px; border: none; padding: 14px 40px;"); // blue
            client.println("text-decoration: none; font-size: 25px; color: #641AA1; width: 300px; margin: 2px; cursor: pointer;}");
            client.println(".button3 {background-color: #aa0000; border-radius: 19px; border: none; padding: 8px 40px;");
            client.println("text-decoration: none; font-size: 15px; width: 200px; margin: 2px; cursor: pointer;}");
            client.println(".button4 {background-color: #9426ED; border-radius: 42px; border: 2px solid #EDAA3E; color: #ffebff; padding: 8px 40px;");
            client.println("text-decoration: none; font-size: 15px; color: #cccccc; font-weight:bold; width: 300px; margin: 2px; cursor: pointer}");
            client.println(".button5 {background-color: #EDAA3E; border-radius: 42px; border: 2px solid #9426ED; color: #ffebff; padding: 8px 40px;");
            client.println("text-decoration: none; font-size: 15px; color: #aa0000; width: 300px; margin: 2px; cursor: pointer}</style>");

            // set page to refresh every CONTENT seconds  // removed in 2.07 - see readme.md
            //client.println("<META HTTP-EQUIV=\"refresh\" CONTENT= \"15\"></head>");
            client.println("</head>");
            
            // Web Page Heading
            client.println("<body bgcolor=\"#000000\"><h1><p style=\"color:white\">Coop Door Control</p></h1>");

            if (doorState=="moving") {
              client.println("<h2><p style=\"color:#641AA1\">Door State: <span style=\"color:#0EED63\">"+ doorState + "</span></p></h2>");
//              client.println("<p><a href=\"/9/off\"><button class=\"button2\">door is moving</button></a></p>");
//              client.println("<p><a href=\"/5/on\"><button class=\"button\">press to stop door</button></a></p>");
//              client.println("<p><a href=\"/6/off\"><button class=\"button2\">door is moving</button></a></p>");
            } else if (doorState=="closed") {
              client.println("<h2><p style=\"color:#641AA1\">Door State: <span style=\"color:#12A148\">"+ doorState + "</span></p></h2>");
//              client.println("<p><a href=\"/9/on\"><button class=\"button\">press to open door</button></a></p>");
//              client.println("<p><a href=\"/5/off\"><button class=\"button2\">stop not needed</button></a></p>");
//              client.println("<p><a href=\"/6/off\"><button class=\"button2\">door is closed</button></a></p>");
            } else if (doorState=="open") {
              client.println("<h2><p style=\"color:#641AA1\">Door State: <span style=\"color:#EDAA3E\">" + doorState + "</span></p></h2>");
//              client.println("<p><a href=\"/9/off\"><button class=\"button2\">door is open</button></a></p>");
//              client.println("<p><a href=\"/5/off\"><button class=\"button2\">stop not needed</button></a></p>");
//              client.println("<p><a href=\"/6/on\"><button class=\"button\">press to close door</button></a></p>");
            } else {
              client.println("<h2><p style=\"color:#641AA1\">Door State: <span style=\"color:#0EED63\">"+ doorState + "</span></p></h2>");
//              client.println("<p><a href=\"/9/on\"><button class=\"button\">press to open door</button></a></p>");
//              client.println("<p><a href=\"/5/off\"><button class=\"button2\">stop not needed</button></a></p>");
//              client.println("<p><a href=\"/6/on\"><button class=\"button\">press to close door</button></a></p>");
            }

            client.println("<p><a href=\"/9/on\"><button class=\"button\">open</button></a></p>");
            client.println("<p><a href=\"/5/on\"><button class=\"button1\">stop</button></a></p>");
            client.println("<p><a href=\"/6/on\"><button class=\"button2\">close</button></a></p>");

            client.println("<hr class=\"dashed /\"><p></p>");
            if (autoOpenOn == true) {
              client.println("<a href=\"/8/off\"><button class=\"button4\">Automatic Mode</button>");
            } else if (autoOpenOn == false) {
              client.println("<a href=\"/8/on\"><button class=\"button5\">Manual Mode</button>");
            }
            client.println("<hr class=\"solid /\"><p></p>");
            client.println("<p><img src=\"http://liskfamilycom.ipage.com/files/look_right.jpg\" style=\"width:50px\"  align=\"middle\">");
            client.println("<a href=\"/7/on\"><button class=\"button3\">close connection</button>");
            client.println("<img src=\"http://liskfamilycom.ipage.com/files/look_left.jpg\" style=\"width:50px\" align=\"middle\"></a></p>");
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
      currentMillis = millis();                           // reset timing reference
      recWithEndMarker();                                 // look for new instructions from master
      masterSaysNewData();                                // process new instructions from master
      sendToMaster();                                     // send responses back to master
      coopDoorLed();                                      // LED feedback processing
      operateCoopDoor();                                  // instructions to operate door
      doorMoving();                                       // door is moving, look to stop
      localButtons();                                     // local button presses
      automaticControl();                                 // monitor for transition between auto door mode and manual mode
    }
    header = "";                                          // Clear the header variable
    client.stop();                                        // Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void sendToThingSpeak(int status) {
  if(thingSpeakOff == false) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    causeCodeStr = "api_key=";                              // thingspeak string development
    causeCodeStr += causeCodeKey;                           
    causeCodeStr +="&field1=";
    causeCodeStr +=String(causeCode);
    causeCodeStr +="&field2=";
    causeCodeStr +=String(causeCodeText);
    switch (status) {                                       
      case doorOpened:                                      // thingspeak for door open
        causeCodeStr +="&field3=";
        causeCodeStr +=String(openTimeStamp);
        break;
      case doorClosed:                                      // thingspeak for door closed
        causeCodeStr +="&field4=";
        causeCodeStr +=String(closeTimeStamp);
        break;
    }
    int httpResponseCode = http.POST(causeCodeStr);         // post string to thingspeak
    Serial.print("cause code to ThingSpeak: ");
    Serial.println(causeCode);
    Serial.print("string to ThingSpeak: ");
    Serial.println(causeCodeStr);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

void debugStatus(int fromCall) {
  if (debugOn == true) {
    Serial.println("- - - - - - - - - -");
    printLocalTime();
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
      case fromAutomaticControlOn:
        Serial.println("automaticControl - On");
        break;
      case fromAutomaticControlOff:
        Serial.println("automaticControl - Off");
        break;
    }
    if (superDebugOn == true) {
      Serial.println("Debugging: ");
    // Variable to store the HTTP request
      Serial.print("HTTP output state: doorState: ");
      Serial.print(doorState);
      Serial.print(" wifiSays: ");
      Serial.println(wifiSays);
    // automatic mode info:
      Serial.print("automatic door control mode:");
      Serial.println(autoOpenOn);
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
      Serial.print(doorGoingUp);
      Serial.print(" | timeTheMotor= ");
      Serial.println(timeTheMotor);
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
    // ThingSpeak
      Serial.print("Last Cause Code: ");
      Serial.println(causeCode);
      Serial.print("ThingSpeak Info: last string: ");
      Serial.println(causeCodeStr);
    // timeStamps
      Serial.print("openTimeStamp: ");
      Serial.print(openTimeStamp);
      Serial.print(", closeTimeStamp: ");
      Serial.println(closeTimeStamp);
    // timing
      Serial.print("Timing: lastDebounceTime: ");
      Serial.println(lastDebounceTime);
    // override buttons - inside case
      Serial.print("door override buttons- buttonA:");
      Serial.print(overrideAPressed);
      Serial.print(", buttonB: ");
      Serial.println(overrideBPressed);
    // delay
      if (debugWithDelay == true) {
        delay(10000);                                     // 10 second delay to read results in verbose debugging
      }
    }
    Serial.println("- - - - - - - - - -");
  }
}

void printLocalTime(){
  struct tm timeinfo;                                     // gather time information
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %m/%d/%Y %H:%M:%S");
  ntpMonth = timeinfo.tm_mon +1;
  ntpDay = timeinfo.tm_mday;
  ntpYear = timeinfo.tm_year +1900;
  ntpHour = timeinfo.tm_hour;
  ntpMinute = timeinfo.tm_min;
  ntpSecond = timeinfo.tm_sec;
  if (timeHasBeenSet == 0) {                              // only set time once
    setTime(ntpHour,ntpMinute,ntpSecond,ntpDay,ntpMonth,ntpYear);
    timeHasBeenSet = 1;                                   // time setting flag
  }
  if(ntpHour < 10) {
    timeStamp = String("0") + String(ntpHour);            // adding leading zero, thingspeak ignores leading zero for hour
  } else {
    timeStamp = String(ntpHour);
  }
  if(ntpMinute < 10) {
    timeStamp += String("0") + String(ntpMinute);         // adding leading zero
  } else {
    timeStamp += String(ntpMinute);
  }
  timeStamp +=String(".");                                // format (h)hmm.ss
  if (ntpSecond <10) {
    timeStamp += String("0") + String(ntpSecond);         // adding leadiner zero
  } else {
    timeStamp +=  String(ntpSecond);
  }
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");                                      // add colon between minutes and seconds
  if(digits < 10)
    Serial.print('0');                                    // adding zero
  Serial.print(digits);
}
void loop() {
  ledcWrite(ledChannel, dutyCycle);                       // WiFi connected LED PWM control
  currentMillis = millis();                               // reset timing reference
  recWithEndMarker();                                     // look for new instructions from master
  masterSaysNewData();                                    // process new instructions from master
  sendToMaster();                                         // send responses back to master
  wifiProcessing();                                       // look for new connections on webpage
  coopDoorLed();                                          // LED feedback processing
  operateCoopDoor();                                      // instructions to operate door
  doorMoving();                                           // door is moving, look to stop
  localButtons();                                         // local button presses
  automaticControl();                                     // monitor for transition between auto door mode and manual mode
  overrideButtons();                                      // override buttons inside case for rope correction
  motorTimerMonitor();                                    // motor timer to prevent over running
}
