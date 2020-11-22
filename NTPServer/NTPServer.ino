#include <WiFi.h>
#include <TimeLib.h>

const char* ssid       = "OurCoop";
const char* password   = "4TheChickens!";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;
int timeStampHour;
int timeStampMin;
int timeStampSec;
String timeStamp;

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(&timeinfo, "%b %d %y %H:%M:%S");
  timeStamp = String(timeinfo.tm_hour) + String(timeinfo.tm_min) + String(timeinfo.tm_sec);
  Serial.print("timeStamp: ");
  Serial.println(timeStamp);
}

void setup()
{
  Serial.begin(115200);
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop()
{
  delay(1000);
  printLocalTime();
}
