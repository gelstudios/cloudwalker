#include <WiFi.h>
#include <HTTPClient.h>

uint64_t chipid;

// pin ID setup
int Bpin = 15;
int Gpin = 32;
int Rpin = 14;
int VBAT = A13;

// input voltage
int vbatt = 0;

// target colors
int r = 0;
int g = 0;
int b = 0;

// current colors
int currentR = 0;
int currentG = 0;
int currentB = 0;

// LED Pulse width modulation (frequency / analog ish)
int freq = 100;
int resolution = 8;
int PWM_MAX = 255; // depends on resolution

int Bchannel = 0;
int Gchannel = 1;
int Rchannel = 2;

// WIFI network and password
const char* ssid = "iot";
const char* password = "iloveiot";

// HTTP polling delay
int pollingDelay = 500;

// wifi + http client
WiFiClient wifi;
HTTPClient http;

const int RAINBOW = 9;
const int rainbow[RAINBOW][3] = {
  {255,  16,   0}, // orange
  {  0, 200, 255}, // teal
  {200,  32,   0}, // yellow
  {  0,   0, 255}, // blue
  {255,   0,  32}, // pink
  {  0, 200,   0}, // green
  {255,   0,   0}, // red
  {196, 160, 196}, // white
  {255,   0, 255}, // purple
};

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
    for (int i = 0; i <= 1; i++) {
      blinkStatusLed();
      blinkChannel(Rchannel);
      delay(100);
    }
    tasteTheRainbow();
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  checkBattery();
  checkWifi();

  getColorFromCloud();

  fadeColor();
  // if you dont like fade, use update :)
  // updateColor();

  delay(pollingDelay);
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }
}

void getColorFromCloud() {
//  if (http.connected() != true){
      http.begin("http://api.iot.shoes/checkin/1");
//  }
  http.setReuse(true);
  http.setTimeout(500);
  // report voltage on request
  http.addHeader("X-cloudwalker-vbatt", String(vbatt));

  int httpStatusCode = http.GET();
  if (httpStatusCode > 0) {
    blinkStatusLed();
    String payload = http.getString();
    Serial.print(httpStatusCode);
    Serial.print(": ");
    Serial.println(payload);

    int requestR, requestG, requestB;

    if (sscanf(payload.c_str(), "%d,%d,%d", &requestR, &requestG, &requestB) == 3) {
      if (requestR >= 0 && requestR <= PWM_MAX) {
        if (requestG >= 0 && requestG <= PWM_MAX) {
          if (requestB >= 0 && requestB <= PWM_MAX) {
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
    Serial.print(": ");
    Serial.println(httpStatusCode);
  }
}

void blinkStatusLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
}

void blinkChannel(int CHAN){
  ledcWrite(Rchannel, 0);
  ledcWrite(Gchannel, 0);
  ledcWrite(Bchannel, 0);

  ledcWrite(CHAN, 64);
  delay(50);
  ledcWrite(CHAN, 0);
}

void updateColor() {
  ledcWrite(Rchannel, r);
  ledcWrite(Gchannel, g);
  ledcWrite(Bchannel, b);
}

void fadeColor() {
  while (r != currentR || g != currentG || b != currentB) {
    Serial.print(r);
    Serial.print(" ");
    Serial.print(currentR);

    Serial.print("\t");
    
    Serial.print(g);
    Serial.print(" ");
    Serial.print(currentG);

    Serial.print("\t");

    Serial.print(b);
    Serial.print(" ");
    Serial.println(currentB);

    stepTo(Rchannel, currentR, r);
    stepTo(Gchannel, currentG, g);
    stepTo(Bchannel, currentB, b);
  }
}

void stepTo(int CHAN, int &current, int target) {
  int v = 0;
  if (current > target) {
    v = -1;
  }
  if (current < target) {
    v = +1;
  }
  ledcWrite(CHAN, current + v);
  current += v;
}

void tasteTheRainbow() {
  for (int i = 0; i <= RAINBOW - 1; i++) {
    r = rainbow[i][0];
    g = rainbow[i][1];
    b = rainbow[i][2];
    fadeColor();
    delay(2000);
  }
}

void checkBattery(){
  vbatt = analogRead(VBAT) * 2;
  Serial.println(vbatt);
}
