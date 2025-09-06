# Extended Speed Range System

This document describes Motorillo's comprehensive speed control system, supporting everything from rapid positioning to ultra-slow timelapse motion with millisecond precision.

## Overview

Motorillo uses **milliseconds exclusively** for all timing operations, providing an extended speed range perfect for diverse applications from quick setup to professional timelapse work.

**Speed Range**: 1ms to 4,294,967,295ms per step (~49.7 days per step)

## Speed System Architecture

### Unified Timing

All operations use milliseconds for consistency:

- **Manual movements**: User-configurable speed via WebUSB interface
- **Program execution**: Per-step timing in loop and complex programs
- **Configuration**: Default speeds stored in EEPROM
- **Commands**: All speed parameters in milliseconds

### Binary Protocol Efficiency

All commands use optimized binary format for maximum performance:

```cpp
// 32-bit speed parameter (little-endian)
uint32_t speedMs = *(uint32_t *)(data + offset);
```

## Command Formats

### Configuration Command (CMD_CONFIG = 1)

**Binary Format**: 9 bytes total

```
[Command Code: 1][Total Steps: uint16][Speed: uint32][Acceleration: uint8][Microstepping: uint8]
```

**Example Configurations**:

```javascript
// Fast setup movement (1ms per step)
const fastConfig = [1, 2000, 1, 50, 1]; // 2000 steps/second

// Normal operation (1000ms per step)
const normalConfig = [1, 2000, 1000, 50, 1]; // 1 step/second

// Slow timelapse (30000ms per step)
const slowConfig = [1, 2000, 30000, 50, 1]; // 1 step per 30 seconds
```

### Position Command with Speed

**Binary Format**: 6 bytes total

```
[Command Code: 15][Position: uint16][Speed: uint32]
```

**JavaScript Implementation**:

```javascript
handleMove() {
  const position = parseInt(document.getElementById("targetPosition").value);
  const speed = parseInt(document.getElementById("manualSpeed").value);

  const buffer = new ArrayBuffer(6);
  const view = new DataView(buffer);
  view.setUint16(0, position, true);  // Little-endian position
  view.setUint32(2, speed, true);     // Little-endian speed

  this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
}
```

## Program Types

### Loop Programs

**Purpose**: Simple back-and-forth motion with consistent timing

**Binary Format**: 17 bytes total

```
[Command: 9][Program ID: uint8][Name: 8 chars][Steps: uint16][Delay: uint32][Cycles: uint8]
```

**Example**:

```javascript
// Ultra-slow sunrise timelapse
const sunriseProgram = {
  id: 0,
  name: "SUNRISE  ", // 8 characters, space-padded
  steps: 1000, // 1000 steps per direction
  delayMs: 120000, // 2 minutes between steps
  cycles: 1, // Single forward/back cycle
};

// Total duration: 1000 steps × 2 minutes × 2 directions = 66.7 hours
```

### Complex Programs

**Purpose**: Multi-waypoint sequences with variable speeds

**Binary Format**: Variable length

```
[Command: 10][Program ID: uint8][Name: 8 chars][Step Count: uint8]
[Position1: uint16][Speed1: uint32][Pause1: uint16]
[Position2: uint16][Speed2: uint32][Pause2: uint16]
...
```

**Example**:

```javascript
// Mixed-speed cinematic sequence
const cinematicProgram = {
  id: 1,
  name: "CINEMATIC",
  steps: [
    { pos: 0, speed: 5000, pause: 2000 }, // Slow start from home
    { pos: 800, speed: 1000, pause: 1000 }, // Quick to mid-position
    { pos: 2000, speed: 10000, pause: 5000 }, // Very slow to end
    { pos: 0, speed: 2000, pause: 0 }, // Medium return to home
  ],
};
```

## Practical Speed Ranges

### Application Categories

| Category       | Speed Range    | Steps/Second | Use Cases                                    |
| -------------- | -------------- | ------------ | -------------------------------------------- |
| **Ultra Fast** | 1-10ms         | 100-1000     | Emergency positioning, rapid testing         |
| **Fast**       | 10-100ms       | 10-100       | Quick setup, repositioning                   |
| **Normal**     | 100-2000ms     | 0.5-10       | General operation, smooth movement           |
| **Slow**       | 2-30 seconds   | 0.03-0.5     | Precise positioning, product photography     |
| **Ultra Slow** | 30s-10 minutes | 0.002-0.03   | Timelapse, astronomy, artistic effects       |
| **Extreme**    | 10+ minutes    | <0.002       | Long-term timelapse, scientific applications |

