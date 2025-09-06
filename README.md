# Arduino Stepper Motor Controller - Modular Refactor

This project has been refactored into a modular structure for better maintainability and code organization.

## Project Structure

```
steppper/
├── steppper.ino          # Main Arduino sketch (simplified)
├── src/
│   ├── config_manager.h/.cpp    # EEPROM configuration and program storage
│   ├── motor_control.h/.cpp     # Stepper motor control and movement
│   ├── menu_system.h/.cpp       # OLED menu navigation and button handling
│   ├── display_manager.h/.cpp   # OLED display management and animations
│   └── command_processor.h/.cpp # WebUSB/Serial command processing
├── webusb/
│   ├── index.html        # Web interface
│   ├── script.js         # JavaScript functionality
│   ├── serial.js         # WebUSB serial communication
│   └── styles.css        # CSS styling
└── README.md             # This file
```

## Module Descriptions

### 1. Config Manager (`config_manager.h/.cpp`)

- **Purpose**: Handles EEPROM operations for program storage
- **Functions**: Store/retrieve movement programs, manage program count
- **Data Structures**: `SliderConfig` (simplified), `MovementStep`, `LoopProgram`
- **Constants**: Default motor settings (speed, acceleration, microstepping)

### 2. Motor Control (`motor_control.h/.cpp`)

- **Purpose**: Controls stepper motor movement, microstepping, and positioning
- **Functions**: Move to position, run programs, set microstepping modes
- **Features**: Acceleration/deceleration, speed conversion, position tracking

### 3. Menu System (`menu_system.h/.cpp`)

- **Purpose**: Handles OLED menu navigation and button input processing
- **Functions**: Menu navigation, program selection, pause/resume functionality
- **Features**: Debounced button input, long/short press detection

### 4. Display Manager (`display_manager.h/.cpp`)

- **Purpose**: Manages OLED display output and animations
- **Functions**: Update display, show messages, boot animation
- **Features**: Status display, menu rendering, visual feedback

### 5. Command Processor (`command_processor.h/.cpp`)

- **Purpose**: Processes WebUSB and serial commands
- **Functions**: Parse commands, execute actions, send responses
- **Features**: Program management, manual control, bulk EEPROM loading
- **Simplified**: No configuration commands - programs contain their own speed settings

## Benefits of Modular Structure

1. **Maintainability**: Each module has a single responsibility
2. **Reusability**: Modules can be reused in other projects
3. **Testing**: Individual modules can be tested separately
4. **Collaboration**: Multiple developers can work on different modules
5. **Readability**: Smaller, focused files are easier to understand
6. **Debugging**: Issues can be isolated to specific modules

## Compilation

The Arduino IDE will automatically compile all `.cpp` and `.h` files in the project directory. The modular structure doesn't change the compilation process.

## Usage

The functionality remains exactly the same as the original monolithic version. All features are preserved:

- WebUSB browser control
- Standalone operation with OLED menu
- Program storage and execution with individual speed settings
- Button control with pause/resume
- Program management (no global configuration needed)
- Microstepping support
- **Bulk EEPROM loading** for efficient data transfer

## Migration Notes

- All existing functionality is preserved
- EEPROM data format remains compatible
- Web interface unchanged
- No breaking changes to user-facing features

## Future Improvements

With the modular structure, future enhancements can be easily added:

- Unit testing for individual modules
- Additional motor control algorithms
- Network connectivity modules
- Advanced display features
- Hardware abstraction layers
