# Motorillo Troubleshooting Guide

This guide covers common issues and solutions for Motorillo setup and operation.

## Connection Issues

### WebUSB Connection Problems

**Problem**: Browser shows "No compatible devices found"

**Solutions**:

1. **Check Browser Compatibility**:

   - Use Chrome 61+ or Edge 79+ (Firefox/Safari not supported)
   - Ensure JavaScript is enabled
   - Try incognito/private mode to rule out extensions

2. **Verify Arduino Setup**:

   ```bash
   # Check if Arduino is detected by system
   # macOS/Linux:
   ls /dev/cu.usbmodem*
   ls /dev/ttyACM*

   # Windows:
   # Check Device Manager for COM ports
   ```

3. **Firmware Verification**:
   - Confirm WebUSB library is included in firmware
   - Check Serial Monitor shows "WebUSB initialized"
   - Verify Arduino board supports WebUSB (Leonardo/Micro/Zero)

**Problem**: Connection drops frequently

**Solutions**:

1. **Cable Issues**:

   - Try different USB cable (ensure data support, not power-only)
   - Test different USB ports on computer
   - Check for loose connections

2. **Power Supply**:

   - Ensure stable power supply to Arduino
   - Check if motor driver draws too much current
   - Try powered USB hub if computer USB is weak

3. **Interference**:
   - Move away from electromagnetic interference sources
   - Check for ground loops in wiring
   - Ensure proper shielding on stepper motor cables

### Serial Communication Issues

**Problem**: Commands not reaching Arduino

**Solutions**:

1. **Check Serial Settings**:

   ```cpp
   // Verify in firmware
   Serial.begin(9600);  // Baud rate must match
   ```

2. **Buffer Issues**:

   - Add delays between rapid commands
   - Check for buffer overflow in Serial communication
   - Monitor Serial.available() in firmware

3. **Binary Protocol**:
   ```javascript
   // Ensure proper binary encoding
   const view = new DataView(buffer);
   view.setUint16(0, position, true); // Little-endian required
   view.setUint32(2, speed, true);
   ```

## Hardware Issues

### Motor Problems

**Problem**: Motor not moving

**Diagnostic Steps**:

1. **Check Connections**:

   ```
   Arduino Pin 10 → Driver DIR
   Arduino Pin 16 → Driver STEP
   Arduino GND    → Driver GND
   ```

2. **Motor Driver Power**:

   - Verify 12V power to VMOT pin
   - Check 5V logic power to VDD pin
   - Ensure GND connections are solid

3. **Test Movement**:
   ```cpp
   // Simple test sketch
   digitalWrite(STEP_PIN, HIGH);
   delayMicroseconds(1000);
   digitalWrite(STEP_PIN, LOW);
   delayMicroseconds(1000);
   ```

**Problem**: Motor moves wrong direction

**Solutions**:

1. **Swap Motor Wires**:

   - Swap A+ ↔ A- OR B+ ↔ B- (not both)
   - This reverses rotation direction

2. **Software Inversion**:
   ```cpp
   // In motor_control.cpp
   digitalWrite(DIR_PIN, !targetDirection);  // Invert direction
   ```

**Problem**: Motor skips steps or vibrates

**Solutions**:

1. **Speed Adjustment**:

   - Increase delay between steps (slower speed)
   - Check mechanical load isn't too high
   - Verify acceleration settings

2. **Driver Configuration**:

   - Check microstepping pins (MS1, MS2, MS3)
   - Adjust current limiting potentiometer
   - Ensure driver heat sink is adequate

3. **Power Supply**:
   - Verify adequate current capacity (2A minimum)
   - Check voltage stability under load
   - Add capacitors if voltage drops during stepping

### Display Issues

**Problem**: OLED display blank or corrupted

**Solutions**:

1. **I2C Connection Check**:

   ```cpp
   // Test I2C scanner
   #include <Wire.h>

   void setup() {
     Wire.begin();
     Serial.begin(9600);

     for(byte addr = 1; addr < 127; addr++) {
       Wire.beginTransmission(addr);
       if(Wire.endTransmission() == 0) {
         Serial.print("Device found at 0x");
         Serial.println(addr, HEX);
       }
     }
   }
   ```

2. **Display Initialization**:

   - Try different I2C addresses (0x3C, 0x3D)
   - Check power supply voltage (3.3V or 5V)
   - Verify display resolution settings

3. **Wiring Verification**:
   ```
   Arduino SDA (A4) → OLED SDA
   Arduino SCL (A5) → OLED SCL
   Arduino VCC      → OLED VCC
   Arduino GND      → OLED GND
   ```

**Problem**: Button not responding

**Solutions**:

1. **Check Wiring**:

   ```
   Button Pin 1 → Arduino Pin 2
   Button Pin 2 → Arduino GND
   ```

