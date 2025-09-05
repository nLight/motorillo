# Set Home Feature Documentation

## Overview
A new "Set Home" feature has been added to allow users to reset the current position to zero without physically moving the motor. This is useful for re-calibrating the position reference when the slider is manually positioned or after power cycling.

## Changes Made

### Arduino Firmware (`steppper.ino`)
- **New Command**: `SETHOME` - Sets the current position to zero without moving the motor
- **Existing Command**: `HOME` - Still available, moves the motor to position 0

### Web Interface (`webusb/`)
- **New Button**: "Set Home (Current Position)" - Green button that sends `SETHOME` command
- **Modified Button**: "Go to Home (Position 0)" - Orange button that sends `HOME` command
- **Visual Distinction**: Different colors to clearly indicate which button moves the motor

## Usage

### From Web Interface
1. **Set Home (Current Position)**: Click the green "Set Home (Current Position)" button to mark the current position as home without moving
2. **Go to Home (Position 0)**: Click the orange "Go to Home (Position 0)" button to physically move the motor to position 0

### From Serial Commands
- `SETHOME` - Sets current position to 0 without movement
- `HOME` - Moves motor to position 0

## Use Cases
- **Manual Positioning**: After manually moving the slider to a desired starting position
- **Power Cycling**: After restarting the Arduino when the motor position is unknown
- **Calibration**: Setting a new reference point for timelapse or program sequences
- **Emergency Reset**: Quick position reset without waiting for motor movement

## Safety Notes
- The `SETHOME` command only changes the software position counter
- No physical movement occurs, so ensure the slider is actually at your desired home position
- Use `HOME` command if you want to physically move to a known zero position

## Technical Details
- Command parsing in `processCommand()` function
- Updates `currentPosition` global variable
- Displays "Set Home" message on OLED
- Maintains all existing home functionality through `HOME` command