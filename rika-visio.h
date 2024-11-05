/**
 * Rika Visio ESPHome Integration
 *
 * This is an integration of the Rika Visio pellet stove.
 *
 * This integration contains:
 * - A switch that can be used to turn the stove on and off (Class:
 * RVPowerSwitch)
 * - A switch that can be used to turn the stove fan on and off (Class:
 * RVFanSwitch)
 * - A sensor that shows the current intensity of the pellet stove (Class:
 * RVIntensity)
 * - A sensor that shows the current fillment level of the pellet stove (Class:
 * RVLevel)
 * - A text sensor that shows the current status of the pellet stove (Class:
 * RVStatus)
 *
 */

//
using namespace esphome;
#include "Adafruit_VL53L0X.h"
#include "Servo.h"
#include "esphome.h"

// Servo settings for Power Button
#define POWER_SERVO_PIN D7
#define POWER_SERVO_PRESS 25
#define POWER_SERVO_RELEASE 55
#define POWER_ON_PERIOD 1000 * 60 * 21
#define POWER_OFF_PERIOD 1000 * 60 * 8

// Servo settings for Value Buttons
#define VALUE_SERVO_PIN D6
#define VALUE_SERVO_PRESS_MINUS 110
#define VALUE_SERVO_PRESS_PLUS 50
#define VALUE_SERVO_RELEASE 76

// Fan settings
#define FAN_PIN D3

// Level Settings
#define LEVEL_EMPTY = 500;
#define LEVEL_FULL = 60;

// Logging tag
static const char *TAG = "rika-visio";

// Map function
#define mapRange(a1, a2, b1, b2, s) (b1 + (s - a1) * (b2 - b1) / (a2 - a1))

// Servo objects
Servo powerServo;
Servo valueServo;

// Lifecycle variables
bool power = false;
bool lastPower = false;
bool poweringUp = false;
bool poweringDown = false;
int lastTimePower = 0;
bool lastTimePowerPropagate = false;
bool fan = false;
bool lastFan = false;
int intensity = 0;
int lastIntensity = 0; // Unknown, so Maximum 20 in theory

// Preferences
ESPPreferenceObject savedIntensity;
ESPPreferenceObject savedPower;
ESPPreferenceObject savedFan;

// Adafruit_VL53L0X measures distance
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

class RVPowerSwitch : public Component, public Switch {
public:
  void setup() override {
    ESP_LOGD(TAG, "Setup RVPowerSwitch");

    savedPower = global_preferences->make_preference<int>(1);
    if (!savedPower.load(&power)) {
      power = false;
    }
    lastPower = power;
    publish_state(power);

    savedIntensity = global_preferences->make_preference<int>(2);
    if (!savedIntensity.load(&intensity)) {
      intensity = 0;
      lastIntensity = 20;
    } else {
      lastIntensity = intensity;
    }
  }

  void write_state(bool state) override {
    if (poweringDown || poweringUp)
      // Do not allow to change the state while powering up or down
      return;
    power = state;
    publish_state(state);
  }
};

class RVFanSwitch : public Component, public Switch {
public:
  void setup() override {
    ESP_LOGD(TAG, "Setup RVFanSwitch");
    pinMode(FAN_PIN, OUTPUT);
    savedFan = global_preferences->make_preference<int>(3);
    if (!savedFan.load(&fan)) {
      fan = false;
    }
    lastFan = fan;
    digitalWrite(FAN_PIN, fan);
    publish_state(fan);
  }
  void write_state(bool state) override {
    digitalWrite(FAN_PIN, state);
    savedFan.save(&state);
    publish_state(state);
  }
};

class RVStatus : public PollingComponent {
public:
  TextSensor *status = new TextSensor();

  // PollingComponent refreshes every 1000ms
  RVStatus() : PollingComponent(1000) {}

  void setup() override {
    ESP_LOGD(TAG, "Setup RVStatus");
    powerServo.attach(POWER_SERVO_PIN);
    powerServo.write(POWER_SERVO_RELEASE);
    powerServo.detach();
    valueServo.attach(VALUE_SERVO_PIN);
    valueServo.write(VALUE_SERVO_RELEASE);
    valueServo.detach();
    status->publish_state(power ? "On" : "Off");
    ESP_LOGD(TAG, "PollingComponent setup complete");
  }

