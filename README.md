# ESPHome Configuration for Wood Pellet Stove Control

This repository contains the ESPHome YAML configuration for controlling a Rika Visio wood pellet stove using an ESP32-S3-based system. The project uses servo motors to physically press stove buttons, a relay to control a cross-flow fan, and a VL53L0X ToF sensor to monitor pellet supply level.

## Features

- **Button Control with Servos**: Simulates pressing power and intensity buttons using two servo motors with configurable press/release positions.
- **Fan Control**: Manages a cross-flow fan via a GPIO relay.
- **Pellet Level Monitoring**: Uses a VL53L0X Time-of-Flight (ToF) sensor over I2C to measure pellet supply, mapped to 0–100%.
- **Intensity Control**: Adjustable intensity (0–100%, step 5%) via a Home Assistant number slider, with directional servo presses (plus/minus).
- **Status Monitoring**: Real-time status updates (On, Off, Turning On, Turning Off) via a text sensor.
- **On-Board LED**: WS2812 RGB LED on GPIO21 for status indication.
- **Boot Recovery**: Restores power, intensity, and last-power state from flash on reboot; servos detach on boot.

## Setup

1. **Clone this repository** and upload `rika-visio.yaml` to your ESP32-S3 board using ESPHome.
2. Configure your Wi-Fi credentials in a `secrets.yaml` file.
3. Adjust hardware pin assignments, servo positions, and timing parameters in the `substitutions` section as needed.

## Hardware Requirements

- **Waveshare ESP32-S3 Zero** (or compatible ESP32-S3 board)
- **2x Mini Servos** (for power and intensity buttons)
- **Relay Module** (for fan control, on GPIO4)
- **VL53L0X Time-of-Flight Sensor** (I2C, SDA on GPIO5, SCL on GPIO6)

## Configuration Overview

The YAML configuration uses the ESP-IDF framework with a clean, section-based structure. All tuneable values are in `substitutions` for easy customization.

### 1. **Substitutions**

All configurable values are defined in the `substitutions` section:

```yaml
substitutions:
  # Hardware Pins (ESP32-S3)
  power_servo_pin: GPIO1
  intensity_servo_pin: GPIO2
  fan_pin: GPIO4
  i2c_sda_pin: GPIO5
  i2c_scl_pin: GPIO6

  # Servo Positions (-1.0 to 1.0)
  power_servo_press: 0.25
  power_servo_release: 0.4
  intensity_servo_press_plus: -0.25
  intensity_servo_release: -0.08
  intensity_servo_press_minus: 0.08

  # Timing (seconds)
  power_on_period: 1260   # 21 min
  power_off_period: 480   # 8 min

  # Pellet Level Sensor (VL53L0X)
  level_full_cm: 6.0
  level_empty_cm: 50.0
```

### 2. **Number Component for Intensity Control**

Intensity is controlled via a `number` component exposed as a slider in Home Assistant (0–100%, step 5%):

```yaml
number:
  - platform: template
    name: "Rika Visio Intensity"
    unit_of_measurement: "%"
    mode: SLIDER
    min_value: 0
    max_value: 100
    step: 5
    optimistic: true
    restore_value: yes
```

The interval logic compares this value against `g_last_intensity` and presses the intensity servo in the appropriate direction (plus or minus), one 5% step at a time.

### 3. **Global Variables**

State management using ESPHome globals with flash restore where needed:

| Variable | Type | Restored | Purpose |
|---|---|---|---|
| `g_power` | bool | yes | Current power state |
| `g_last_power` | bool | yes | Previous power state (edge detection) |
| `g_powering_up` | bool | no | Power-on transition in progress |
| `g_powering_down` | bool | no | Power-off transition in progress |
| `g_last_time_power` | int | no | Timestamp of last power transition |
| `g_last_intensity` | float | yes | Last applied intensity (0–100) |

### 4. **Servo Control**

Two servos simulate button presses via LEDC PWM outputs at 50 Hz:

- **Power Servo** (`GPIO1`): Single press/release position pair.
- **Intensity Servo** (`GPIO2`): Three positions — press-plus, press-minus, and release.

Both servos use `auto_detach_time: 5s` to save power and reduce noise after actuation.

### 5. **Scripts**

Two reusable scripts encapsulate servo button presses:

