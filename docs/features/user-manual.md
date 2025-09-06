# Motorillo User Manual

Welcome to Motorillo, your precision stepper motor controller for camera sliders, timelapses, and automation projects. This manual will guide you through all features and operations.

## Getting Started

### First Connection

1. **Power Up**: Connect your Arduino to computer via USB
2. **Open Browser**: Use Chrome or Edge (WebUSB required)
3. **Navigate**: Go to `http://localhost:8000` (after starting local server)
4. **Connect**: Click "Connect Slider" and select your Arduino device

### Initial Setup

**Configure Your Slider**:

- **Total Steps**: Set the maximum travel distance (default: 2000 steps)
- **Default Speed**: Set movement timing (default: 1000ms per step)
- **Acceleration**: Smoothness of speed changes (default: 50)

## Operating Modes

### WebUSB Mode (Browser Control)

When connected to browser, Motorillo provides full programming capabilities:

**Manual Controls**:

- **Position Movement**: Move to specific step positions
- **Speed Control**: Adjust movement speed from 1ms to hours per step
- **Home Functions**: Set reference points or return to home position

**Program Creation**:

- **Loop Programs**: Simple back-and-forth motion with timing control
- **Complex Programs**: Multi-waypoint sequences with variable speeds

### Standalone Mode (OLED + Button)

When WebUSB is disconnected, use physical controls:

**Navigation**:

- **Single Press**: Navigate through saved programs
- **Long Press**: Start/stop selected program
- **During Execution**: Single press pauses/resumes

**Display Information**:

- Current position and program status
- Selected program name and progress
- System mode and connection state

## Creating Motion Programs

### Loop Programs

Perfect for timelapse and repetitive motions:

```javascript
Program Settings:
- Name: "SUNSET" (up to 8 characters)
- Steps: 1000 (distance to travel)
- Delay: 30000ms (30 seconds between steps)
- Cycles: 1 (forward and back once)
```

**Use Cases**:

- **Timelapse**: Slow camera movement over time
- **Product Photography**: Rotating turntable sequences
- **Focus Stacking**: Macro photography depth sequences

### Complex Programs

For advanced multi-point sequences:

```javascript
Waypoint Example:
1. Position 0, Speed 2000ms, Pause 1000ms (slow start)
2. Position 500, Speed 1000ms, Pause 500ms (normal speed)
3. Position 1500, Speed 5000ms, Pause 2000ms (slow finish)
4. Position 0, Speed 1500ms, Pause 0ms (return home)
```

**Use Cases**:

- **Cinematography**: Complex camera movements
- **Automation**: Multi-step manufacturing sequences
- **Art Projects**: Custom motion patterns

## Speed Control Guide

### Understanding Speed Values

Speed is measured in **milliseconds per step**:

- **Lower values = Faster movement**
- **Higher values = Slower movement**

### Recommended Speed Ranges

| Application           | Speed Range  | Example  | Duration Per Step |
| --------------------- | ------------ | -------- | ----------------- |
| **Quick Setup**       | 10-100ms     | 50ms     | 0.05 seconds      |
| **Normal Operation**  | 500-2000ms   | 1000ms   | 1 second          |
| **Smooth Timelapse**  | 5000-30000ms | 15000ms  | 15 seconds        |
| **Ultra-Slow Motion** | 60000ms+     | 300000ms | 5 minutes         |

### Speed Selection Tips

**For Different Projects**:

- **Product Photography**: 1000-5000ms (smooth rotation)
- **Sunset Timelapse**: 30000-120000ms (very slow movement)
- **Macro Focus Stack**: 500-2000ms (precise positioning)
- **Video Production**: 1000-3000ms (cinematic movement)

## Advanced Features

### Home Positioning

**Two Types of Home Operations**:

1. **Set Home (Green Button)**:

   - Sets current position as reference point
   - No motor movement
   - Use after manual positioning