2. **Test Button**:

   ```cpp
   // Simple button test
   pinMode(2, INPUT_PULLUP);
   Serial.println(digitalRead(2));  // Should toggle with button press
   ```

3. **Debouncing**:
   - Ensure proper debouncing in firmware
   - Add small capacitor across button if needed
   - Check for proper pullup configuration

## Software Issues

### Compilation Problems

**Problem**: Library not found errors

**Solutions**:

1. **Install Missing Libraries**:

   ```
   Arduino IDE → Tools → Manage Libraries
   Search and install:
   - Adafruit SSD1306
   - Adafruit GFX Library
   - WebUSB
   ```

2. **Library Path Issues**:
   - Restart Arduino IDE after library installation
   - Check Documents/Arduino/libraries folder
   - Manually install libraries if needed

**Problem**: "Board not supported" errors

**Solutions**:

1. **Board Selection**:

   ```
   Tools → Board → Arduino Leonardo (or Micro/Zero)
   ```

2. **Core Installation**:
   - Ensure Arduino AVR Boards core is installed
   - Update board definitions if needed

### Runtime Errors

**Problem**: Memory errors or crashes

**Solutions**:

1. **Memory Optimization**:

   ```cpp
   // Use F() macro for string constants
   Serial.println(F("Message"));

   // Remove debug messages in production
   #ifdef DEBUG
     Serial.print("Debug info");
   #endif
   ```

2. **EEPROM Issues**:
   ```cpp
   // Check EEPROM bounds
   if(address < EEPROM.length()) {
     EEPROM.write(address, data);
   }
   ```

**Problem**: Position tracking drift

**Solutions**:

1. **Check for Missed Steps**:

   - Reduce maximum speed
   - Increase acceleration time
   - Verify mechanical load

2. **Encoder Feedback** (if available):
   ```cpp
   // Add position verification
   if(abs(encoderPosition - calculatedPosition) > threshold) {
     // Recalibrate position
     currentPosition = encoderPosition;
   }
   ```

## Performance Issues

### Slow Response

**Problem**: Laggy web interface or motor response

**Solutions**:

1. **Optimize Communication**:

   - Reduce polling frequency
   - Use binary commands instead of text
   - Batch multiple commands

2. **Firmware Optimization**:

   ```cpp
   // Reduce delay in main loop
   delay(10);  // Instead of delay(100)

   // Use non-blocking delays for long operations
   if(millis() - lastUpdate > updateInterval) {
     updateDisplay();
     lastUpdate = millis();
   }
   ```

### Memory Constraints

**Problem**: Running out of RAM or program space

**Solutions**:

1. **String Optimization**:

   ```cpp
   // Store strings in flash memory
   const char message[] PROGMEM = "Long message text";

   // Use character arrays instead of String objects
   char buffer[20];
   strcpy(buffer, "message");
   ```

2. **Code Optimization**:
   - Remove unused functions and variables
   - Combine similar functions
   - Use more efficient data types

## Debugging Tools

### Serial Monitor Debugging

```cpp
// Enable verbose debugging
#define DEBUG 1

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// Usage
DEBUG_PRINTLN("Motor moving to position: " + String(targetPos));
```

### WebUSB Console Logging

```javascript
// Enhanced logging with timestamps
log(message, level = 'info') {
  const timestamp = new Date().toLocaleTimeString();
  const logMessage = `[${timestamp}] ${message}`;

  console.log(logMessage);

  // Optional: Display in UI log area
  const logArea = document.getElementById('logArea');
  if (logArea) {
    logArea.innerHTML += `<div class="${level}">${logMessage}</div>`;
    logArea.scrollTop = logArea.scrollHeight;
  }
}
```

### Hardware Testing

**Multimeter Checks**:

- Voltage at motor driver pins (5V, 12V, GND)
- Continuity of all connections
- Signal integrity at STEP/DIR pins

**Oscilloscope (if available)**:

- STEP pulse timing and amplitude
- DIR signal timing relative to STEP
- I2C communication signals (SDA/SCL)

## Getting Help

### Information to Gather

Before seeking help, collect:

1. **Hardware Setup**:

   - Arduino board model
   - Motor driver type
   - Stepper motor specifications
   - Power supply voltage/current

2. **Software Versions**:

   - Arduino IDE version
   - Library versions (check Library Manager)
   - Firmware compilation date

3. **Error Details**:

   - Exact error messages
   - When problem occurs (startup, during operation, etc.)
   - Steps to reproduce issue

4. **Serial Output**:
   ```bash
   # Capture serial monitor output
   # Include startup messages and error conditions
   ```

### Support Resources

- **GitHub Issues**: Report bugs with detailed reproduction steps
- **Arduino Forum**: Community help for hardware/software issues
- **Documentation**: Check other sections of this guide
- **Example Code**: Reference working examples in repository

This troubleshooting guide should resolve most common issues. For persistent problems, provide detailed information when seeking help to enable faster resolution.
