# ESPHome Configuration for Wood Pellet Stove Control

This repository contains the ESPHome YAML configuration for controlling a wood pellet stove using an ESP32-based system. The project includes functionalities such as button pressing with servos, fan control, and pellet level monitoring.

## Features

- **Button Control with Servos**: Simulates pressing power and intensity buttons using servo motors.
- **Fan Control**: Manages a cross-flow fan via a relay.
- **Pellet Level Monitoring**: Uses a VL53L0X Time-of-Flight (ToF) sensor to monitor pellet supply.
- **Intensity Control**: Adjustable intensity levels (0-100%) via Home Assistant number component.
- **Status Monitoring**: Real-time status updates (On, Off, Turning On, Turning Off).

## Setup

1. **Clone this repository** and upload the `rika-visio.yaml` to your ESP32 board using ESPHome.
2. Configure your Wi-Fi credentials in a `secrets.yaml` file.
3. Adjust hardware pin assignments and timing parameters in the `substitutions` section if needed.

## Hardware Requirements

- **ESP32 Board** (e.g. Waveshare ESP32-S3 Zero)
- **2x Mini Servos** (for power and intensity buttons)
- **Relay Module** (for fan control)
- **VL53L0X Time-of-Flight Sensor** (for pellet level monitoring)

## Configuration Overview

The YAML configuration file uses modern ESPHome features with a clean, maintainable structure. Below is a detailed breakdown of the main sections:

### 1. **Substitutions**

All configurable values are defined in the `substitutions` section for easy customization:

```yaml
substitutions:
  # Hardware Pins
  power_servo_pin: GPIO1
  intensity_servo_pin: GPIO2
  fan_pin: GPIO4

  # Servo Positions
  power_servo_press: 0.25
  power_servo_release: 0.4

  # Timing
  power_on_period: 1260   # 21 minutes
  power_off_period: 480   # 8 minutes

  # Pellet Level Calibration
  level_full_cm: 6.0
  level_empty_cm: 50.0
```

### 2. **Number Component for Intensity Control**

The intensity is controlled via a `number` component in Home Assistant, providing a slider interface (0-100%, step 5):

```yaml
number:
  - platform: template
    name: "Rika Visio Intensity Level"
    min_value: 0
    max_value: 100
    step: 5
```

The component automatically converts percentage values to internal servo levels (1-20).

### 3. **Global Variables**

State management using ESPHome globals:

- **g_power**: Current power state (on/off)
- **g_intensity**: Current intensity level (0-20)
- **g_powering_up/down**: Transition states
- **g_last_time_power**: Timing for power transitions

### 4. **Servo Control**

Two servos simulate button presses:

- **Power Servo**: Toggles the stove on/off
- **Intensity Servo**: Adjusts intensity up (+) or down (-)

Servos automatically detach after 5 seconds to save power and reduce noise.

### 5. **Automation Logic**

An `interval` component runs every second to:
- Handle power on/off transitions with configurable delays
- Adjust intensity by pressing buttons incrementally
- Update status text sensor

### 6. **Sensors**

- **Status Sensor**: Text sensor showing current state (On, Off, Turning On, Turning Off)
- **Intensity Sensor**: Template sensor displaying current intensity as percentage
- **Pellet Level Sensor**: VL53L0X distance sensor with mapping to 0-100% and moving average filter

```yaml
sensor:
  - platform: template
    name: "Rika Visio Intensity"
    unit_of_measurement: "%"
    lambda: return id(g_intensity) * 5.0;

  - platform: vl53l0x
    name: "Rika Visio Level Raw"

  - platform: template
    name: "Rika Visio Level"
    filters:
      - sliding_window_moving_average:
          window_size: 10
```

### 7. **Switches**

- **Power Switch**: Template switch to turn the stove on/off
- **Fan Switch**: GPIO switch to control the relay for the fan

### 8. **Wi-Fi and OTA**

Standard ESPHome configuration with:
- Static IP address (10.1.1.112)
- Fast connect and optimized power settings
- OTA updates with password protection
- Fallback AP mode

### 9. **API Services**

Custom service for manual servo position testing:

```yaml
api:
  services:
    - service: manual_servo_positions
      variables:
        power: float
        intensity: float
```

## Usage

### In Home Assistant

1. **Power Control**: Use the "Rika Visio Power" switch to turn the stove on/off
2. **Intensity Adjustment**: Use the "Rika Visio Intensity Level" number slider (0-100%, step 5%)
3. **Fan Control**: Toggle the "Rika Visio Fan" switch
4. **Monitoring**: View status, current intensity, and pellet level

### Timing

- **Power On**: Takes ~21 minutes (configurable via `power_on_period`)
- **Power Off**: Takes ~8 minutes (configurable via `power_off_period`)
- **Intensity Changes**: Applied incrementally, one level at a time

## Customization

All hardware-specific values can be adjusted in the `substitutions` section:
- Pin assignments
- Servo positions (calibrate for your specific servos and button positions)
- Timing periods
- Pellet level sensor calibration values

## License

MIT License. Contributions are welcome!