  void update() override {
    if (power && !lastPower) {
      ESP_LOGD(TAG, "Turn on oven");
      status->publish_state("Turning On");
      pressPowerButton();
      lastTimePower = millis();
      poweringUp = true;
      lastPower = true;
      savedPower.save(&power);
      return;
    }

    if (power && lastPower && (millis() - lastTimePower) > POWER_ON_PERIOD &&
        poweringUp) {
      poweringUp = false;
      status->publish_state("On");
    }

    if (!power && lastPower && !poweringDown) {
      ESP_LOGD(TAG, "Turn oven off");
      status->publish_state("Turning Off");
      lastTimePower = millis();
      poweringDown = true;
      pressPowerButton();
      lastPower = false;
      return;
    }

    if (!power && !lastPower && poweringDown &&
        (millis() - lastTimePower) > POWER_OFF_PERIOD) {
      ESP_LOGD(TAG, "Oven turned off");
      status->publish_state("Off");
      poweringDown = false;
      savedPower.save(&power);
      return;
    }

    if (power && !poweringDown && intensity != lastIntensity) {
      ESP_LOGD(TAG, "Change Intensity");

      if (intensity > lastIntensity) {
        ESP_LOGD(TAG, "Pressing (+)");
        pressValueButton(1);
        lastIntensity++;
      }

      if (intensity < lastIntensity) {
        ESP_LOGD(TAG, "Pressing (-)");
        pressValueButton(-1);
        lastIntensity--;
      }
      savedIntensity.save(&lastIntensity);
    }
  }

  void pressPowerButton() {
    powerServo.attach(POWER_SERVO_PIN);
    pressButton(powerServo, POWER_SERVO_RELEASE, POWER_SERVO_PRESS);
    delay(300);
    powerServo.detach();
  }

  void pressValueButton(int direction) {
    valueServo.attach(VALUE_SERVO_PIN);
    int press =
        (direction > 0) ? VALUE_SERVO_PRESS_PLUS : VALUE_SERVO_PRESS_MINUS;
    pressButton(valueServo, VALUE_SERVO_RELEASE, press);
    delay(300);
    valueServo.detach();
  }

  void pressButton(Servo servo, int release, int press) {
    servo.write(press);
    delay(350);
    servo.write(release);
  }
};

class RVValuesAPI : public Component, public CustomAPIDevice {
public:
  void setup() override {
    ESP_LOGD(TAG, "Setup RVValuesAPI");

    register_service(&RVValuesAPI::setIntensity, "set_intensity",
                     {"intensity"});
    register_service(&RVValuesAPI::manualServoPos, "manual_servo_positions",
                     {"power", "value"});
  }
  void setIntensity(int percent) {
    ESP_LOGD(TAG, "Percentage set: %d", percent);
    intensity = percent / 5;
  }
  void manualServoPos(int power, int value) {
    ESP_LOGD(TAG, "Manual Position Value: %d / Power: %d", value, power);
    powerServo.write(power);
    valueServo.write(value);
  }
};

class RVIntensity : public PollingComponent, public Sensor {
public:
  RVIntensity() : PollingComponent(1000) {}
  void setup() override {
    ESP_LOGD(TAG, "Setup RVIntensity");
    publish_state(intensity * 5);
  }
  void update() override {
    if (state != (intensity * 5)) {
      publish_state(intensity * 5);
    }
  }
};

class RVLevel : public PollingComponent, public Sensor {
public:
  RVLevel() : PollingComponent(1000) {}
  bool firstStart = true;
  int smoothed = 0;
  void setup() override {
    ESP_LOGD(TAG, "Setup RVLevel");
    Serial.println("VL53L0X test");
    if (!lox.begin()) {
      ESP_LOGD(TAG, "Failed to boot VL53L0X");
    }
    lox.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
  }
  void update() override {
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure,
                    false); // pass in 'true' to get debug data printout!
    if (measure.RangeStatus != 4) { // phase failures have incorrect data
      int mm = measure.RangeMilliMeter;
      if (mm > LEVEL_EMPTY) {
        return;
      }
      if (firstStart) {
        smoothed = 10 * mm;
      } else {
        smoothed = 0.9 * smoothed + mm;
      }
      int level = max(0, min(100, mapRange(LEVEL_EMPTY, LEVEL_FULL, 0, 100,
                                           (smoothed / 10))));
      ESP_LOGD(TAG,
               "measure: %d / smoothed: %d / leveled: %d / range status %d", mm,
               smoothed, level, measure.RangeStatus);
      if (state != level && (power || firstStart)) {
        publish_state(level);
      }
      firstStart = false;
    } else {
      ESP_LOGD(TAG, " out of range ");
    }
  }
};
