# Home Positioning System

This document describes Motorillo's flexible home positioning system, providing both physical homing and reference point setting capabilities.

## Overview

Motorillo provides two complementary approaches to home positioning:

- **Physical Homing**: Move the motor to position 0 with controllable speed
- **Reference Setting**: Set current position as home without movement

This dual approach accommodates different workflows and use cases, from automatic calibration to manual setup scenarios.

## Home Positioning Options

### Set Home (Current Position)

**Purpose**: Establish a new reference point without motor movement

**Usage**:

- After manually positioning the slider to desired starting location
- When recalibrating after power cycling
- Setting custom reference points for specific projects

**Command**: `SETHOME` (standalone) or via green "Set Home" button (WebUSB)

**Behavior**:

- Updates software position counter to zero
- No physical motor movement
- Immediate operation
- Displays "Set Home" confirmation on OLED

### Go to Home (Position 0)

**Purpose**: Physically move motor to the zero position with controlled speed

**Usage**:

- Returning to known reference position
- Automatic calibration sequences
- Preparing for program execution

**Command**: `HOME` or `CMD_POS_WITH_SPEED` with position=0 (WebUSB)

**Behavior**:

- Physical movement to position 0
- Uses current manual speed setting (WebUSB mode)
- Respects speed and acceleration settings
- Shows movement progress on display

## WebUSB Interface

### Visual Interface Design

```html
<!-- Green button for reference setting -->
<button id="setHomeBtn" class="btn btn-success">
  Set Home (Current Position)
</button>

<!-- Orange button for physical movement -->
<button id="goHomeBtn" class="btn btn-warning">Go to Home (Position 0)</button>
```

### Implementation

```javascript
handleSetHome() {
  // Set current position as home without movement
  this.sendCommand(this.CMD_SETHOME, new Uint8Array([]));
  this.log("Setting current position as home (no movement)");
}

handleGoHome() {
  // Move to position 0 with current speed setting
  const speed = parseInt(document.getElementById("manualSpeed").value);
  const buffer = new ArrayBuffer(6);
  const view = new DataView(buffer);
  view.setUint16(0, 0, true);      // Position = 0 (home)
  view.setUint32(2, speed, true);  // User-defined speed

  this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
  this.log(`Moving to home position (0) at ${speed}ms per step`);
}
```

## Arduino Implementation

### Set Home Command

```cpp
case CMD_SETHOME: {
  currentPosition = 0;  // Reset position counter
  displayMessage(F("Set Home"), 2000);
  break;
}
```

### Physical Home Movement

```cpp
case CMD_POS_WITH_SPEED: {
  uint16_t targetPosition = *(uint16_t *)data;
  uint32_t speedMs = *(uint32_t *)(data + 2);

  if (targetPosition == 0) {
    // This is a home operation - move to position 0
    moveToPositionWithSpeed(0, speedMs);
  } else {
    // Regular position movement
    moveToPositionWithSpeed(targetPosition, speedMs);
  }
  break;
}
```

## Use Cases

### Manual Setup Workflow

1. **Physical Positioning**: Manually move slider to desired starting position
2. **Set Reference**: Use "Set Home (Current Position)" to establish reference
3. **Begin Work**: Start timelapse or motion sequence from this position

### Automatic Calibration

1. **Power On**: Arduino boots with unknown position
2. **Go to Home**: Use "Go to Home (Position 0)" to move to known reference
3. **Ready State**: Now at calibrated zero position for accurate movements

### Project-Specific References

1. **Scene Setup**: Position camera/subject for specific shot
2. **Mark Position**: Set current position as home for this project
3. **Motion Programming**: Create programs relative to this reference point

### Recovery Scenarios

1. **Power Loss**: After power cycling, position may be unknown
2. **Manual Movement**: If slider was manually moved during transport
3. **Reference Reset**: Quick way to establish new reference without complex calibration

## Benefits

### Flexibility

- **No Movement Required**: Set home instantly without waiting for motor movement
- **Speed Control**: Physical homing respects user speed preferences
- **Multiple References**: Can set different home positions for different projects
- **Fast Setup**: Quick reference establishment for rapid workflow

### Safety

- **Position Awareness**: Clear distinction between movement and reference operations
- **Visual Feedback**: Different colored buttons prevent user confusion
- **Controlled Movement**: Physical homing uses safe, user-defined speeds
- **Immediate Feedback**: OLED display confirms all home operations

### Workflow Integration

- **Project Setup**: Establish references that make sense for each specific project
- **Equipment Transport**: Re-establish references after moving equipment
- **Calibration**: Support both manual and automatic calibration workflows
- **Recovery**: Quick recovery from power loss or manual positioning

## Advanced Usage

### Timelapse Setup

```javascript
// 1. Manually position for perfect framing
// 2. Set as home reference
handleSetHome();

// 3. Create timelapse program starting from this reference
const program = {
  name: "SUNSET",
  startPos: 0, // Start from home reference
  endPos: 2000, // End position
  duration: 7200000, // 2 hours total
};
```

### Multi-Point Calibration

```javascript
// Set multiple reference points for complex sequences
// Home = starting position
handleSetHome();

// Create program with multiple waypoints relative to home
const waypoints = [
  { pos: 0, speed: 1000 }, // Start at home
  { pos: 500, speed: 2000 }, // Slow to first waypoint
  { pos: 1500, speed: 1000 }, // Normal to second waypoint
  { pos: 0, speed: 500 }, // Slow return to home
];
```

This flexible home positioning system makes Motorillo adaptable to a wide variety of workflows, from simple timelapse setups to complex multi-position cinematography sequences.
