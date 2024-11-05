# ESPHome Configuration for Wood Pellet Stove Control

This repository contains the ESPHome YAML configuration and the custom components for controlling a wood pellet stove using an ESP8266-based system. The project includes functionalities such as button pressing with servos, fan control, and pellet level monitoring.

## Features

- **Button Control with Servos**: Simulates pressing power and intensity buttons.
- **Fan Control**: Manages a cross-flow fan via a relay.
- **Pellet Level Monitoring**: Uses a Time-of-Flight (ToF) sensor.
- **Custom Status and Intensity Monitoring**.

## Setup

1. **Clone this repository** and upload the `rika-visio.yaml` to your ESP board.
2. Ensure the additional libraries (`Servo`, `Wire`, `Adafruit_VL53L0X`) are installed.
3. Configure your Wi-Fi credentials and secrets in the `!secret` section.

## Hardware Requirements

- **ESP8266 Board** (e.g., NodeMCU v2)
- **Mini Servos**
- **Relay Module**
- **Time-of-Flight (ToF) Sensor**

## Code Overview

The YAML configuration file defines a custom ESPHome setup for controlling a wood pellet stove, with integrated components for button pressing, fan control, and monitoring. Below is a detailed breakdown of the main sections:

### 1. **Custom Components**

The `custom_component` and `lambda` functions in the YAML file integrate custom C++ classes (e.g., `RVStatus`, `RVIntensity`, `RVPowerSwitch`, `RVFanSwitch`) defined in the `rika-visio.h` file. These classes manage the interaction between ESPHome and the external hardware.

```yaml
custom_component:
  - lambda: |-
      auto rv_values = new RVValuesAPI();
      return {rv_values};
```

### 2. **Text Sensors**

The configuration includes a custom `text_sensor` for monitoring the stove's status. The `RVStatus` component is registered, and its output is displayed as a text sensor in the ESPHome dashboard.

```yaml
text_sensor:
  - platform: custom
    lambda: |-
      auto rv_status = new RVStatus();
      App.register_component(rv_status);
      return {rv_status->status};
    text_sensors:
      - name: "Rika Visio Status"
```

### 3. **Numeric Sensors**

There are two `custom` sensors for reading the stove's intensity and pellet level:

- **Intensity Sensor**: Displays the stove’s operating intensity as a percentage.
- **Pellet Level Sensor**: Monitors the pellet supply and warns when the level is low.

```yaml
sensor:
  - platform: custom
    lambda: |-
      auto rv_intensity = new RVIntensity();
      App.register_component(rv_intensity);
      return {rv_intensity};
    sensors:
      name: "Rika Visio Intensity"
      unit_of_measurement: "%"
```

### 4. **Switches**

Two custom `switch` components handle power and fan control:

- **Power Switch**: Emulates the power button press using a servo.
- **Fan Switch**: Operates the cross-flow fan using a relay.

```yaml
switch:
  - platform: custom
    lambda: |-
      auto rv_power_switch = new RVPowerSwitch();
      App.register_component(rv_power_switch);
      return {rv_power_switch};
    switches:
      name: "Rika Visio Power"
```

### 5. **Wi-Fi and OTA Setup**

The configuration includes standard ESPHome modules for Wi-Fi connectivity and OTA (over-the-air) updates. The `!secret` placeholders ensure sensitive data like passwords and API keys are kept secure.

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

ota:
  password: !secret ota_pw_rika_visio
```

### 6. **AP Mode**

A fallback hotspot (`ap`) is set up to maintain accessibility if the primary Wi-Fi connection fails.

```yaml
ap:
  ssid: "Rika Visio Fallback Hotspot"
  password: !secret ap_pw_rika_visio
```

This overview provides a deeper understanding of how the custom components and sensors interact within the ESPHome framework. Feel free to modify and extend the YAML file as needed to match your hardware configuration and specific requirements.


## License

MIT License. Contributions are welcome!