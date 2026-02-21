# ESPHome Configuration for Rika Visio Pellet Stove

ESPHome YAML configuration for controlling a Rika Visio wood pellet stove with an ESP32-S3. Servo motors physically press the stove's power and intensity buttons, a relay controls a cross-flow fan, and a VL53L0X ToF sensor monitors the pellet supply level.

## Hardware

- **Waveshare ESP32-S3 Zero** (or compatible)
- **2x Mini Servos** — power button (GPIO1), intensity button (GPIO2)
- **Relay Module** — fan control (GPIO4)
- **VL53L0X ToF Sensor** — pellet level (I2C: SDA GPIO5, SCL GPIO6)
- **WS2812 LED** — on-board status LED (GPIO21)

## Setup

1. Upload `rika-visio.yaml` to your board using ESPHome.
2. Provide Wi-Fi credentials in `secrets.yaml`.
3. Calibrate servo positions and timing in the `substitutions` section.

## How It Works

### Substitutions

All tuneable values (pins, servo positions, timing, sensor calibration) are in the `substitutions` block at the top of the YAML for easy customization.

### Power Control

A template switch ("Rika Visio Power") sets the `g_power` global. An interval loop running every second detects state changes and calls `press_power_button` to physically toggle the stove. The stove takes ~21 min to start (`power_on_period`) and ~8 min to shut down (`power_off_period`). A text sensor reports the current state: On, Off, Turning On, Turning Off.

### Intensity Control

A Home Assistant number slider ("Rika Visio Intensity", 0–100%, step 5%) sets the target intensity. The interval loop compares it against `g_last_intensity` and presses the intensity servo in the appropriate direction (plus or minus), one 5% step per second, until the target is reached.

### Servo Scripts

- **`press_power_button`**: Press → 350 ms → release → 300 ms.
- **`press_intensity_button`**: Parameterized with `direction` (int). Positive = press-plus, negative = press-minus. Runs in `single` mode. Both servos auto-detach after 5 s.

### Pellet Level

The VL53L0X reads distance every 10 s. A template sensor applies a sliding window moving average (window 10), maps the distance to 0–100% using `level_full_cm` / `level_empty_cm`, and clamps the result.

### Fan

A GPIO switch ("Rika Visio Fan") directly drives the relay.

### Globals

| Variable | Type | Restored | Purpose |
|---|---|---|---|
| `g_power` | bool | yes | Current power state |
| `g_last_power` | bool | yes | Previous power state (edge detection) |
| `g_powering_up` | bool | no | Power-on transition in progress |
| `g_powering_down` | bool | no | Power-off transition in progress |
| `g_last_time_power` | int | no | Timestamp of last power transition |
| `g_last_intensity` | float | yes | Last applied intensity (0–100) |

### Boot Behaviour

On boot, both servos are detached, all global states are logged, and the status sensor is restored from flash.

### API Service

A `manual_servo_positions` service accepts `power` and `intensity` floats for manual servo testing.

## Home Assistant Entities

| Entity | Type | Description |
|---|---|---|
| Rika Visio Power | Switch | Turn stove on/off |
| Rika Visio Intensity | Number (slider) | Set intensity 0–100% |
| Rika Visio Fan | Switch | Toggle cross-flow fan |
| Rika Visio Status | Text Sensor | On / Off / Turning On / Turning Off |
| Rika Visio Level | Sensor (%) | Pellet supply level |
| Rika Visio Level Raw | Sensor | Raw VL53L0X distance |
| on board light | Light | WS2812 RGB LED |

## License

MIT