# Extended Speed Range Usage Guide

## Overview

The motorized slider now uses **milliseconds exclusively** for all timing, providing extended speed ranges for slower movements perfect for timelapse sequences.

## New Binary Command Format

All commands now use **binary format** for maximum memory efficiency and performance. No more comma-separated strings!

### Configuration Command (CMD_CONFIG = 1)

**Binary format:** 9 bytes total

- Byte 0: Command code (1)
- Bytes 1-2: totalSteps (uint16, little-endian)
- Bytes 3-6: speed (uint32, little-endian)
- Byte 7: acceleration (uint8)
- Byte 8: microstepping (uint8)

**Examples:**

#### Fast Movement

```
[1] [2000 as uint16] [1 as uint32] [50] [1]
```

- 2000 total steps
- 1ms between steps (1000 steps/second)
- 1x microstepping

#### Slow Movement

```
[1] [2000 as uint16] [500 as uint32] [50] [1]
```

- 2000 total steps
- 500ms between steps (2 steps/second)
- 1x microstepping

#### Very Slow Movement

```
[1] [2000 as uint16] [30000 as uint32] [50] [1]
```

- 2000 total steps
- 30,000ms between steps (30 seconds per step)
- 1x microstepping

## Program Commands

### Loop Program

### Loop Program (CMD_LOOP_PROGRAM = 9)

**Binary format:** 17 bytes total

- Byte 0: Command code (9)
- Byte 1: programId (uint8)
- Bytes 2-9: name (8 bytes, space-padded)
- Bytes 10-11: steps (uint16, little-endian)
- Bytes 12-15: delayMs (uint32, little-endian)
- Byte 16: cycles (uint8)

**Example:**

```
[9] [0] ['T','I','M','E','L','A','P','S','E'] [1000 as uint16] [30000 as uint32] [5]
```

- Program ID 0
- Name: "TIMELAPSE"
- 1000 steps per direction
- 30 second delay between steps
- 5 cycles

### Complex Program (CMD_PROGRAM = 10)

**Binary format:** Variable length

- Byte 0: Command code (10)
- Byte 1: programId (uint8)
- Bytes 2-9: name (8 bytes, space-padded)
- Byte 10: stepCount (uint8)
- Bytes 11+: steps (8 bytes each: position uint16 + speed uint32 + pauseMs uint16)

**Example with 2 steps:**

```
[10] [1] ['M','O','V','I','E',' ',' ',' '] [2]
[position1 uint16] [speed1 uint32] [pause1 uint16]
[position2 uint16] [speed2 uint32] [pause2 uint16]
```

**Parameters:**

- `9` = CMD_LOOP_PROGRAM command code
- `programId` = Program slot (0-9)
- `name` = Program name (8 chars max)
- `steps` = Steps per direction
- `delayMs` = Delay between steps in milliseconds
- `cycles` = Number of forward/backward cycles

### Complex Program

```
10,programId,name,stepCount,pos1,speed1,pause1,pos2,speed2,pause2,...
```

**Parameters:**

- `10` = CMD_PROGRAM command code
- `programId` = Program slot (0-9)
- `name` = Program name (8 chars max)
- `stepCount` = Number of movement steps
- `pos1,speed1,pause1` = Position, speed (ms), pause (ms) for step 1
- Additional steps follow the same pattern

### Example: Mixed Speed Program

```
10,0,SLOWLAPSE,3,0,5000,1000,1000,2000,500,2000,10000,2000
```

This creates a program with:

- **Step 1:** Position 0, 5000ms delay, 1000ms pause
- **Step 2:** Position 1000, 2000ms delay, 500ms pause
- **Step 3:** Position 2000, 10000ms delay, 2000ms pause

## Simple Commands

### Move to Position (CMD_POS = 2)

**Binary format:** 3 bytes

- Byte 0: Command code (2)
- Bytes 1-2: position (uint16, little-endian)

### Run Program (CMD_RUN = 3)

**Binary format:** 2 bytes

- Byte 0: Command code (3)
- Byte 1: programId (uint8)

### Control Commands (No data)

- CMD_START = 4: Start program execution
- CMD_STOP = 5: Stop program execution
- CMD_HOME = 6: Move to home position (0)
- CMD_GET_CONFIG = 7: Request current configuration
- CMD_SETHOME = 8: Set current position as home

