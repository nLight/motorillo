# Extended Speed Range Usage Guide

## Overview
The motorized slider now supports extended speed ranges beyond the previous 1,000,000μs (1 second) limitation. You can now specify speeds in **milliseconds** for slower movements, perfect for fractional seconds and long timelapse sequences.

## New Speed Format

### Configuration Command
```
CFG,totalSteps,speed,acceleration,microstepping,speedUnit
```

**speedUnit parameter:**
- `0` = Microseconds (μs) - Default for backward compatibility
- `1` = Milliseconds (ms) - For slower movements and fractional seconds

### Examples

#### Fast Movement (Microseconds)
```
CFG,2000,1000,50,1,0
```
- 2000 total steps
- 1000μs speed (1ms between steps)
- Microseconds unit

#### Slow Movement (Milliseconds)
```
CFG,2000,500,50,1,1
```
- 2000 total steps
- 500ms between steps (half second per step)
- Milliseconds unit

#### Very Slow Movement (Milliseconds)
```
CFG,2000,30000,50,1,1
```
- 2000 total steps  
- 30,000ms between steps (30 seconds per step)
- Milliseconds unit

## Program Command Extensions

### New Program Format
```
PROGRAM,id,name,stepCount,pos1,speed1,pause1,speedUnit1,pos2,speed2,pause2,speedUnit2,...
```

### Example: Mixed Speed Program
```
PROGRAM,0,SLOWLAPSE,3,0,5000,1000,1,1000,2000,500,0,2000,10000,2000,1
```

This creates a program with:
- **Step 1:** Position 0, 5000ms (5 second) delay, 1s pause, milliseconds unit
- **Step 2:** Position 1000, 2000μs delay, 500ms pause, microseconds unit  
- **Step 3:** Position 2000, 10000ms (10 second) delay, 2s pause, milliseconds unit

## Web Interface Changes

### Configuration Tab

- Speed input now has a dropdown to select **microseconds** or **milliseconds**
- Removed the old max limit of 1,000,000μs

### Program Mode

- Each program step now includes a speed unit selector
- You can mix microseconds and milliseconds within the same program
- Backward compatibility maintained for existing programs

## Practical Use Cases

### Half-Second Intervals
```
CFG,2000,500,50,1,1
```
- 500ms between steps = 0.5 seconds
- Perfect for smooth product rotation

### 1.5 Second Timelapse
```
CFG,1000,1500,50,1,1
```
- 1500ms = 1.5 seconds between steps
- Great for cloud movement capture

### Ultra-Slow Timelapse
```
CFG,2000,60000,50,1,1
```
- 60,000ms = 60 seconds (1 minute) between steps
- Perfect for sunrise/sunset capture

### Sunrise/Sunset Capture

```
PROGRAM,0,SUNRISE,2,0,60,5000,1,2000,60,5000,1
```

- Move from position 0 to 2000 over 2+ hours
- 60 seconds between each step
- Perfect for capturing sun movement

### Product Photography

Mix fast setup with slow capture:

```
PROGRAM,0,PRODUCT,3,0,1000,500,0,500,30,10000,1,1000,30,10000,1
```

- Fast move to start position (1000μs)
- Slow capture sequence (30s between steps)

## Migration Notes

### Existing Programs

- All existing configurations and programs continue to work
- Default speed unit is microseconds (0) for backward compatibility
- No changes needed to existing setups

### Memory Usage

- Configuration struct updated but maintains compatibility
- Programs store speed unit per step for maximum flexibility
- EEPROM layout remains compatible with proper initialization

## Maximum Ranges

### Microseconds Mode
- Range: 1μs to 4,294,967,295μs (~71 minutes)
- Best for: Normal operations, fast sequences

### Milliseconds Mode  
- Range: 1ms to 4,294,967,295ms (~49 days)
- Best for: Slow timelapse, fractional seconds (0.5s, 1.5s, etc.)

The system automatically handles very long delays by breaking them into smaller chunks while maintaining responsiveness to pause/stop commands.
