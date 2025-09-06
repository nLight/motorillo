# WebUSB Disconnection Detection

This document describes the implementation of WebUSB disconnection detection to automatically disable programming mode when the connection is lost.

## Overview

The system implements a dual-approach disconnection detection:

1. **Arduino Firmware Side**: Timeout-based detection
2. **JavaScript Client Side**: Event-based and ping-based detection

## Arduino Firmware Implementation

### Variables

```cpp
unsigned long lastWebUSBActivity = 0;
const unsigned long webUSBTimeoutMs = 3000; // 3 seconds timeout
bool wasInProgrammingMode = false;
```

### Detection Logic

- **Activity Tracking**: `lastWebUSBActivity` is updated every time data is received from WebUSB
- **Timeout Detection**: If no activity is detected for 3 seconds, the system automatically:
  - Switches from programming mode to standalone mode
  - Enters menu mode for user interaction
  - Updates the display to reflect the new mode
  - Outputs debug message: `"[DEBUG] WebUSB timeout - switched to standalone mode"`

### Files Modified

- `steppper.ino`: Main loop includes disconnection detection logic
- `src/command_processor.cpp`: Added `CMD_DEBUG_INFO` handler for ping responses

## JavaScript Client Implementation

### Event Handlers

```javascript
this.port.onReceiveError = (error) => {
  this.handleDisconnection("Receive error: " + error.message);
};

this.port.onDisconnect = () => {
  this.handleDisconnection("Device disconnected");
};
```

### Connection Health Check

- **Periodic Ping**: Sends `CMD_DEBUG_INFO` command every 2 seconds to check connection
- **Error Detection**: Any send/receive errors trigger disconnection handling
- **Automatic Cleanup**: Clears intervals and updates UI when disconnection is detected

### Enhanced Error Handling

- All `sendCommand()` calls now detect send failures and trigger disconnection handling
- Connection status is immediately updated in the UI
- Console logs provide clear disconnection reasons

## Benefits

1. **Fast Detection**: 3-second timeout on Arduino side, 2-second ping on JavaScript side
2. **Automatic Recovery**: Arduino automatically enters menu mode for continued operation
3. **User Feedback**: Clear console messages and UI updates inform the user
4. **Robust Handling**: Multiple detection methods ensure reliable disconnection detection
5. **Resource Cleanup**: Proper cleanup of intervals and connection resources

## Usage

The disconnection detection works automatically:

1. **Normal Operation**: User connects via WebUSB, Arduino enters programming mode
2. **Disconnection Event**: USB cable unplugged, browser tab closed, or connection error
3. **Detection**: Both Arduino (timeout) and JavaScript (events/ping) detect the disconnection
4. **Recovery**: Arduino switches to standalone mode, JavaScript updates UI to disconnected state
5. **Continuation**: User can continue using the device via physical controls and display

## Technical Details

### Arduino Side

- Timeout reduced from 5 seconds to 3 seconds for faster response
- Debug output helps with troubleshooting
- Seamless transition between programming and standalone modes

### JavaScript Side

- Uses WebUSB's native disconnection events when available
- Implements periodic health checks for additional reliability
- Graceful error handling prevents JavaScript exceptions

### Commands

- `CMD_DEBUG_INFO` (14): Simple ping command that responds with "PONG"
- Used by JavaScript for connection health monitoring
- Lightweight and doesn't interfere with normal operation

This implementation ensures that users don't get stuck in programming mode when the WebUSB connection is lost, providing a seamless experience whether using the device via web interface or standalone operation.
