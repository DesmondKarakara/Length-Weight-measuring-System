#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define TRIG_PIN             9
#define ECHO_PIN            10
#define HX_DOUT_PIN          3
#define HX_SCK_PIN           2
#define MODE_BUTTON_PIN      7
#define CONFIRM_BUTTON_PIN   8

HX711 scale;

const float MOUNT_HEIGHT       = 17.3;
const float CALIBRATION_FACTOR = -7050.0;
const float CONTAINER_TARE     = 10.7;

bool  mode            = false;
bool  lastModeState   = HIGH;
unsigned long lastModeMillis  = 0;

bool  lastConfState   = HIGH;
unsigned long lastConfMillis  = 0;

const unsigned long DEBOUNCE_DELAY = 200;

float dims[3]     = {0.0, 0.0, 0.0};
int   captureStep = 0;

float weight, rawDist, objHeight, volume;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CONFIRM_BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  delay(1500);

  scale.begin(HX_DOUT_PIN, HX_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();
}

void loop() {
  handleModeToggle();

  weight    = scale.get_units(5) - CONTAINER_TARE;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 50000);
  rawDist   = dur * 0.034 / 2.0;
  objHeight = MOUNT_HEIGHT - rawDist;

  if (rawDist < 0.5 || rawDist > MOUNT_HEIGHT) {
    showError("Ultrasonic Error!");
    return;
  }

  if (!mode) {
    // Weight + Height mode
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Weight: ");
    display.print(weight, 1);
    display.println(" g");
    display.print("Height: ");
    display.print(objHeight, 1);
    display.println(" cm");
    display.display();
  } else {
    // Volume Capture mode
    handleConfirmCapture();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Volume Mode:");

    if (captureStep < 3) {
      display.setCursor(0, 16);
      display.print("Confirm for D");
      display.print(captureStep + 1);
      display.display();
    } else {
      volume = dims[0] * dims[1] * dims[2];
      display.setCursor(0, 16);
      display.print("D1: ");
      display.print(dims[0], 1);
      display.println(" cm");
      display.print("D2: ");
      display.print(dims[1], 1);
      display.println(" cm");
      display.print("D3: ");
      display.print(dims[2], 1);
      display.println(" cm");
      display.print("Vol: ");
      display.print(volume, 1);
      display.println(" cm3");
      display.display();
    }
  }

  delay(50);
}

void handleModeToggle() {
  bool reading = digitalRead(MODE_BUTTON_PIN);
  if (reading != lastModeState) lastModeMillis = millis();
  if (millis() - lastModeMillis > DEBOUNCE_DELAY) {
    if (reading == LOW && lastModeState == HIGH) {
      mode = !mode;
      captureStep = 0;
      memset(dims, 0, sizeof(dims));
    }
  }
  lastModeState = reading;
}

void handleConfirmCapture() {
  bool reading = digitalRead(CONFIRM_BUTTON_PIN);
  if (reading != lastConfState) lastConfMillis = millis();
  if (millis() - lastConfMillis > DEBOUNCE_DELAY) {
    if (reading == LOW && lastConfState == HIGH) {
      if (captureStep < 3) {
        dims[captureStep] = objHeight;
        captureStep++;
      }
    }
  }
  lastConfState = reading;
}

void showError(const char* msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
  delay(2000);
}
