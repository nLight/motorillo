# Motorillo API Reference

This document provides a comprehensive reference for Motorillo's WebUSB command protocol and programming interface.

## WebUSB Command Protocol

### Overview

Motorillo uses a binary protocol for efficient communication between the browser and Arduino. All multi-byte values use little-endian encoding for cross-platform compatibility.

### Command Format

```
[Command Code: uint8][Parameters: variable length]
```

## Core Commands

### System Configuration

#### CMD_CONFIG (1)

Configure system parameters and defaults.

**Format**: 9 bytes total

```
[1][totalSteps: uint16][speed: uint32][acceleration: uint8][microstepping: uint8]
```

**Parameters**:

- `totalSteps`: Maximum travel distance (0-65535 steps)
- `speed`: Default movement speed (1-4294967295 milliseconds per step)
- `acceleration`: Acceleration profile (1-255, higher = smoother)
- `microstepping`: Driver microstepping mode (1, 2, 4, 8, 16)

**Example**:

```javascript
const config = new ArrayBuffer(9);
const view = new DataView(config);
view.setUint8(0, 1); // Command code
view.setUint16(1, 2000, true); // 2000 steps total
view.setUint32(3, 1000, true); // 1000ms default speed
view.setUint8(7, 50); // Acceleration
view.setUint8(8, 1); // Full steps
```

### Movement Commands

#### CMD_POS_WITH_SPEED (15)

Move to specific position with custom speed.

**Format**: 6 bytes total

```
[15][position: uint16][speed: uint32]
```

**Parameters**:

- `position`: Target position (0-65535 steps)
- `speed`: Movement speed (1-4294967295 milliseconds per step)

**Example**:

```javascript
// Move to position 1500 at 2000ms per step
const moveCmd = new ArrayBuffer(6);
const view = new DataView(moveCmd);
view.setUint8(0, 15); // Command code
view.setUint16(1, 1500, true); // Target position
view.setUint32(3, 2000, true); // Speed (2 seconds per step)
```

**Special Cases**:

- Position 0 = Home operation
- Use current manual speed setting for consistent behavior

### Program Management

#### CMD_LOOP_PROGRAM (9)

Create or update simple back-and-forth programs.

**Format**: 17 bytes total

```
[9][programId: uint8][name: 8 chars][steps: uint16][delayMs: uint32][cycles: uint8]
```

**Parameters**:

- `programId`: Program slot (0-4)
- `name`: Program name (8 ASCII characters, space-padded)
- `steps`: Steps per direction (1-32767)
- `delayMs`: Delay between steps (1-4294967295 milliseconds)
- `cycles`: Number of forward/backward cycles (1-255)

**Example**:

```javascript
// Create timelapse program
const program = new ArrayBuffer(17);
const view = new DataView(program);
view.setUint8(0, 9); // Command code
view.setUint8(1, 0); // Program slot 0

// Program name (8 chars, space-padded)
const name = "TIMELAPS";
for (let i = 0; i < 8; i++) {
  view.setUint8(2 + i, name.charCodeAt(i) || 32);
}

view.setUint16(10, 1000, true); // 1000 steps per direction
view.setUint32(12, 30000, true); // 30 seconds between steps
view.setUint8(16, 1); // 1 cycle (forward + back)
```

#### CMD_PROGRAM (10)

Create complex multi-waypoint programs.

**Format**: Variable length

```
[10][programId: uint8][name: 8 chars][stepCount: uint8]
[position1: uint16][speed1: uint32][pause1: uint16]
[position2: uint16][speed2: uint32][pause2: uint16]
...
```

**Parameters**:

- `programId`: Program slot (0-4)
- `name`: Program name (8 ASCII characters, space-padded)
- `stepCount`: Number of waypoints (1-20)
- For each step:
  - `position`: Target position (0-65535 steps)
  - `speed`: Movement speed (1-4294967295 milliseconds per step)
  - `pause`: Pause at position (0-65535 milliseconds)

**Example**:

```javascript
// 3-waypoint cinematic sequence
const program = new ArrayBuffer(11 + 3 * 8); // Header + 3 steps
const view = new DataView(program);
view.setUint8(0, 10); // Command code
view.setUint8(1, 1); // Program slot 1

// Program name
const name = "CINEMATIC";
for (let i = 0; i < 8; i++) {
  view.setUint8(2 + i, name.charCodeAt(i) || 32);
}

view.setUint8(10, 3); // 3 waypoints

// Waypoint 1: Slow start
let offset = 11;
view.setUint16(offset, 0, true); // Position 0
view.setUint32(offset + 2, 3000, true); // 3 seconds per step
view.setUint16(offset + 6, 2000, true); // 2 second pause

// Waypoint 2: Normal speed
offset += 8;
view.setUint16(offset, 1000, true); // Position 1000
view.setUint32(offset + 2, 1000, true); // 1 second per step
view.setUint16(offset + 6, 1000, true); // 1 second pause

// Waypoint 3: Slow end
offset += 8;
view.setUint16(offset, 2000, true); // Position 2000
view.setUint32(offset + 2, 5000, true); // 5 seconds per step
view.setUint16(offset + 6, 0, true); // No final pause
```

