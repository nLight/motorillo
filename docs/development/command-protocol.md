# Command Protocol Simplification

This document describes the streamlined command protocol implemented in Motorillo, reducing complexity while maintaining full functionality.

## Overview

Motorillo's command protocol has been simplified to use fewer, more consistent commands. This reduces code complexity, improves maintainability, and provides a cleaner API for position control operations.

## Protocol Evolution

### Before Simplification

**Multiple Position Commands**:

- `CMD_POS (2)`: Move to position (no speed control)
- `CMD_HOME (6)`: Move to home (no speed control)
- `CMD_POS_WITH_SPEED (15)`: Move to position with speed
- `CMD_HOME_WITH_SPEED (16)`: Move to home with speed

**Problems**:

- Redundant command codes
- Inconsistent parameter formats
- Multiple code paths for similar operations
- Maintenance complexity

### After Simplification

**Unified Position Command**:

- `CMD_POS_WITH_SPEED (15)`: All position movements with speed control

**Benefits**:

- Single command for all position operations
- Consistent binary format
- Simplified code paths
- Easier maintenance

## Unified Command Format

### CMD_POS_WITH_SPEED (15)

**Binary Format**: 6 bytes total

```
[Command Code: 15][Position: uint16][Speed: uint32]
```

**Parameters**:

- **Position**: Target position (0-65535 steps)
- **Speed**: Movement speed (1-4294967295 milliseconds per step)
- **Encoding**: Little-endian byte order for cross-platform compatibility

**Usage Examples**:

```javascript
// Move to position 1000 at normal speed
const moveBuffer = new ArrayBuffer(6);
const moveView = new DataView(moveBuffer);
moveView.setUint16(0, 1000, true); // Position = 1000
moveView.setUint32(2, 1000, true); // Speed = 1000ms per step

// Home operation (move to position 0)
const homeBuffer = new ArrayBuffer(6);
const homeView = new DataView(homeBuffer);
homeView.setUint16(0, 0, true); // Position = 0 (home)
homeView.setUint32(2, 2000, true); // Speed = 2000ms per step
```

## Implementation Changes

### JavaScript (WebUSB Interface)

**Removed Legacy Commands**:

```javascript
// OLD - Multiple command codes
this.CMD_POS = 2; // Removed
this.CMD_HOME = 6; // Removed
this.CMD_HOME_WITH_SPEED = 16; // Removed

// NEW - Single command
this.CMD_POS_WITH_SPEED = 15; // Unified position command
```

**Updated Movement Functions**:

```javascript
handleMove() {
  const position = parseInt(document.getElementById("targetPosition").value);
  const speed = parseInt(document.getElementById("manualSpeed").value);

  // Create unified position command
  const buffer = new ArrayBuffer(6);
  const view = new DataView(buffer);
  view.setUint16(0, position, true);
  view.setUint32(2, speed, true);

  this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
  this.log(`Moving to position ${position} at ${speed}ms per step`);
}

handleHome() {
  const speed = parseInt(document.getElementById("manualSpeed").value);

  // Home is just position 0 with user speed
  const buffer = new ArrayBuffer(6);
  const view = new DataView(buffer);
  view.setUint16(0, 0, true);        // Position = 0 (home)
  view.setUint32(2, speed, true);    // User-defined speed

  this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
  this.log(`Going home (position 0) at ${speed}ms per step`);
}
```

### Arduino Firmware

**Removed Command Handlers**:

```cpp
// OLD - Multiple separate handlers
case CMD_POS: {
  // Legacy position command - REMOVED
  break;
}
case CMD_HOME: {
  // Legacy home command - REMOVED
  break;
}
case CMD_HOME_WITH_SPEED: {
  // Separate home with speed - REMOVED
  break;
}
```

**Unified Command Handler**:

```cpp
case CMD_POS_WITH_SPEED: {
  // Extract position and speed from binary data
  uint16_t position = *(uint16_t *)data;
  uint32_t speedMs = *(uint32_t *)(data + 2);

  // Single function handles all position movements
  moveToPositionWithSpeed(position, speedMs);

  // Special case: position 0 is home operation
  if (position == 0) {
    displayMessage(F("Going Home"), 1000);
  } else {
    displayMessage(F("Moving"), 1000);
  }

  break;
}
```

## Conceptual Benefits

### Simplified Mental Model

**Home Operation Redefined**:

- **Old Concept**: "Home" is a special operation requiring separate commands
- **New Concept**: "Home" is simply moving to position 0

This makes the system more intuitive and reduces the number of concepts users need to understand.

### Consistent Speed Control

**All Movements Have Speed Control**:

- No legacy commands without speed parameters
- User always has control over movement speed
- Consistent behavior across all operations
- Predictable timing for all movements

### Code Maintenance

**Single Code Path**:

```cpp
// Before: Multiple movement functions
void moveToPosition(uint16_t pos);
void moveHome();
void moveToPositionWithSpeed(uint16_t pos, uint32_t speed);
void moveHomeWithSpeed(uint32_t speed);

// After: Single movement function
void moveToPositionWithSpeed(uint16_t pos, uint32_t speed);
```

**Reduced Complexity**:

- 75% fewer command handlers
- Single binary format to maintain
- Consistent parameter validation
- Unified error handling

## Migration Benefits

### For Developers

1. **Fewer Commands**: Simpler API with fewer command codes to remember
2. **Consistent Format**: All position commands use same binary structure
3. **Single Code Path**: One movement function handles all cases
4. **Better Testing**: Fewer combinations to test and validate

### For Users

1. **Consistent Behavior**: All movements respect user speed settings
2. **Intuitive Home**: Home is clearly "position 0" rather than special case
3. **Speed Control**: Never stuck with fixed-speed movements
4. **Predictable Timing**: All operations use millisecond timing

### For Maintenance

1. **Less Code**: Fewer lines of code to maintain and debug
2. **Single Format**: One binary protocol to optimize and secure
3. **Unified Logic**: Same validation and error handling for all positions
4. **Cleaner Architecture**: More logical separation of concerns

## Protocol Summary

### Current Active Commands

| Command              | Code | Purpose                  | Format                                              |
| -------------------- | ---- | ------------------------ | --------------------------------------------------- |
| `CMD_POS_WITH_SPEED` | 15   | All position movements   | `pos(2) + speed(4)`                                 |
| `CMD_CONFIG`         | 1    | System configuration     | `steps(2) + speed(4) + accel(1) + microstep(1)`     |
| `CMD_LOOP_PROGRAM`   | 9    | Simple back-and-forth    | `id(1) + name(8) + steps(2) + delay(4) + cycles(1)` |
| `CMD_PROGRAM`        | 10   | Multi-waypoint sequences | `id(1) + name(8) + count(1) + steps(var)`           |

### Removed Commands

| Command               | Code | Replaced By                       |
| --------------------- | ---- | --------------------------------- |
| `CMD_POS`             | 2    | `CMD_POS_WITH_SPEED`              |
| `CMD_HOME`            | 6    | `CMD_POS_WITH_SPEED` (position=0) |
| `CMD_HOME_WITH_SPEED` | 16   | `CMD_POS_WITH_SPEED` (position=0) |

This simplification makes Motorillo's command protocol more intuitive, maintainable, and consistent while preserving all functionality and improving user control over movement timing.