## Web Interface Changes

### Configuration Tab

- Speed input is now in milliseconds only
- No unit selector needed - everything uses milliseconds
- Extended range: 1ms to 4,294,967,295ms (~49.7 days)

### Program Mode

- All speeds are now in milliseconds
- Simplified interface - no unit selectors
- Consistent timing across all program steps

## Practical Use Cases

### Half-Second Intervals

```
1,2000,500,50,1
```

- 500ms between steps = 0.5 seconds
- Perfect for smooth product rotation

### 1.5 Second Timelapse

```
1,1000,1500,50,1
```

- 1500ms = 1.5 seconds between steps
- Great for cloud movement capture

### Ultra-Slow Timelapse

```
1,2000,60000,50,1
```

- 60,000ms = 60 seconds (1 minute) between steps
- Perfect for sunrise/sunset capture

### Sunrise/Sunset Capture

```
10,0,SUNRISE,2,0,60000,5000,2000,60000,5000
```

- Move from position 0 to 2000 over 2+ hours
- 60 seconds between each step
- Perfect for capturing sun movement

### Product Photography

Slow capture sequence:

```
10,0,PRODUCT,2,0,30000,10000,1000,30000,10000
```

- Slow capture sequence (30s between steps)

## EEPROM Configuration

- **Storage**: Configuration automatically saved to EEPROM
- **Max Programs**: 5 programs (reduced from 10 to fit 1024-byte EEPROM)
- **Debug**: Send command code 99 to get EEPROM info
- **ESP32 Support**: Automatic EEPROM.commit() on ESP32 boards

## Debug Commands

- **Command 99**: EEPROM info (addresses, sizes)
- **Command 100 + programId**: Dump program EEPROM data
- **Command 101 + test data**: Test binary parsing

## Troubleshooting

### Config Not Persisting

1. Check Arduino board EEPROM size (Uno/Nano = 1024 bytes)
2. Use debug command 99 to verify EEPROM addresses
3. Ensure WebUSB interface is used (binary format)
4. Check serial output for "Config saved successfully"

### Loop Program Not Saving

1. Check serial output for "Parsed:" messages
2. Use command 100 to dump EEPROM program data
3. Use command 101 to test binary parsing
4. Verify WebUSB connection (binary format required)

## Program Management

### Loading Programs from EEPROM

The WebUI now automatically loads programs from Arduino EEPROM when local data is not available:

1. **Local Storage First**: Programs are loaded from browser localStorage for fast access
2. **EEPROM Fallback**: If no local data exists, WebUI requests program data from Arduino EEPROM
3. **Automatic Sync**: Loaded EEPROM programs are cached in localStorage for future use

### New Commands for EEPROM Access

- **CMD_GET_LOOP_PROGRAM (11)**: Request loop program data from EEPROM
- **CMD_GET_PROGRAM (12)**: Request complex program data from EEPROM
- **CMD_GET_ALL_DATA (13)**: Request all EEPROM data in bulk (config + all programs)
- **CMD_DEBUG_INFO (14)**: Request debug information

### Binary Response Formats

**Loop Program Response:**

```
[programId][name(8)][steps(2)][delayMs(4)][cycles(1)]
```

**Complex Program Response:**

```
[programId][name(8)][stepCount(1)][steps(stepCount*8)]
```

Each step: `[position(2)][speed(4)][pauseMs(2)]`

**Bulk Data Response:**

```
[config(8)][programCount(1)][programs(variable)]
```

Config: `[totalSteps(2)][speed(4)][acceleration(1)][microstepping(1)]`

Each program: `[programId(1)][type(1)][name(8)][data(variable)]`

- Loop program data: `[steps(2)][delayMs(4)][cycles(1)]`
- Complex program data: `[stepCount(1)][steps(stepCount*8)]`

## Maximum Ranges

### Milliseconds Mode (Exclusive)

- Range: 1ms to 4,294,967,295ms (~49.7 days)
- Best for: All operations - normal speed to ultra-slow timelapse

The system automatically handles very long delays by breaking them into smaller chunks while maintaining responsiveness to pause/stop commands.
