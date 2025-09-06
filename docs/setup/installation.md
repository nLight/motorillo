# Motorillo Installation Guide

This comprehensive guide covers everything needed to set up Motorillo, from hardware assembly to software configuration.

## Hardware Requirements

### Essential Components

| Component          | Specification            | Notes                              |
| ------------------ | ------------------------ | ---------------------------------- |
| **Arduino Board**  | Leonardo, Micro, or Zero | WebUSB compatibility required      |
| **Stepper Motor**  | NEMA 17 or similar       | 200 steps/revolution recommended   |
| **Motor Driver**   | A4988, DRV8825, TMC2208  | Compatible with STEP/DIR interface |
| **Display**        | SSD1306 OLED 96×16px     | I2C interface (0.96" typical)      |
| **Control Button** | Momentary push button    | Normally open, any current rating  |
| **Power Supply**   | 12V 2A minimum           | Depends on motor specifications    |
| **USB Cable**      | USB-A to Micro-USB       | For Arduino Leonardo/Micro         |

### Optional Components

- **Enclosure**: Protect electronics and provide mounting points
- **Heat Sinks**: For motor driver thermal management
- **Limit Switches**: End-stop detection for safety
- **LED Indicators**: Status and power indication

## Wiring Diagram

### Arduino Leonardo/Micro Connections

```
┌─────────────────┐
│  Arduino Micro  │
├─────────────────┤
│ Pin 10 ────────────→ Motor Driver DIR
│ Pin 16 ────────────→ Motor Driver STEP
│ Pin 2  ────────────→ Control Button (other leg to GND)
│ VCC    ────────────→ OLED VCC (3.3V or 5V)
│ GND    ────────────→ OLED GND + Button + Motor Driver GND
│ SDA    ────────────→ OLED SDA (Pin A4 on Leonardo)
│ SCL    ────────────→ OLED SCL (Pin A5 on Leonardo)
│ USB    ────────────→ Computer (WebUSB + Power)
└─────────────────┘
```

### Motor Driver Connections (A4988/DRV8825)

```
┌─────────────────┐     ┌──────────────┐
│  Motor Driver   │     │ Stepper Motor│
├─────────────────┤     ├──────────────┤
│ DIR   ←──────── Pin 10│              │
│ STEP  ←──────── Pin 16│              │
│ ENABLE ←─────── GND   │              │
│ MS1   ←─────── GND    │ A+  ←────────┤ 1A
│ MS2   ←─────── GND    │ A-  ←────────┤ 1B
│ MS3   ←─────── GND    │ B+  ←────────┤ 2A
│ GND   ←─────── GND    │ B-  ←────────┤ 2B
│ VDD   ←─────── 5V     │              │
│ VMOT  ←─────── 12V    └──────────────┘
└─────────────────┘
```

## Software Installation

### Arduino IDE Setup

1. **Install Arduino IDE**:

   ```bash
   # Download from https://www.arduino.cc/en/software
   # Or via package manager (macOS):
   brew install --cask arduino
   ```

2. **Install Required Libraries**:

   - Open Arduino IDE → Tools → Manage Libraries
   - Search and install:
     - `Adafruit SSD1306` (display support)
     - `Adafruit GFX Library` (graphics)
     - `WebUSB` (USB communication)

3. **Board Configuration**:
   ```
   Board: Arduino Leonardo (or Micro/Zero)
   Port: /dev/cu.usbmodem* (varies by system)
   Programmer: AVRISP mkII
   ```

### Library Dependencies

**Required Libraries** (install via Library Manager):

```cpp
#include <Adafruit_GFX.h>      // Graphics library
#include <Adafruit_SSD1306.h>  // OLED display support
#include <EEPROM.h>            // Configuration storage
#include <WebUSB.h>            // WebUSB communication
#include <Wire.h>              // I2C communication
```

**Auto-installed Dependencies**:

- Adafruit BusIO
- Adafruit GFX Library dependencies

## Firmware Installation

### Download and Upload

1. **Clone Repository**:

   ```bash
   git clone https://github.com/yourusername/motorillo.git
   cd motorillo
   ```

2. **Open Firmware**:

   - Open `motorillo.ino` in Arduino IDE
   - Verify all includes are resolved (no red underlines)

3. **Compile and Upload**:

   ```
   Arduino IDE → Sketch → Upload
   Or press Ctrl+U (Cmd+U on macOS)
   ```

4. **Verify Installation**:
   - OLED should display "Motorillo v1.0"
   - Serial monitor shows WebUSB initialization
   - Button press cycles through menu items

### Configuration Verification

**Check Pin Assignments** (in `src/motor_control.h`):

```cpp
#define STEP_PIN 16    // Verify matches your wiring
#define DIR_PIN 10     // Verify matches your wiring
#define BUTTON_PIN 2   // Button with internal pullup
```

**Check Display Settings** (in `src/display_manager.h`):

```cpp
#define SCREEN_WIDTH 128   // OLED width in pixels
#define SCREEN_HEIGHT 32   // OLED height in pixels (16 for smaller displays)
#define OLED_RESET -1      // Reset pin (-1 if not used)
```

## WebUSB Interface Setup

### Browser Requirements

**Compatible Browsers**:

- Google Chrome 61+
- Microsoft Edge 79+
- Opera 48+
- **Not supported**: Firefox, Safari

### Local Server Setup

1. **Navigate to WebUSB Directory**:

   ```bash
   cd motorillo/webusb
   ```

2. **Start Local Server** (choose one method):

   **Option A - Python 3**:

   ```bash
   python -m http.server 8000
   ```

   **Option B - Python 2**:

   ```bash
   python -m SimpleHTTPServer 8000
   ```

   **Option C - Node.js**:

   ```bash
   npx http-server -p 8000
   ```

   **Option D - PHP**:

   ```bash
   php -S localhost:8000
   ```

3. **Access Interface**:
   - Open Chrome/Edge browser
   - Navigate to `http://localhost:8000`
   - Click "Connect Slider" button
   - Select Arduino device from popup

### WebUSB Permissions

**Grant USB Access**:

1. Browser will show device selection popup
2. Select your Arduino board (should show as "Arduino Leonardo" or similar)
3. Click "Connect" to establish WebUSB connection
4. Interface should show "Connected" status

**Troubleshooting Permissions**:

- Ensure Arduino is plugged in and powered
- Try different USB ports/cables
- Check browser console for error messages
- Verify WebUSB library is properly included in firmware

## Initial Configuration

### Basic Settings

1. **Connect via WebUSB Interface**:

   - Browser shows connection status
   - Arduino display switches to "Programming Mode"

2. **Set Basic Parameters**:

   ```javascript
   // Default configuration
   Total Steps: 2000        // Adjust for your slider length
   Default Speed: 1000ms    // 1 second per step
   Acceleration: 50         // Smooth acceleration
   Microstepping: 1         // Full steps (adjust for driver)
   ```

3. **Test Movement**:
   - Use manual controls to verify direction
   - Check speed feels appropriate
   - Ensure position tracking is accurate

### Motor Calibration

**Direction Setup**:

1. Test movement in both directions
2. If backward, swap motor wires A+ ↔ A- or B+ ↔ B-
3. Alternatively, invert `DIR_PIN` logic in firmware

**Speed Calibration**:

1. Start with 1000ms per step (safe default)
2. Adjust based on mechanical constraints
3. Test various speeds: 100ms (fast) to 10000ms (slow)

**Position Calibration**:

1. Manually position slider at one end
2. Use "Set Home (Current Position)"
3. Test movement to other end
4. Adjust "Total Steps" configuration as needed

## Verification Checklist

### Hardware Tests

- [ ] OLED display shows startup message
- [ ] Button press changes display menu
- [ ] Motor responds to movement commands
- [ ] Direction is correct for intended application
- [ ] No unusual heating of driver or motor
- [ ] All connections are secure

### Software Tests

- [ ] Arduino IDE compiles without errors
- [ ] Firmware uploads successfully
- [ ] Serial monitor shows initialization messages
- [ ] WebUSB interface loads in browser
- [ ] Connection establishes successfully
- [ ] Manual controls work in both directions

### Integration Tests

- [ ] Create and run simple loop program
- [ ] Test program saving/loading
- [ ] Verify speed control works
- [ ] Check position tracking accuracy
- [ ] Test disconnection/reconnection

## Troubleshooting

### Hardware Issues

**OLED Display Not Working**:

- Check I2C connections (SDA/SCL)
- Verify power supply (3.3V or 5V)
- Try different I2C address (0x3C or 0x3D)
- Use I2C scanner sketch to detect device

**Motor Not Moving**:

- Check STEP/DIR pin connections
- Verify motor driver power supply
- Test with different speed settings
- Check motor wiring order
- Ensure ENABLE pin is pulled low

**Button Not Responding**:

- Verify button wiring (one leg to pin 2, other to GND)
- Check for proper debouncing in firmware
- Test with multimeter for continuity

### Software Issues

**Compilation Errors**:

- Install all required libraries
- Check Arduino board selection
- Verify USB cable supports data (not power-only)
- Try different USB port

**WebUSB Connection Failed**:

- Use Chrome/Edge browser only
- Ensure HTTPS or localhost origin
- Check Arduino is running WebUSB firmware
- Verify USB cable supports data communication

**Position Drift**:

- Check for missed steps (speed too fast)
- Verify power supply stability
- Ensure proper microstepping configuration
- Check for mechanical binding

### Performance Issues

**Slow Response**:

- Reduce delay values in firmware
- Optimize WebUSB communication
- Check for memory constraints
- Profile critical code sections

**Memory Errors**:

- Remove debug Serial.print() statements
- Optimize string usage
- Reduce program storage if needed
- Consider using F() macro for constants

This installation guide should get Motorillo up and running reliably. For additional help, check the troubleshooting section or consult the development documentation.
