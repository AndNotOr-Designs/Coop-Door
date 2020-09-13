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
 check the IP address for production vs. Development
END Note */

// Coop_Door_Control_v0.01 = not implemented, but the control from master is working along with the sensors
// Coop_Door_Control_v0.02 = adds web control, lost control from master - need to go 232... - not fully implemented yet
// Coop_Door_Control_v0.03 = adds communication with Mega Master
// Coop_Door_Control_V1.00 = implemented still need to test communication from master (haven't implemented Master code yet)


#include <WiFi.h>

// Replace with your network credentials
const char* ssid     = "Kayaking Moose";
const char* password = "The420Matrix!";

// Set web server port number to 80
WiFiServer server(80);

// Set IP address
//IPAddress local_IP(192, 168, 151, 248); // production IP address
IPAddress local_IP(192, 168, 151, 247); // development IP address
IPAddress gateway(192, 168, 151, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output4State = "off";
String output5State = "off";

// print debug messages or not to serial 
const boolean SerialDisplay = true;

// system power
const int systemOn = 5;         // blue LED for system power

// door sensor definitions
const int upLimitSwitch = 34;   // red
const int downLimitSwitch = 35; // wht

// LEDs that match door sensors
const int doorOpenLED = 2;      // red
const int doorClosedLED = 15;    // green
const int doorMovingLED = 4;   // yellow

// door tracking
int upLimitVal;
int upLimitVal2;        // debounce
int upSwitchState;
int downLimitVal;
int downLimitVal2;      // debounce
int downSwitchState;
int doorGoingUp;
int doorGoingDown;

// debouncing delays
long lastDebounceTime = 0;
long debounceDelay = 100;

// motor control
const int motorOpenTheDoor = 12;      // orange
const int motorCloseTheDoor = 13;     // yellow

// buttons
const int buttonUp = 23;
const int buttonStop = 22;
const int buttonDown = 21;
int buttonUpPressed = 0;
int buttonStopPressed = 0;
int buttonDownPressed = 0;

// feeds from master
String masterSays;
boolean newData = false;

void setup() {
  pinMode (upLimitSwitch, INPUT_PULLUP);
  pinMode (downLimitSwitch, INPUT_PULLUP);
  pinMode (doorOpenLED, OUTPUT);
  pinMode (doorClosedLED, OUTPUT);
  pinMode (doorMovingLED, OUTPUT);
  pinMode (motorOpenTheDoor, OUTPUT);
  pinMode (motorCloseTheDoor, OUTPUT);
  pinMode (buttonUp, INPUT);
  pinMode (buttonStop, INPUT);
  pinMode (buttonDown, INPUT);
  
  pinMode (systemOn, OUTPUT);
  //digitalWrite(systemOn, HIGH);
  
  Serial.begin(115200);
  Serial.println("Coop_Door_Control_v0.02 - 7/2/2019");
  Serial2.begin(9600);
  Serial.println("Master Communication open @ 9600");
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void debugStringPrint(String whatToPrint) {
  if(SerialDisplay) {
    Serial.println(whatToPrint);
  }  
}
  
void debounceUpLimitSwitch() {
  upLimitVal = digitalRead(upLimitSwitch);
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay 10ms for consistent readings
    upLimitVal2 = digitalRead(upLimitSwitch);
    if (upLimitVal == upLimitVal2) {
      if (upSwitchState != upLimitVal) {
        Serial.print("upSwitchState: ");
        Serial.println(upSwitchState);
        upSwitchState = upLimitVal;
        if (upSwitchState == 0) {
          debugStringPrint("debounced up switch - door is open");
        Serial2.print("door up>");
        }
      }
    }
  }  
}
void debounceDownLimitSwitch() {
  downLimitVal = digitalRead(downLimitSwitch);
  Serial.print("downval=");
  Serial.println(downLimitVal);
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay 10ms for consistent readings
    downLimitVal2 = digitalRead(downLimitSwitch);
    Serial.print("downval2=");
    Serial.println(downLimitVal2);
    if (downLimitVal == downLimitVal2) {
      if (downSwitchState != downLimitVal) {
        Serial.print("downSwitchState: ");
        Serial.println(downSwitchState);
        downSwitchState = downLimitVal;
        if (downSwitchState == 0) {
          debugStringPrint("debounced down switch - door is closed");
        Serial2.print("door down>");
        }
      }
    }
  }  
}
void stopCoopDoor () {
  Serial.println("running stopCoopDoor");
  digitalWrite(motorOpenTheDoor, LOW);
  digitalWrite(motorCloseTheDoor, LOW);
  output5State = "stop";
  doorGoingUp = 0;
  doorGoingDown = 0;
}
void openCoopDoor() {
  if ((upSwitchState == 1) && (downSwitchState == 0)) {
    Serial.println("opening door");
    digitalWrite(motorOpenTheDoor, HIGH);
    digitalWrite(motorCloseTheDoor, LOW);
    doorGoingUp = 1;
    doorGoingDown = 0;
    output4State = "open";
    output5State = "moving";
    Serial2.print("door opening>");
  }
}
void closeCoopDoor() {
  if ((upSwitchState == 0) && (downSwitchState == 1)) {
    Serial.println("closing door");
    digitalWrite(motorOpenTheDoor, LOW);
    digitalWrite(motorCloseTheDoor, HIGH);
    doorGoingUp = 0;
    doorGoingDown = 1;
    output4State = "close";
    output5State = "moving";
    Serial2.print("door closing>");
  }
}
void getReadyToStop() {
    if (doorGoingUp == 1) {
      if(upSwitchState == 0) {
        stopCoopDoor();
        doorGoingUp = 0;
      }
    }
    if (doorGoingDown == 1) {
      if(downSwitchState == 0) {
        stopCoopDoor();
        doorGoingDown = 0;
      }
    }
}