- **`press_power_button`**: Writes press position, waits 350 ms, writes release, waits 300 ms.
- **`press_intensity_button`**: Parameterized with `direction` (int). Direction > 0 uses `intensity_servo_press_plus`, otherwise `intensity_servo_press_minus`. Same timing as power. Runs in `single` mode to prevent overlapping presses.

### 6. **Automation Logic**

An `interval` component runs every second and handles:

1. **Power on**: Detects `g_power && !g_last_power`, calls `press_power_button`, starts the power-on timer.
2. **Power on complete**: After `power_on_period` seconds, clears `g_powering_up` and sets status to "On".
3. **Power off**: Detects `!g_power && g_last_power`, calls `press_power_button`, starts the power-off timer.
4. **Power off complete**: After `power_off_period` seconds, clears `g_powering_down` and sets status to "Off".
5. **Intensity adjustment**: When powered on and not transitioning, compares `rika_intensity_number.state` to `g_last_intensity` and presses the intensity button in the appropriate direction, incrementing/decrementing `g_last_intensity` by 5 each step.

### 7. **Sensors**

- **Status Sensor** (`text_sensor`): Shows current state — "On", "Off", "Turning On", "Turning Off".
- **Pellet Level Raw** (`vl53l0x`): Raw distance reading, updated every 10 s.
- **Pellet Level** (`template`): Reads from the raw sensor, applies a sliding window moving average (window 10), maps the distance to 0–100% using `level_full_cm` and `level_empty_cm`, and clamps the result.

```yaml
sensor:
  - platform: vl53l0x
    name: "Rika Visio Level Raw"
    update_interval: 10s

  - platform: template
    name: "Rika Visio Level"
    unit_of_measurement: "%"
    icon: "mdi:silo"
    update_interval: 10s
    lambda: return id(rika_level_raw).state;
    filters:
      - sliding_window_moving_average:
          window_size: 10
          send_every: 1
      - lambda: |-
          float level_empty = ${level_empty_cm} / 100.0;
          float level_full = ${level_full_cm} / 100.0;
          float mapped_value = 100.0f * (x - level_empty) / (level_full - level_empty);
          return std::min(100.0f, std::max(0.0f, mapped_value));
```

### 8. **Switches**

- **Power Switch** (`template`): Controls `g_power` global; restores default off; reports state from `g_power`.
- **Fan Switch** (`gpio`): Directly drives the relay on `GPIO4`; restores default off.

### 9. **Light**

An on-board WS2812 RGB LED on `GPIO21` (via ESP32 RMT) is available for status indication:

```yaml
light:
  - platform: esp32_rmt_led_strip
    pin: GPIO21
    num_leds: 1
    chipset: ws2812
    name: "on board light"
```

### 10. **Wi-Fi and OTA**

- Static IP: `10.1.1.112`
- Fast connect enabled, power save disabled, output power 18 dBm
- OTA updates with password protection
- Fallback AP mode ("Rika Visio Fallback Hotspot")

### 11. **API**

Encrypted API connection with a custom service for manual servo testing:

```yaml
api:
  encryption:
    key: "..."
  services:
    - service: manual_servo_positions
      variables:
        power: float
        intensity: float
```

### 12. **Boot Behaviour**

On boot (priority -100):
1. Both servos are detached.
2. All global states are logged.
3. Status sensor is set to "On" or "Off" based on restored `g_power`.

## Usage

### In Home Assistant

1. **Power Control**: Use the "Rika Visio Power" switch to turn the stove on/off.
2. **Intensity Adjustment**: Use the "Rika Visio Intensity" number slider (0–100%, step 5%).
3. **Fan Control**: Toggle the "Rika Visio Fan" switch.
4. **Monitoring**: View status text, pellet level percentage, and on-board LED.

### Timing

- **Power On**: Takes ~21 minutes (configurable via `power_on_period`)
- **Power Off**: Takes ~8 minutes (configurable via `power_off_period`)
- **Intensity Changes**: Applied incrementally, one 5% step at a time per second

## Customization

All hardware-specific values can be adjusted in the `substitutions` section:
- Pin assignments (power servo, intensity servo, fan, I2C SDA/SCL)
- Servo positions — press/release values for power; press-plus, press-minus, and release for intensity
- Timing periods for power on/off transitions
- Pellet level sensor calibration (full/empty distance in cm)

## License

MIT License. Contributions are welcome!