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

String masterSays;

void setup() {
  Serial.begin(115200);
  Serial.println("Coop_Door_Control_v0.02 - 7/2/2019");
  Serial2.begin(9600);
  Serial.println("Door Control Reporting");
}

void loop() {
  if (Serial2.available() > 0) {
          // read the incoming byte:
          masterSays = Serial2.readString();

          // say what you got:
          Serial.print("I received: ");
          Serial.println(masterSays);
          Serial2.print(masterSays);
  }
}
