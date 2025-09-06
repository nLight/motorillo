# Manual Movement Velocity Control

This document describes the implementation of custom velocity control for manual movements in the WebUSB interface.

## Overview

When the configuration system was removed, the ability to set custom movement speeds for manual controls was lost. This feature restores that capability by adding velocity input to the WebUI and new commands to support custom speeds.

## WebUI Changes

### New Control Element

```html
<div class="velocity-control">
  <label for="manualSpeed">Movement Speed (milliseconds per step):</label>
  <input type="number" id="manualSpeed" value="1000" min="1" max="4294967295" />
  <span class="help"
    >Lower = faster, Higher = slower (1ms = very fast, 10000ms = very
    slow)</span
  >
</div>
```

### Features

- **Default Value**: 1000ms per step (same as `DEFAULT_SPEED_MS`)
- **Range**: 1ms to 4,294,967,295ms (full uint32_t range)
- **Persistence**: Speed setting is saved to localStorage and restored on page reload
- **Validation**: Input validation prevents invalid values

## Command Protocol Changes

### Unified Position Command

- **CMD_POS_WITH_SPEED (15)**: Move to any position with custom speed
  - Format: `position(2 bytes) + speed(4 bytes)`
  - Handles both regular moves and home operations (position 0)
  - Replaces all previous position and home commands

### Removed Commands

- **CMD_POS (2)**: Legacy position command without speed - removed
- **CMD_HOME (6)**: Legacy home command without speed - removed
- **CMD_HOME_WITH_SPEED (16)**: Separate home command - removed

### Simplified Protocol

- All manual movements now use the unified position command with custom speed
- Home operation is implemented as moving to position 0 with user-specified speed
- This simplifies the protocol while maintaining full functionality

## JavaScript Implementation

### Enhanced Functions

```javascript
handleMove() {
  const position = parseInt(document.getElementById("targetPosition").value);
  const speed = parseInt(document.getElementById("manualSpeed").value);

  // Validation + binary protocol
  this.sendCommand(this.CMD_POS_WITH_SPEED, buffer);
}

handleHome() {
  const speed = parseInt(document.getElementById("manualSpeed").value);
  const position = 0; // Home is position 0

  // Uses same position command with position 0
  this.sendCommand(this.CMD_POS_WITH_SPEED, buffer);
}
```

### Input Validation

- Checks for valid numeric input
- Ensures speed is within 1ms to 4,294,967,295ms range
- Provides user feedback for invalid inputs
- Logs movement details to console

### Persistence

- `saveManualSpeed()`: Saves speed to localStorage on change
- `loadManualSpeed()`: Restores speed on page load
- Storage key: `"sliderManualSpeed"`

## Arduino Firmware Changes

### Unified Command Handler

```cpp
case CMD_POS_WITH_SPEED: {
  uint16_t position = *(uint16_t *)data;
  uint32_t speedMs = *(uint32_t *)(data + 2);
  moveToPositionWithSpeed(position, speedMs);
  // Handles both regular moves and home (when position = 0)
  break;
}
```

### Data Format

- Uses little-endian byte order for multi-byte values
- Position: 16-bit unsigned integer (0-65535)
- Speed: 32-bit unsigned integer (1-4294967295 milliseconds)

## Usage Examples

### Fast Movement

- Speed: `100ms` per step
- Use case: Quick positioning, rapid testing

### Normal Movement

- Speed: `1000ms` per step (default)
- Use case: General operation, smooth movement

### Slow Movement

- Speed: `5000ms` per step
- Use case: Precise positioning, timelapse applications

### Very Slow Movement

- Speed: `60000ms` per step (1 minute per step)
- Use case: Ultra-slow timelapse, precise control

## Benefits

1. **Flexibility**: Users can adjust movement speed for different applications
2. **Precision**: Fine control over movement timing
3. **Persistence**: Settings are remembered between sessions
4. **Validation**: Prevents invalid inputs and provides feedback
5. **Compatibility**: Maintains backward compatibility with existing code
6. **Consistency**: Uses the same speed units (milliseconds) as program loops

## Speed Guidelines

- **1-100ms**: Very fast (use with caution for mechanical safety)
- **100-1000ms**: Fast (good for quick positioning)
- **1000-5000ms**: Normal (smooth, visible movement)
- **5000-30000ms**: Slow (precise control, timelapse setup)
- **30000ms+**: Very slow (ultra-precision, long timelapse)
