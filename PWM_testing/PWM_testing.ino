// the number of the LED pin
const int ledPin = 5;  // 16 corresponds to GPIO16

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
int dutyCycle =190; 
void setup(){
  Serial.begin(115200);
  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPin, ledChannel);
}
 
void loop(){
  ledcWrite(ledChannel, dutyCycle);
  if(dutyCycle == 0) {
    dutyCycle = 100;
  } else if(dutyCycle == 100) {
    dutyCycle = 200;
  } else if(dutyCycle == 200) {
    dutyCycle = 255;
  } else if(dutyCycle == 255) {
    dutyCycle = 0;
  }
  delay(500);
}