2. **Go to Home (Orange Button)**:
   - Moves motor to position 0
   - Uses current speed setting
   - Use for returning to known position

### Program Management

**Saving Programs**:

- Up to 5 programs stored in Arduino memory
- Programs persist through power cycles
- WebUSB automatically loads saved programs

**Program Types**:

- **Loop Programs**: Simple back-and-forth (16 bytes each)
- **Complex Programs**: Multi-waypoint (variable size)

### Connection Management

**Automatic Mode Switching**:

- Arduino detects WebUSB connection automatically
- Switches between browser and standalone modes
- Graceful fallback if connection is lost

**Disconnection Handling**:

- Arduino continues operating with OLED interface
- All programs remain accessible via button navigation
- Reconnection restores full WebUSB functionality

## Practical Applications

### Timelapse Photography

**Basic Setup**:

1. Position camera at starting point
2. Use "Set Home (Current Position)"
3. Create loop program with desired duration
4. Start program and let it run

**Example - 2 Hour Sunset**:

```
Program: SUNSET
Steps: 240 (240 positions over travel)
Delay: 30000ms (30 seconds per step)
Cycles: 1 (single sweep)
Total Time: 240 × 30s = 2 hours
```

### Product Photography

**360° Turntable**:

```
Program: PRODUCT
Steps: 72 (72 positions = 5° per step)
Delay: 2000ms (2 seconds per position)
Cycles: 1 (full rotation)
Total Time: 72 × 2s = 2.4 minutes
```

### Video Production

**Smooth Camera Movement**:

```
Complex Program: CINEMATIC
- Start: Position 0, Speed 3000ms (slow start)
- Middle: Position 1000, Speed 1000ms (normal)
- End: Position 2000, Speed 5000ms (slow stop)
- Return: Position 0, Speed 2000ms (medium return)
```

### Focus Stacking

**Macro Photography**:

```
Program: FOCUS
Steps: 50 (50 focus positions)
Delay: 1000ms (1 second between shots)
Cycles: 1 (single pass)
Total Time: 50 seconds
```

## Troubleshooting Common Issues

### Connection Problems

**WebUSB Won't Connect**:

- Ensure Chrome/Edge browser (not Firefox/Safari)
- Check USB cable supports data (not power-only)
- Try different USB port on computer
- Restart browser and Arduino

**Connection Drops Frequently**:

- Check USB cable quality
- Verify stable power supply
- Move away from electromagnetic interference
- Try powered USB hub

### Movement Issues

**Motor Not Moving**:

- Verify wiring connections (STEP/DIR pins)
- Check motor driver power supply (12V)
- Test with slower speed settings
- Ensure motor driver is not overheating

**Wrong Direction**:

- Swap motor wires (A+ ↔ A- OR B+ ↔ B-)
- Or invert direction in firmware

**Skipping Steps**:

- Reduce speed (increase milliseconds per step)
- Check mechanical binding
- Verify adequate power supply current
- Adjust motor driver current limiting

### Display Issues

**OLED Blank**:

- Check I2C wiring (SDA/SCL connections)
- Verify power supply to display
- Try different I2C address in firmware

**Button Not Responding**:

- Check button wiring to pin 2 and GND
- Test button continuity with multimeter
- Verify pullup configuration in firmware

## Maintenance and Care

### Regular Maintenance

- Keep stepper motor and rails clean
- Check all connections periodically
- Update firmware when new versions available
- Backup important programs via WebUSB export

### Storage and Transport

- Power down system before moving
- Secure moving parts during transport
- Use "Set Home" to re-establish reference after setup

### Performance Optimization

- Use appropriate speeds for your mechanical system
- Consider microstepping for smoother motion
- Add proper heat sinking for motor drivers
- Use quality power supplies for stable operation

This user manual covers all essential operations. For technical details, troubleshooting, or development information, see the additional documentation in the `docs/` folder.
