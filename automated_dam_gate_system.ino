 #define BLYNK_TEMPLATE_ID "TMPL6Win_AwVm"
#define BLYNK_TEMPLATE_NAME "Water Dam Controller"
#define BLYNK_AUTH_TOKEN "sxV7AXAJvAyPUCpFWgMqbhOzRKQHscsf"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "DHT.h"
#include "HX710B.h"
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN 27
#define SENSOR  17
#define sensorPower 32
#define sensorPin 34
#define DOUT_Pin 25
#define SCK_Pin 33
#define yellow 2
#define red 4
#define blue 16
#define DHTTYPE DHT22
#define OLED_Address 0x3C

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "kaushi";
char pass[] = "12345678";

const int DIR = 19;
const int STEP = 18;
const int  steps_per_rev = 200;
int t, h, level;
int val = 0;

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;

DHT dht(DHTPIN, DHTTYPE);
HX710B pressure_sensor;
Adafruit_SSD1306 display(128, 64);

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(sensorPower, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  pressure_sensor.begin(DOUT_Pin, SCK_Pin);
  digitalWrite(sensorPower, LOW);
  digitalWrite(yellow, LOW);
  digitalWrite(red, LOW);
  digitalWrite(blue, LOW);
  dht.begin();
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_Address);
  display.clearDisplay();
}

void loop() {
  Blynk.run();
  temRead();
  flowRead();
  waterlevel();
  pressureRead();
  displaydata();

}
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
}
BLYNK_WRITE(V2) {
  int inlet = param.asInt();
  Serial.print("inlet :");
  Serial.println(inlet);
  if (inlet == 1) {
    digitalWrite(DIR, HIGH);
    Serial.println("Spinning Clockwise...");

    for (int i = 0; i < steps_per_rev; i++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(2000);
      digitalWrite(STEP, LOW);
      delayMicroseconds(2000);
    }
    delay(1000);
  } else {
    digitalWrite(DIR, LOW);
    Serial.println("Spinning Anti-Clockwise...");

    for (int i = 0; i < steps_per_rev; i++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(1000);
      digitalWrite(STEP, LOW);
      delayMicroseconds(1000);
    }
    delay(1000);
  }
}
void temRead() {

  h = dht.readHumidity();
  t = dht.readTemperature();

  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));

}
void flowRead() {

  currentMillis = millis();

  if (currentMillis - previousMillis > interval) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);

    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;

    Serial.print("Flow rate: ");
    Serial.print(float(flowRate));
    Serial.print("L/min");
    Serial.print("\t");
    Blynk.virtualWrite(V3, float(flowRate));

    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Blynk.virtualWrite(V4, totalMilliLitres);

    Serial.print("mL / ");
    Serial.print(totalLitres);
    Serial.println("L");
  }
}
void waterlevel() {

  level = readSensor();
  Serial.print("Water level: ");
  Serial.println(level);
  Blynk.virtualWrite(V5, level);
}
int readSensor() {
  digitalWrite(sensorPower, HIGH);
  delay(10);
  val = analogRead(sensorPin);
  val = map(val, 0, 4085, 0, 100);
  digitalWrite(sensorPower, LOW);
  return val;
}
void pressureRead() {

  if (pressure_sensor.is_ready()) {
    Serial.print("Pressure (kPa): ");
    Blynk.virtualWrite(V6, pressure_sensor.pascal());
    Serial.println(pressure_sensor.pascal());
  } else {
    Serial.println("Pressure sensor not found.");
  }
  delay(1000);
  if (pressure_sensor.pascal() < 620 && pressure_sensor.pascal() > 605) {
    digitalWrite(yellow, HIGH);
  } else if (pressure_sensor.pascal() < 635 && pressure_sensor.pascal() > 620) {
    digitalWrite(yellow, HIGH);
    digitalWrite(red, HIGH);
  } else if (pressure_sensor.pascal() < 650 && pressure_sensor.pascal() > 635) {
    digitalWrite(yellow, HIGH);
    digitalWrite(red, HIGH);
    digitalWrite(blue, HIGH);
  } else {
    digitalWrite(yellow, LOW);
    digitalWrite(red, LOW);
    digitalWrite(blue, LOW);
  }

  Serial.print("ATM: ");
  Serial.println(pressure_sensor.atm());
  Serial.print("mmHg: ");
  Serial.println(pressure_sensor.mmHg());
  Serial.print("PSI: ");
  Serial.println(pressure_sensor.psi());
  Blynk.virtualWrite(V7, pressure_sensor.psi());
}
void displaydata() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2, 0);
  display.print("Water Dam Controller");
  display.setCursor(0, 10);
  display.print("Temperature : ");
  display.println(t);
  display.print("Humidity : ");
  display.println(h);
  display.print("Water Level : ");
  display.println(level);
  display.print("Pressure(kPa): "); 
  display.println(pressure_sensor.pascal());
  display.print("Output Liquid : ");
  display.println(totalMilliLitres);
  display.display();
}