void operateCoopDoor(){
  Serial.println("operateCoopDoor executed");
  if (masterSays == "raise coop door") {
    masterSays = "";
    Serial.println("operating door - raise");
    if ((doorGoingUp == 0) && (doorGoingDown == 0)) {
      openCoopDoor();
    }
  }
  else {
    Serial.println("not raise");
  }
  if (masterSays == "lower coop door") {
    masterSays = "";
    Serial.println("operating door - lower");
    if ((doorGoingUp == 0) && (doorGoingDown == 0)) {
      closeCoopDoor();
    }
  }
  else {
    Serial.println("not lower");
  }
}

void masterSaysNewData() {
  if (newData == true) {
    Serial.print("Master says ");
    Serial.println(masterSays);
    operateCoopDoor();
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

void buttons() {
  if (buttonUpPressed == HIGH) {
    Serial.println("buttonUpPressed");
    if(doorGoingDown == HIGH) {
      stopCoopDoor();
      delay (10);
        openCoopDoor();
    }
    else {
      openCoopDoor();
    }
  }
  if (buttonStopPressed == HIGH) {
    Serial.println("buttonStopPressed");
    stopCoopDoor();
  }
  if (buttonDownPressed == HIGH) {
    Serial.println("buttonDownPressed");
    if(doorGoingUp == HIGH) {
      stopCoopDoor();
      delay (10);
        closeCoopDoor();
    }
    else {
      closeCoopDoor();
    }
  }
}

void loop() {
  recWithEndMarker();
  masterSaysNewData();
  debounceUpLimitSwitch();
  debounceDownLimitSwitch();
  buttonUpPressed = digitalRead(buttonUp);
  buttonStopPressed = digitalRead(buttonStop);
  buttonDownPressed = digitalRead(buttonDown);
  buttons();
  if (upSwitchState == 0) {
    digitalWrite(doorOpenLED, HIGH);
  } else {
    digitalWrite(doorOpenLED, LOW);
  }
  if (downSwitchState == 0) {
    digitalWrite(doorClosedLED, HIGH);
  } else {
    digitalWrite(doorClosedLED, LOW);
  }
  if ((downSwitchState == 1) && (upSwitchState == 1)) {
    digitalWrite(doorMovingLED, HIGH);
  } else {
    digitalWrite(doorMovingLED, LOW);
    getReadyToStop();
  }
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
            if (header.indexOf("GET /4/on") >= 0) {
              output4State = "open";
              output5State = "moving";
              openCoopDoor();
            } else if (header.indexOf("GET /4/off") >= 0) {
              output4State = "close";
              output5State = "moving";
              closeCoopDoor();
            } else if (header.indexOf("GET /5/on") >=0) {
              output5State = "stop";
              stopCoopDoor();
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Coop Door Control</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 4  
            client.println("<p>Door State " + output4State + "</p>");
            // If the output4State is off, it displays the ON button       
            if (output4State=="close") {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">open</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">close</button></a></p>");
            } 

            if (output5State=="moving") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">stop</button></a></p>");
            } else {
              client.println("<p><button class=\"button button2\">stopped</button></a></p>");
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
