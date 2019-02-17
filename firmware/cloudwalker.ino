#include <WiFi.h>
#include <HTTPClient.h>

uint64_t chipid;  

// pin ID setup
int Bpin = 15; 
int Gpin = 32;
int Rpin = 14;
int VBAT = A13;

// target colors
int r = 0;
int g = 0;
int b = 0;

// current colors
int currentR = 0;
int currentG = 0;
int currentB = 0;

// LED Pulse width modulation (frequency / analog ish)
int freq = 5000;
int Bchannel = 0;
int Gchannel = 1;
int Rchannel = 2;
int resolution = 8;

// WIFI network and password
const char* ssid = "iot";
const char* password = "iloveiot";

// HTTP polling delay
int pollingDelay = 100;

// wifi + http client 
WiFiClient wifi;
HTTPClient http;

void setup() {
	Serial.begin(115200);
  pinMode(Rpin, OUTPUT); // R
  pinMode(Gpin, OUTPUT); // G
  pinMode(Bpin, OUTPUT); // B
  pinMode(VBAT, INPUT); // battery voltage
  pinMode(LED_BUILTIN, OUTPUT);

  ledcSetup(Rchannel, freq, resolution);
  ledcSetup(Gchannel, freq, resolution);
  ledcSetup(Bchannel, freq, resolution);

  ledcAttachPin(Rpin, Rchannel);
  ledcAttachPin(Gpin, Gchannel);
  ledcAttachPin(Bpin, Bchannel);

  setupWifi();
}

void setupWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  getColorFromCloud();
  
  blinkStatusLed();

  fadeColor();
  // if you dont like fade, use update :)
  // updateColor();
 
  Serial.println(analogRead(VBAT) * 2);

  delay(pollingDelay);
}

void getColorFromCloud(){
  http.begin("http://api.iot.shoes/checkin/1");
  http.setReuse(true);
  http.setTimeout(500);

  int httpStatusCode = http.GET();
  if (httpStatusCode > 0) {
        String payload = http.getString();
        Serial.print(httpStatusCode);
        Serial.println(payload);

        int requestR, requestG, requestB;

        if (sscanf(payload.c_str(), "%d,%d,%d", &requestR, &requestG, &requestB) == 3) {
          if ( requestR >= 0 && requestR <= 256) {
            if ( requestG >= 0 && requestG <= 256) {
              if ( requestB >= 0 && requestB <= 256) {
                 r = requestR;
                 g = requestG;
                 b = requestB;
              }
            }
          }
        }
  } else {
    http.end(); // free http client handle
    Serial.print("Error on HTTP request");
    Serial.print(":");
    Serial.println(httpStatusCode);
  }
}

void blinkStatusLed(){
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
}

void updateColor(){
  ledcWrite(Rchannel, r);
  ledcWrite(Gchannel, g);
  ledcWrite(Bchannel, b);
}

void fadeColor(){
  while (r != currentR || g != currentG || b != currentB){
    Serial.println("step 2");
    stepTo(Rchannel, currentR, r);
    stepTo(Gchannel, currentG, g);
    stepTo(Bchannel, currentB, b);
  }
}

void stepTo(int CHAN, int &current, int target){
  int v = 0;

  if ( current > target ){
     v = -1;
  }
  if ( current < target ){
     v = +1;
  }
  ledcWrite(CHAN, current + v);
  current += v;
}