### Program Control

#### CMD_RUN (3)

Execute stored program by ID.

**Format**: 2 bytes total

```
[3][programId: uint8]
```

**Example**:

```javascript
// Run program in slot 0
const runCmd = new ArrayBuffer(2);
const view = new DataView(runCmd);
view.setUint8(0, 3); // Command code
view.setUint8(1, 0); // Program ID
```

#### CMD_START (4)

Start program execution.

**Format**: 1 byte

```
[4]
```

#### CMD_STOP (5)

Stop program execution.

**Format**: 1 byte

```
[5]
```

### Data Retrieval

#### CMD_GET_ALL_DATA (13)

Request all stored configuration and programs.

**Format**: 1 byte

```
[13]
```

**Response**: Text format for reliability

```
PROGRAMS:n
PROG:id,name,steps,delay,cycles
PROG:id,name,steps,delay,cycles
...
```

**Example Response**:

```
PROGRAMS:2
PROG:0,SUNSET  ,1000,30000,1
PROG:1,PRODUCT ,72,2000,1
```

#### CMD_DEBUG_INFO (14)

Connection health check (ping).

**Format**: 1 byte

```
[14]
```

**Response**: Text "PONG" for connection verification

### Utility Commands

#### CMD_SETHOME (8)

Set current position as home reference.

**Format**: 1 byte

```
[8]
```

**Behavior**: Updates position counter to 0 without motor movement

#### CMD_GET_CONFIG (7)

Request current system configuration.

**Format**: 1 byte

```
[7]
```

## JavaScript API

### SliderController Class

```javascript
class SliderController {
  constructor() {
    this.port = null;
    this.connectionHealthy = false;

    // Command codes
    this.CMD_CONFIG = 1;
    this.CMD_POS_WITH_SPEED = 15;
    this.CMD_LOOP_PROGRAM = 9;
    this.CMD_PROGRAM = 10;
    // ... other commands
  }

  async connect() {
    // Establish WebUSB connection
  }

  disconnect() {
    // Clean disconnect and cleanup
  }

  sendCommand(cmdCode, data) {
    // Send binary command to Arduino
  }

  // High-level API methods
  async moveToPosition(position, speed) {
    const buffer = new ArrayBuffer(6);
    const view = new DataView(buffer);
    view.setUint16(0, position, true);
    view.setUint32(2, speed, true);

    return this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
  }

  async createLoopProgram(id, name, steps, delay, cycles) {
    // Implementation for loop program creation
  }

  async runProgram(programId) {
    const buffer = new ArrayBuffer(2);
    const view = new DataView(buffer);
    view.setUint8(0, programId);

    return this.sendCommand(this.CMD_RUN, new Uint8Array(buffer));
  }
}
```

### Usage Examples

#### Basic Movement

```javascript
const slider = new SliderController();
await slider.connect();

// Move to position 1000 at 1500ms per step
await slider.moveToPosition(1000, 1500);

// Go home (position 0) quickly
await slider.moveToPosition(0, 500);
```

#### Program Creation

```javascript
// Create simple timelapse
await slider.createLoopProgram(
  0, // Program ID
  "SUNSET", // Name
  1200, // Steps per direction
  45000, // 45 seconds per step
  1 // 1 cycle (forward + back)
);

// Execute the program
await slider.runProgram(0);
```

## Error Handling

### Command Validation

**Client Side**:

```javascript
function validateSpeed(speed) {
  if (speed < 1 || speed > 4294967295) {
    throw new Error(`Invalid speed: ${speed}. Must be 1-4294967295ms`);
  }
}

function validatePosition(position) {
  if (position < 0 || position > 65535) {
    throw new Error(`Invalid position: ${position}. Must be 0-65535 steps`);
  }
}
```

**Arduino Side**:

- Parameter bounds checking
- EEPROM address validation
- Memory allocation verification
- Safe defaults for invalid data

### Connection Error Handling

```javascript
try {
  await slider.sendCommand(cmdCode, data);
} catch (error) {
  if (error.name === "NetworkError") {
    // Connection lost - handle gracefully
    this.handleDisconnection(error.message);
  } else {
    // Other error - log and retry
    console.error("Command failed:", error);
  }
}
```

## Performance Considerations

### Memory Usage

- **Command Buffers**: Use ArrayBuffer for binary data
- **String Handling**: Minimize string operations
- **Batch Operations**: Combine multiple commands when possible

### Timing

- **Command Delays**: Allow time between rapid commands
- **Long Operations**: Use progress callbacks for user feedback
- **Connection Health**: Implement ping/pong for connection monitoring

### Best Practices

1. **Validate Input**: Check all parameters before sending commands
2. **Handle Errors**: Implement robust error handling and recovery
3. **User Feedback**: Provide clear status and progress information
4. **Resource Cleanup**: Properly disconnect and clean up resources
5. **Performance**: Use binary protocol for efficiency

This API reference provides the complete interface for integrating with Motorillo's WebUSB protocol.
