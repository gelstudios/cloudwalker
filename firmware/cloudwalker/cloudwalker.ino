#include <WiFi.h>
#include <HTTPClient.h>

const char* VERSION = "0.5b";

// pin ID setup
int Bpin = 15;
int Gpin = 32;
int Rpin = 14;
int VBAT = A13;

// input voltage
int vbatt = 0;
const int LOW_BATT = 3400; // 3.4 volts is ~90% discharge

// target colors
int r = 0;
int g = 0;
int b = 0;

// LED pulse width modulation (PWM) setup
int freq = 200;
int resolution = 8;
int PWM_MAX = 255; // depends on resolution

// LED pwm control channels
int Bchannel = 0;
int Gchannel = 1;
int Rchannel = 2;

// WIFI network and password
const char* ssid = "iot";
const char* password = "iloveiot";

// HTTP stuff
String URL = "http://api.iot.shoes/checkin/";
char mac[13];
int pollingDelay = 500;
String payload;

// wifi + http client
WiFiClient wifi;
HTTPClient http;

const int RAINBOW = 9;
const int rainbow[RAINBOW][3] = {
  {255,  16,   0}, // orange
  {  0, 200, 255}, // teal
  {255,  64,   0}, // yellow
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

  checkBattery();
  setURL();
  setupWifi();
  payload.reserve(128);
}

void setupWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    notifyNoWifi();
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void notifyNoWifi() {
  updateTargetColor(64, 0, 0);
  for (int i = 1; i <= 2; i++) {
    blinkStatusLed();
    blinkCurrentColor(1, 50, 0);
    delay(100);
  }
  tasteTheRainbow(1000);
}

void setURL() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(mac, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  Serial.println(mac);
  URL = String(URL + mac);
}

void loop() {
  checkBattery();

  checkWifi();

  getCommandFromCloud();

  handleCommand(payload);

  delay(pollingDelay);
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }
}

void getCommandFromCloud() {
  http.begin(URL);
  http.setReuse(true);
  http.setTimeout(500);
  // report version, uptime, voltage on request
  http.addHeader("X-cloudwalker-version", VERSION);
  http.addHeader("X-cloudwalker-uptime", String(millis()));
  http.addHeader("X-cloudwalker-vbatt", String(vbatt));

  int httpStatusCode = http.GET();
  if (httpStatusCode >= 200 && httpStatusCode < 300) {
    blinkStatusLed();
    payload = http.getString();
    Serial.print(httpStatusCode);
    Serial.print(": ");
    Serial.println(payload);
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

void blinkCurrentColor(int times, int duration, int gap) {
  for (int i = 1; i <= times; i++) {
    delay(gap);
    ledcWrite(Rchannel, 0);
    ledcWrite(Gchannel, 0);
    ledcWrite(Bchannel, 0);

    ledcWrite(Rchannel, r);
    ledcWrite(Gchannel, g);
    ledcWrite(Bchannel, b);

    delay(duration);
    ledcWrite(Rchannel, 0);
    ledcWrite(Gchannel, 0);
    ledcWrite(Bchannel, 0);
  }
}

void changeColor() {
  ledcWrite(Rchannel, r);
  ledcWrite(Gchannel, g);
  ledcWrite(Bchannel, b);
}

void handleCommand(String payload) {
  int requestR, requestG, requestB;
  char morseMsg[128];

  switch (payload.charAt(0)) {
    case 'n': { // quick change
        if (sscanf(payload.c_str(), "%*c,%d,%d,%d", &requestR, &requestG, &requestB) == 3) {
          updateTargetColor(requestR, requestG, requestB);
        }
        changeColor();
        break;
      }

    case 'f': { // fade
        if (sscanf(payload.c_str(), "%*c,%d,%d,%d", &requestR, &requestG, &requestB) == 3) {
          updateTargetColor(requestR, requestG, requestB);
        }
        fadeColor();
        break;
      }

    case 'r': { // rainbow
        tasteTheRainbow(5000);
        break;
      }

    case 'b': { // blink
        if (sscanf(payload.c_str(), "%*c,%d,%d,%d", &requestR, &requestG, &requestB) == 3) {
          updateTargetColor(requestR, requestG, requestB);
        }
        blinkCurrentColor(3, 100, 100);
        break;
      }

    case 'm': { // morse
        // char *fmsg = "m,..-. ..- -.-. -.- / -.-- --- ..-";
        strncpy(morseMsg, payload.c_str()+2, (sizeof morseMsg)-1);
        Serial.println(morseMsg);
        // morseHandler(morseMsg);
        break;
      }

    default: {
        if (sscanf(payload.c_str(), "%d,%d,%d", &requestR, &requestG, &requestB) == 3) {
          updateTargetColor(requestR, requestG, requestB);
        }
        fadeColor();
        break;
      }
  }
}

void updateTargetColor(int R, int G, int B) {
  if (R >= 0 && R <= PWM_MAX && G >= 0 && G <= PWM_MAX && B >= 0 && B <= PWM_MAX) {
    r = R;
    g = G;
    b = B;
  }
}

void fadeColor() {
  int currentR = ledcRead(Rchannel);
  int currentG = ledcRead(Gchannel);
  int currentB = ledcRead(Bchannel);

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
  if (current == target) {
    return;
  }

  int v = 0;
  if (current > target) {
    v = -1;
  }
  if (current < target) {
    v = 1;
  }

  ledcWrite(CHAN, current + v);
  current += v;
}

void tasteTheRainbow(int duration) {
  duration = duration / RAINBOW;
  for (int i = 0; i <= RAINBOW - 1; i++) {
    updateTargetColor(rainbow[i][0], rainbow[i][1], rainbow[i][2]); // R G B
    fadeColor();
    delay(duration);
  }
}

void checkBattery() {
  vbatt = analogRead(VBAT) * 2;
  if (vbatt <= LOW_BATT) {
    updateTargetColor(64, 0, 0);
    for (int i = 0; i <= 10; i++) {
      blinkStatusLed();
      blinkCurrentColor(1, 100, 0);
      delay(100);
    }
    WiFi.mode(WIFI_MODE_NULL);
    btStop();
    esp_deep_sleep_start();
  }
  Serial.print("battery: ");
  Serial.println(vbatt);
}
