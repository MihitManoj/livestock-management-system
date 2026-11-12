// =====================================================
// LIBRARIES
// =====================================================

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "HX711.h"

#include "WiFi.h"
#include "AdafruitIO_WiFi.h"

// =====================================================
// WIFI + ADAFRUIT IO
// =====================================================

#define WIFI_SSID       "<wifi ssid>"
#define WIFI_PASS       "<wifi password>"

#define IO_USERNAME     "<Adafruit IO username>"
#define IO_KEY          "<Adafruit IO key>"

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

// =====================================================
// ADAFRUIT FEEDS
// =====================================================

AdafruitIO_Feed *cowTempFeed = io.feed("cowtemperature");

AdafruitIO_Feed *barnTempFeed = io.feed("barntemperature");

AdafruitIO_Feed *activityFeed = io.feed("activity");

AdafruitIO_Feed *healthFeed = io.feed("healthstatus");

AdafruitIO_Feed *coolingFeed = io.feed("coolingstatus");

// =====================================================
// PIN DEFINITIONS
// =====================================================

// LM35
#define LM35_PIN 34

// MPU6050
#define SDA_PIN 25
#define SCL_PIN 26

// DHT11
#define DHTPIN 13
#define DHTTYPE DHT11

// Relay
#define RELAY_PIN 27

// HX711 Load Cell
#define DOUT 21
#define CLK 22

// =====================================================
// OBJECTS
// =====================================================

Adafruit_MPU6050 mpu;
HX711 scale;
DHT dht(DHTPIN, DHTTYPE);

// =====================================================
// VARIABLES
// =====================================================

String activity;
String healthStatus;
String coolingStatus;
String milkStatus;

// Change after calibration
float calibration_factor = -300.0;
String lastActivity = "";
String lastHealth = "";
String lastCooling = "";

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(2000);

  // =====================================================
  // MPU6050
  // =====================================================

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!mpu.begin()) {

    Serial.println("MPU6050 not detected!");

    while (1);
  }

  // =====================================================
  // DHT11
  // =====================================================

  dht.begin();

  // =====================================================
  // RELAY
  // =====================================================

  pinMode(RELAY_PIN, OUTPUT);

  // Relay OFF initially (ACTIVE LOW)
  digitalWrite(RELAY_PIN, LOW);

  // =====================================================
  // LOAD CELL INITIALIZATION
  // =====================================================

  scale.begin(DOUT, CLK);

  scale.set_scale(calibration_factor);

  scale.tare();

  delay(2000);

  // =====================================================
  // ADAFRUIT IO CONNECTION
  // =====================================================

  Serial.print("Connecting to Adafruit IO");

  io.connect();

  while (io.status() < AIO_CONNECTED) {

    Serial.print(".");

    delay(500);
  }

  Serial.println();
  Serial.println("Connected to Adafruit IO");

  Serial.println("====================================");
  Serial.println("SMART LIVESTOCK MONITORING STARTED");
  Serial.println("====================================");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  io.run();

  // =====================================================
  // LM35 (COW TEMPERATURE)
  // =====================================================

  int adcValue = analogRead(LM35_PIN);

  float voltage = adcValue * (3.3 / 4095.0);

  float cowTemp = voltage * 100.0 + 20;

  // =====================================================
  // DHT11 (BARN TEMPERATURE)
  // =====================================================

  float barnTemp = dht.readTemperature()+10;

  if (isnan(barnTemp)) {

    Serial.println("Failed to read DHT11!");

    delay(2000);

    return;
  }

  // =====================================================
  // MPU6050 (ACTIVITY)
  // =====================================================

  sensors_event_t a, g, temp;

  mpu.getEvent(&a, &g, &temp);

  float movement = abs(a.acceleration.x) +
                   abs(a.acceleration.y) +
                   abs(a.acceleration.z);

  if (movement < 11) {

    activity = "Resting";
  }
  else if (movement < 15) {

    activity = "Walking";
  }
  else {

    activity = "Active";
  }

  // =====================================================
  // HEALTH STATUS
  // =====================================================

  if (cowTemp > 38) {

    healthStatus = "Fever Alert";
  }
  else if (cowTemp < 34) {

    healthStatus = "Low Temperature";
  }
  else if (activity == "Resting") {

    healthStatus = "Low Activity";
  }
  else {

    healthStatus = "Normal";
  }

  // =====================================================
  // PELTIER CONTROL
  // =====================================================

  if (barnTemp > 35) {

    digitalWrite(RELAY_PIN, HIGH);

    coolingStatus = "Cooling ON";
  }
  else {

    digitalWrite(RELAY_PIN, LOW);

    coolingStatus = "Cooling OFF";
  }

  // =====================================================
  // UPDATED ADAFRUIT IO SECTION
  // =====================================================

  // Cow temperature -> every 3 seconds
  static unsigned long lastCowTempUpdate = 0;

  if (millis() - lastCowTempUpdate > 1000) {

    cowTempFeed->save(cowTemp);

    lastCowTempUpdate = millis();
  }

  // Barn temperature -> every 15 seconds
  static unsigned long lastBarnTempUpdate = 0;

  if (millis() - lastBarnTempUpdate > 15000) {

    barnTempFeed->save(barnTemp);

    lastBarnTempUpdate = millis();
  }

  // Activity -> only when changed
  if (activity != lastActivity) {

    activityFeed->save(activity);

    lastActivity = activity;
  }

  // Health -> only when changed
  if (healthStatus != lastHealth) {

    healthFeed->save(healthStatus);

    lastHealth = healthStatus;
  }

  // Cooling -> only when changed
  if (coolingStatus != lastCooling) {

    coolingFeed->save(coolingStatus);

    lastCooling = coolingStatus;
  }

  // =====================================================
  // LOAD CELL (MILK MONITORING)
  // =====================================================

  float weight = scale.get_units(20);

  // Remove tiny fluctuations
  if (abs(weight) < 5) {

    weight = 0;
  }

  // Convert grams to mL
  float milkML = weight;

  // Milk Status
  if (milkML > 300) {

    milkStatus = "Normal Milk";
  }
  else if (milkML > 150) {

    milkStatus = "Reduced Milk";
  }
  else {

    milkStatus = "Low Milk";
  }

  // =====================================================
  // SERIAL MONITOR
  // =====================================================

  Serial.println("====================================");

  Serial.print("[COW TEMP] ");
  Serial.print(cowTemp);
  Serial.println(" °C");

  Serial.print("[BARN TEMP] ");
  Serial.print(barnTemp);
  Serial.println(" °C");

  Serial.print("[ACTIVITY] ");
  Serial.println(activity);

  Serial.print("[HEALTH STATUS] ");
  Serial.println(healthStatus);

  Serial.print("[PELTIER STATUS] ");
  Serial.println(coolingStatus);

  Serial.print("[MILK QUANTITY] ");
  Serial.print(milkML);
  Serial.println(" mL");

  Serial.print("[MILK STATUS] ");
  Serial.println(milkStatus);

  Serial.println("====================================\n");

  delay(1000);
}