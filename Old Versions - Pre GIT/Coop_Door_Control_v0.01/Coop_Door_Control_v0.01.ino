// Board: "DOIT ESP32 DEVKIT V1"
// Upload Speed: "921600"
// Flash Frequency: "80MHz"
// Core Debug Level: "NONE"
// Programmer: "AVRISP mkll"

// Coop_Door_Control_v0.01 = 

// print debug messages or not to serial 
const boolean SerialDisplay = true;

// door sensor definitions
const int upLimitSwitch = 23;   // red
const int downLimitSwitch = 22; // wht

// LEDs that match door sensors
const int doorOpenLED = 5;      // red
const int doorClosedLED = 4;    // green
const int doorMovingLED = 15;   // yellow

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
const int motorOpenTheDoor = 19;      // orange
const int motorCloseTheDoor = 18;     // yellow

// feeds from master
const int masterSaysOpenTheDoor = 0;
int masterSays;
int mastersInstruction;
const int masterSaysOpen = 1;
const int masterSaysClose = 0;
int masterSwitched;

// local switches
const int localSwitchUp = 34;
const int localSwitchDown = 35;

void setup() {
  pinMode (upLimitSwitch, INPUT_PULLUP);
  pinMode (downLimitSwitch, INPUT_PULLUP);
  pinMode (doorOpenLED, OUTPUT);
  pinMode (doorClosedLED, OUTPUT);
  pinMode (doorMovingLED, OUTPUT);
  pinMode (masterSaysOpenTheDoor, INPUT_PULLUP);
  pinMode (motorOpenTheDoor, OUTPUT);
  pinMode (motorCloseTheDoor, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("Coop_Door_Control_v0.01 - 7/2/2019");
  Serial2.begin(115200);
  Serial.println("Door Control Reporting");
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
        }
      }
    }
  }  
}
void debounceDownLimitSwitch() {
  downLimitVal = digitalRead(downLimitSwitch);
  if ((millis() - lastDebounceTime) > debounceDelay) {    // delay 10ms for consistent readings
    downLimitVal2 = digitalRead(downLimitSwitch);
    if (downLimitVal == downLimitVal2) {
      if (downSwitchState != downLimitVal) {
        Serial.print("downSwitchState: ");
        Serial.println(downSwitchState);
        downSwitchState = downLimitVal;
        if (downSwitchState == 0) {
          debugStringPrint("debounced down switch - door is closed");
        }
      }
    }
  }  
}
void stopCoopDoor () {
  Serial.println("running stopCoopDoor");
  digitalWrite(motorOpenTheDoor, LOW);
  digitalWrite(motorCloseTheDoor, LOW);
}
void openCoopDoor() {
  if ((upSwitchState == 1) && (downSwitchState == 0)) {
    Serial.println("opening door");
    digitalWrite(motorOpenTheDoor, HIGH);
    digitalWrite(motorCloseTheDoor, LOW);
    doorGoingUp = 1;
    doorGoingDown = 0;
  }
}
void closeCoopDoor() {
  if ((upSwitchState == 0) && (downSwitchState == 1)) {
    Serial.println("closing door");
    digitalWrite(motorOpenTheDoor, LOW);
    digitalWrite(motorCloseTheDoor, HIGH);
    doorGoingUp = 0;
    doorGoingDown = 1;
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
  if (mastersInstruction == masterSaysOpen) {
    if ((doorGoingUp == 0) && (doorGoingDown == 0)) {
      openCoopDoor();
    }
  }
  else if (mastersInstruction == masterSaysClose) {
    if ((doorGoingUp == 0) && (doorGoingDown == 0)) {
      closeCoopDoor();
    }
  }
}

void loop() {
  masterSays = digitalRead(masterSaysOpenTheDoor);
  if (masterSays == 0) {
    if (masterSwitched != masterSays) {
      Serial.print("masterSays=");
      Serial.println(masterSays);
      masterSwitched = masterSays;
      mastersInstruction = masterSaysOpen;
    }
  }
  else if (masterSays == 1) {
    if (masterSwitched != masterSays) {
      Serial.print("masterSays=");
      Serial.println(masterSays);
      masterSwitched = masterSays;
      mastersInstruction = masterSaysClose;
    }
  }
  operateCoopDoor();
  debounceUpLimitSwitch();
  debounceDownLimitSwitch();
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
}
