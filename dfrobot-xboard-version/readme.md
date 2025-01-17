# Temperature-Controlled Cabinet System

A smart temperature control system for a network/server cabinet using Arduino. The system monitors temperature, controls fans for airflow, and manages a door actuator for additional cooling when needed. Features a web-based monitoring interface.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagram](#wiring-diagram)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [Web Interface](#web-interface)
- [API Documentation](#api-documentation)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)

## Overview

This system provides automated temperature control for a cabinet/enclosure by:
- Monitoring temperature using an SHT31d sensor
- Controlling two 4-pin PWM fans for air circulation
- Managing a linear actuator to open/close the cabinet door
- Providing real-time monitoring via a web interface
- Estimating temperature differential between top and bottom of cabinet

## Features

### Hardware Control
- Dual PWM fan control with variable speed
- Linear actuator door control
- Precise temperature monitoring
- Automatic failsafes and timeout protection

### Web Interface
- Real-time temperature monitoring
- Fan speed visualization
- Actuator status display
- System uptime tracking
- Mobile-responsive design

### Smart Control
- Temperature-based fan speed adjustment
- Automatic door control at threshold temperatures
- Non-linear fan speed curve for optimal cooling
- Separate exhaust/intake fan control
- Temperature trend analysis

## Hardware Requirements

### Main Components
- DFRobot XBoard V2 (ATmega328P with WIZ5100 Ethernet)
- SHT31d Temperature Sensor
- L9110S H-Bridge Motor Driver
- 12V Micro Linear Actuator (30mm stroke, 7mm/s)
- 2x 4-pin PWM Fans (12V DC)
- 12V Power Supply (capacity depends on actuator and fan specifications)

### Specifications
- Operating Voltage: 12V DC
- Controller: XBoard V2 (5V logic)
- Network: Ethernet 10/100 Mbps
- Temperature Range: -40°C to 125°C (SHT31d range)
- PWM Frequency: 25kHz (fan control)

## Wiring Diagram

### XBoard V2 Connections
```
Digital Pins:
- D3  → Fan Exhaust PWM (Blue wire)
- D5  → Fan Intake PWM (Blue wire)
- D6  → Actuator Extend (L9110S A-1A)
- D9  → Actuator Retract (L9110S A-1B)

I2C Connections:
- A4  → SHT31d SDA
- A5  → SHT31d SCL

Power:
- VIN → 12V Power Supply
- GND → Common Ground
```

### Fan Connections (4-pin)
```
Exhaust Fan:
- Pin 1 (Black)  → GND
- Pin 2 (Red)    → 12V
- Pin 3 (Yellow) → Not Connected (TACH)
- Pin 4 (Blue)   → D3 (PWM)

Intake Fan:
- Pin 1 (Black)  → GND
- Pin 2 (Red)    → 12V
- Pin 3 (Yellow) → Not Connected (TACH)
- Pin 4 (Blue)   → D5 (PWM)
```

## Software Requirements

### Required Arduino Libraries
- ArduinoJson (by Benoit Blanchon)
- SHT31 (by Rob Tillaart)
- Ethernet (built into Arduino IDE)

### Development Environment
- Arduino IDE 1.8.x or newer
- Board support for XBoard V2/Arduino Uno

## Installation

1. Install required libraries through Arduino Library Manager
2. Configure network settings in the code:
   ```cpp
   byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
   IPAddress ip(192, 168, 1, 177);  // Change to match your network
   ```

3. Adjust temperature thresholds if needed:
   ```cpp
   const float TEMP_OPEN_THRESHOLD = 29.0;
   const float TEMP_CLOSE_THRESHOLD = 26.5;
   const float FAN_START_TEMP = 25.0;
   const float FAN_MAX_TEMP = 30.0;
   ```

4. Upload code to XBoard V2

## Configuration

### Temperature Settings
```cpp
// Temperature thresholds (Celsius)
const float TEMP_OPEN_THRESHOLD = 29.0;  // Door opens above this temperature
const float TEMP_CLOSE_THRESHOLD = 26.5;  // Door closes below this temperature
const float FAN_START_TEMP = 25.0;       // Fans start at this temperature
const float FAN_MAX_TEMP = 30.0;         // Fans at 100% at this temperature
const float ESTIMATED_TEMP_DIFF = 2.5;   // Estimated top-bottom difference
```

### Fan Control Settings
```cpp
const uint8_t FAN_MIN_SPEED = 0;       // Minimum PWM value
const uint8_t FAN_MAX_SPEED = 255;     // Maximum PWM value
const uint8_t FAN_MIN_START = 64;      // Minimum speed when active (25%)
const float EXHAUST_INTAKE_RATIO = 1.3; // Exhaust runs 30% faster than intake
```

## Web Interface

### Accessing the Interface
1. Connect XBoard to your network
2. Navigate to configured IP address in web browser
3. Interface updates automatically every second

### Features
- Current temperature display
- Estimated top temperature
- Fan speed indicators with visual bars
- Actuator status with color coding
- System uptime display
- Error notifications

## API Documentation

### Status Endpoint
```
GET /api/status
```
Returns JSON with current system status:
```json
{
    "temperature": 27.5,
    "estimatedTopTemp": 30.0,
    "exhaustFanSpeed": 75,
    "intakeFanSpeed": 58,
    "actuatorState": "IDLE",
    "uptime": 3600000
}
```

## Troubleshooting

### Common Issues
1. **No Web Interface**
   - Check network settings
   - Verify IP address configuration
   - Confirm ethernet cable connection

2. **Temperature Reading Errors**
   - Check I2C wiring
   - Verify SHT31d address (default 0x44)

3. **Fan Not Responding**
   - Check PWM connections
   - Verify 12V power supply
   - Confirm fan type (4-pin PWM)

4. **Actuator Issues**
   - Check L9110S connections
   - Verify 12V power supply
   - Check timeout settings

## Contributing

Feel free to contribute to this project:
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Based on initial concept by [Your Name]
- Thanks to DFRobot for the XBoard V2 platform
- Contributors and testers

