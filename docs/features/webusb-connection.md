# WebUSB Connection Management

This document describes Motorillo's robust WebUSB connection management, including automatic disconnection detection and seamless mode switching.

## Overview

Motorillo implements intelligent connection management that provides:

- **Automatic Detection**: Seamless switching between WebUSB and standalone modes
- **Disconnection Handling**: Graceful recovery when USB connection is lost
- **Dual Operation**: Continues functioning even without browser connection
- **Fast Recovery**: Quick detection and response to connection changes

## Connection Architecture

### Dual-Mode Operation

```
┌─────────────────┐    ┌──────────────────┐
│   Web Browser   │◄──►│   Arduino Board  │
│  (WebUSB Mode)  │    │   (Motorillo)    │
└─────────────────┘    └──────────────────┘
                                │
                        ┌───────┴───────┐
                        │ OLED Display   │
                        │ (Standalone)   │
                        └───────────────┘
```

**WebUSB Mode**: Full browser control with web interface
**Standalone Mode**: OLED menu navigation with physical button

## Arduino Firmware Implementation

### Connection Variables

```cpp
unsigned long lastWebUSBActivity = 0;
const unsigned long webUSBTimeoutMs = 3000; // 3 seconds timeout
bool wasInProgrammingMode = false;
bool programmingMode = false;
```

### Detection Logic

```cpp
// Update activity timestamp when data received
if (programmingMode && Serial && Serial.available()) {
  lastWebUSBActivity = millis();
  // Process WebUSB commands...
}

// Check for disconnection timeout
if (programmingMode && serialCheckComplete) {
  if (millis() - lastWebUSBActivity > webUSBTimeoutMs) {
    // Connection lost - switch to standalone mode
    programmingMode = false;
    wasInProgrammingMode = false;

    // Automatically enter menu mode
    if (!inMenuMode) {
      enterMenuMode();
    }
    updateDisplay();
  }
}
```

### Connection Establishment

```cpp
// Detect WebUSB connection during boot
if (!serialCheckComplete) {
  if (Serial && (millis() - bootTime < serialWaitTime)) {
    if (!programmingMode) {
      programmingMode = true;
      Serial.println("WebUSB Connected");
      sendAllEEPROMData(); // Send stored programs
    }
  }
}

// Handle reconnection during operation
if (serialCheckComplete && !programmingMode && Serial) {
  programmingMode = true;
  wasInProgrammingMode = true;
  lastWebUSBActivity = millis();

  // Exit menu mode when switching to programming
  if (inMenuMode) {
    exitMenuMode();
  }

  Serial.println("WebUSB Connected");
  sendAllEEPROMData();
}
```

## JavaScript Client Implementation

### Connection Health Monitoring

```javascript
class SliderController {
  constructor() {
    this.pingInterval = null;
    this.connectionHealthy = true;
  }

  async connect() {
    try {
      // Establish WebUSB connection
      this.port = await serial.requestPort();
      await this.port.connect();

      // Set up error handlers
      this.port.onReceiveError = (error) => {
        this.handleDisconnection("Receive error: " + error.message);
      };

      // Start connection health monitoring
      this.startPing();
    } catch (error) {
      this.handleDisconnection("Connection failed: " + error.message);
    }
  }

  startPing() {
    this.pingInterval = setInterval(() => {
      if (this.connectionHealthy) {
        try {
          // Send ping command to check connection
          this.sendCommand(this.CMD_DEBUG_INFO, new Uint8Array([]));
        } catch (error) {
          this.handleDisconnection("Ping failed: " + error.message);
        }
      }
    }, 2000); // Ping every 2 seconds
  }
}
```

### Disconnection Handling

```javascript
handleDisconnection(reason) {
  console.log(`WebUSB disconnected: ${reason}`);

  // Clear health monitoring
  if (this.pingInterval) {
    clearInterval(this.pingInterval);
    this.pingInterval = null;
  }

  this.connectionHealthy = false;
  this.port = null;

  // Update UI to reflect disconnected state
  document.getElementById("connectBtn").textContent = "Connect Slider";
  document.getElementById("connectBtn").disabled = false;

  // Show disconnection message
  this.log(`Connection lost: ${reason}`, 'error');
}
```

## Connection Flow

### Initial Connection

1. **Browser Request**: User clicks "Connect Slider" button
2. **Device Selection**: Browser shows available Arduino devices
3. **USB Handshake**: WebUSB protocol establishes connection
4. **Mode Switch**: Arduino detects connection and enters programming mode
5. **Data Sync**: Arduino sends all stored programs to browser
6. **Health Monitoring**: JavaScript starts ping monitoring

### Normal Operation

1. **Command Sending**: Browser sends movement/program commands
2. **Activity Tracking**: Arduino updates `lastWebUSBActivity` timestamp
3. **Response Processing**: Browser receives and processes responses
4. **Continuous Ping**: JavaScript sends periodic health checks

### Disconnection Detection

**Arduino Side (Timeout-based):**

1. No data received for 3+ seconds
2. Automatic switch to standalone mode
3. Enter OLED menu for continued operation

**JavaScript Side (Event-based):**

1. USB disconnect event detected
2. Send error during ping or command
3. Clear intervals and update UI
4. Show reconnection option

### Reconnection

1. **Manual Reconnect**: User clicks "Connect Slider" again
2. **Automatic Detection**: Arduino detects new connection
3. **Mode Switch**: Exit standalone mode, enter programming mode
4. **Data Resync**: Send all current program data to browser

## Benefits

### Reliability Features

1. **Fast Detection**: 3-second timeout ensures quick response
2. **Dual Detection**: Both Arduino and browser detect disconnections
3. **Automatic Recovery**: Arduino continues operating without browser
4. **Seamless Switching**: Smooth transitions between connection states

### User Experience

1. **No Data Loss**: Programs stored in Arduino EEPROM persist
2. **Continued Operation**: OLED interface always available
3. **Clear Status**: Connection state clearly displayed
4. **Easy Reconnection**: Single click to restore WebUSB connection

### Robustness

1. **Error Handling**: Comprehensive error detection and logging
2. **Resource Cleanup**: Proper cleanup of intervals and connections
3. **State Consistency**: Connection state kept synchronized
4. **Graceful Degradation**: Reduced functionality rather than failure

## Troubleshooting

### Connection Issues

**Symptom**: Can't establish initial connection
**Solution**:

- Ensure Arduino has WebUSB-compatible firmware
- Use Chrome/Edge browser (WebUSB support required)
- Check USB cable and port

**Symptom**: Frequent disconnections
**Solution**:

- Check USB cable quality
- Reduce ping interval if needed
- Verify power supply stability

**Symptom**: Arduino doesn't switch modes
**Solution**:

- Check serial monitor for debug messages
- Verify timeout settings in firmware
- Ensure WebUSB library is properly included

### Debug Commands

- **CMD_DEBUG_INFO (14)**: Responds with "PONG" for connection testing
- **Serial Monitor**: Shows connection status and timeout events
- **Browser Console**: Displays connection errors and ping responses

This comprehensive connection management ensures Motorillo provides a robust, user-friendly experience whether used via browser control or standalone operation.
