# Manual Movement Velocity Control

This document describes the implementation of custom velocity control for manual movements in the Motorillo WebUSB interface.

## Overview

The Motorillo system provides flexible velocity control for manual movements through the web interface. Users can set custom movement speeds ranging from ultra-fast to extremely slow motion, perfect for various applications from quick positioning to precise timelapse work.

## WebUI Interface

### Velocity Control Element

```html
<div class="velocity-control">
  <label for="manualSpeed">Movement Speed (milliseconds per step):</label>
  <input type="number" id="manualSpeed" value="1000" min="1" max="4294967295" />
  <span class="help">
    Lower = faster, Higher = slower (1ms = very fast, 10000ms = very slow)
  </span>
</div>
```

### Features

- **Default Value**: 1000ms per step (balanced speed)
- **Range**: 1ms to 4,294,967,295ms (full 32-bit range)
- **Persistence**: Speed setting saved to localStorage and restored on page reload
- **Validation**: Input validation prevents invalid values with user feedback

## Command Protocol

### Unified Position Command

- **CMD_POS_WITH_SPEED (15)**: Move to any position with custom speed
  - Format: `position(2 bytes) + speed(4 bytes)`
  - Handles both regular moves and home operations (position 0)
  - Uses little-endian byte order for multi-byte values

### Simplified Protocol Benefits

- All manual movements use the unified position command with custom speed
- Home operation implemented as moving to position 0 with user-specified speed
- Simplified protocol while maintaining full functionality
- Consistent speed units (milliseconds) across all operations

## JavaScript Implementation

### Enhanced Movement Functions

```javascript
handleMove() {
  const position = parseInt(document.getElementById("targetPosition").value);
  const speed = parseInt(document.getElementById("manualSpeed").value);

  // Create binary command buffer
  const buffer = new ArrayBuffer(6);
  const view = new DataView(buffer);
  view.setUint16(0, position, true);  // Position (little-endian)
  view.setUint32(2, speed, true);     // Speed (little-endian)

  this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
}

handleHome() {
  const speed = parseInt(document.getElementById("manualSpeed").value);
  const position = 0; // Home is position 0

  // Uses same position command with position 0
  const buffer = new ArrayBuffer(6);
  const view = new DataView(buffer);
  view.setUint16(0, position, true);
  view.setUint32(2, speed, true);

  this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
}
```

### Input Validation & Persistence

- **Validation**: Checks for valid numeric input within acceptable range
- **Feedback**: Provides user feedback for invalid inputs with console logging
- **Storage**: `saveManualSpeed()` saves to localStorage on change
- **Restoration**: `loadManualSpeed()` restores saved speed on page load

## Arduino Firmware

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

### Data Processing

- Processes little-endian byte order for cross-platform compatibility
- Position: 16-bit unsigned integer (0-65535 steps)
- Speed: 32-bit unsigned integer (1-4294967295 milliseconds per step)

## Usage Examples

### Application-Specific Speeds

| Use Case             | Speed Range   | Example  | Description                 |
| -------------------- | ------------- | -------- | --------------------------- |
| **Quick Testing**    | 10-100ms      | 50ms     | Rapid positioning for setup |
| **Normal Operation** | 500-2000ms    | 1000ms   | Smooth, visible movement    |
| **Precise Control**  | 2000-10000ms  | 5000ms   | Careful positioning         |
| **Timelapse Setup**  | 10000-60000ms | 30000ms  | Ultra-precise movement      |
| **Long Exposure**    | 60000ms+      | 300000ms | Extremely slow motion       |

### Practical Examples

```javascript
// Fast positioning for quick adjustments
speed: 100ms  // 10 steps per second

// Standard operation for general use
speed: 1000ms // 1 step per second (default)

// Slow precision for critical positioning
speed: 5000ms // 1 step per 5 seconds

// Timelapse preparation
speed: 30000ms // 1 step per 30 seconds

// Ultra-slow for long exposure work
speed: 600000ms // 1 step per 10 minutes
```

## Benefits

1. **Flexibility**: Adaptable speed for different applications and requirements
2. **Precision**: Fine-grained control over movement timing
3. **Persistence**: User preferences remembered between sessions
4. **Safety**: Input validation prevents mechanical damage from excessive speeds
5. **Consistency**: Same millisecond units used throughout the system
6. **Simplicity**: Unified command protocol reduces complexity

## Speed Guidelines

### Safety Recommendations

- **1-50ms**: Use with caution - very fast movement may cause mechanical stress
- **50-500ms**: Fast but safe for most applications
- **500-2000ms**: Optimal range for general use
- **2000-30000ms**: Precision range for careful work
- **30000ms+**: Specialized applications (timelapse, astronomy)

### Application Guidelines

- **Product Photography**: 1000-5000ms for smooth rotation
- **Timelapse Setup**: 5000-30000ms for precise framing
- **Video Production**: 500-2000ms for smooth camera movements
- **Scientific Applications**: 10000ms+ for precise positioning
- **Art/Long Exposure**: 60000ms+ for creative effects

This flexible velocity control system makes Motorillo suitable for a wide range of applications, from rapid prototyping to professional cinematography and scientific research.