### Real-World Examples

**Product Photography Turntable**:

```javascript
// 360° rotation over 2 minutes with 72 positions (5° steps)
{
  steps: 72,
  delayMs: 1667,    // 1.667 seconds per step
  cycles: 1         // Single full rotation
}
```

**Sunset Timelapse**:

```javascript
// 2-hour sunset capture with 240 positions
{
  steps: 240,
  delayMs: 30000,   // 30 seconds per step
  cycles: 1         // Single sweep
}
```

**Macro Focus Stacking**:

```javascript
// 50 focus positions over 30 seconds
{
  steps: 50,
  delayMs: 600,     // 0.6 seconds per step
  cycles: 1         // Single focus sweep
}
```

**Astronomical Tracking**:

```javascript
// Earth rotation compensation (extremely slow)
{
  steps: 86400,     // Steps per day
  delayMs: 1000,    // 1 second per step
  cycles: 365       // Full year tracking
}
```

## Memory Optimization

### EEPROM Efficiency

- **Configuration**: 8 bytes (reduced from previous versions)
- **Loop Programs**: 16 bytes per program (compact format)
- **Complex Programs**: Variable length (step count dependent)
- **Total Storage**: Fits within Arduino Uno's 1024-byte EEPROM

### Performance Considerations

```cpp
// Long delay handling with responsiveness
void delayWithResponsiveness(uint32_t delayMs) {
  const uint32_t chunkSize = 100;  // 100ms chunks
  uint32_t remaining = delayMs;

  while (remaining > 0 && !programStopped) {
    uint32_t thisDelay = min(remaining, chunkSize);
    delay(thisDelay);
    remaining -= thisDelay;

    // Check for pause/stop commands during long delays
    checkForCommands();
  }
}
```

## WebUSB Interface

### Speed Input Control

```html
<div class="speed-control">
  <label for="manualSpeed">Speed (ms per step):</label>
  <input
    type="number"
    id="manualSpeed"
    value="1000"
    min="1"
    max="4294967295"
    title="1ms = very fast, 60000ms = 1 minute per step"
  />

  <div class="speed-presets">
    <button onclick="setSpeed(100)">Fast (100ms)</button>
    <button onclick="setSpeed(1000)">Normal (1s)</button>
    <button onclick="setSpeed(10000)">Slow (10s)</button>
    <button onclick="setSpeed(60000)">Very Slow (1min)</button>
  </div>
</div>
```

### Speed Validation

```javascript
validateSpeed(speed) {
  const minSpeed = 1;
  const maxSpeed = 4294967295;

  if (!speed || speed < minSpeed || speed > maxSpeed) {
    this.log(`Invalid speed: ${speed}. Must be between ${minSpeed} and ${maxSpeed} ms`, 'error');
    return false;
  }

  // Warn about very fast speeds
  if (speed < 50) {
    this.log(`Warning: Very fast speed (${speed}ms) - use caution`, 'warning');
  }

  // Info about very slow speeds
  if (speed > 60000) {
    const minutes = Math.round(speed / 60000);
    this.log(`Very slow speed: ${minutes} minute(s) per step`, 'info');
  }

  return true;
}
```

## Benefits

### Versatility

- **Full Range**: From emergency positioning to multi-day timelapse
- **Precision**: Millisecond-level timing control
- **Consistency**: Same units across all operations
- **Scalability**: Handles projects from seconds to months

### Performance

- **Memory Efficient**: Optimized binary protocols
- **Responsive**: Long delays broken into chunks for user interaction
- **Reliable**: Timeout and error handling for extreme durations
- **Cross-Platform**: Little-endian format ensures compatibility

### User Experience

- **Intuitive**: Speed in familiar time units (milliseconds)
- **Flexible**: Easy switching between different speed ranges
- **Persistent**: Speed preferences saved and restored
- **Safe**: Validation prevents harmful speed settings

This comprehensive speed system makes Motorillo suitable for virtually any motion control application, from rapid prototyping to professional cinematography and scientific research.
