# Motorillo Architecture Overview

This document provides a comprehensive overview of Motorillo's software architecture, design patterns, and module organization.

## System Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                    USER INTERFACES                          │
├──────────────────┬──────────────────────────────────────────┤
│   Web Browser    │            OLED + Button                 │
│   (WebUSB Mode)  │         (Standalone Mode)                │
├──────────────────┴──────────────────────────────────────────┤
│                 COMMUNICATION LAYER                         │
├─────────────────────────────────────────────────────────────┤
│                 COMMAND PROCESSOR                           │
├─────────────────────────────────────────────────────────────┤
│                    CORE MODULES                             │
├─────────┬─────────┬─────────┬─────────┬───────────────────────┤
│ Motor   │ Display │  Menu   │ Config  │      EEPROM          │
│ Control │ Manager │ System  │ Manager │     Storage          │
└─────────┴─────────┴─────────┴─────────┴───────────────────────┘
```

### Modular Design Principles

**Separation of Concerns**: Each module handles a specific aspect of functionality

- **Motor Control**: Step generation, positioning, movement timing
- **Display Management**: OLED output, status display, menu rendering
- **Menu System**: Navigation, user interaction, program selection
- **Configuration**: Settings storage, EEPROM management, data persistence
- **Communication**: WebUSB protocol, command parsing, response handling

**Loose Coupling**: Modules communicate through well-defined interfaces

- Clear function signatures with minimal dependencies
- Event-driven communication where appropriate
- Shared state management through controlled interfaces

## Core Modules

### Motor Control (`src/motor_control.h/cpp`)

**Responsibilities**:

- Stepper motor step generation
- Position tracking and management
- Speed control and timing
- Movement execution and coordination

**Key Functions**:

```cpp
void setupMotorPins();                           // Initialize hardware pins
void moveToPositionWithSpeed(uint16_t pos, uint32_t speedMs);
void executeStoredProgram();                     // Run saved motion sequences
void updateMotorStatus();                        // Position tracking
```

**State Management**:

```cpp
extern long currentPosition;      // Current motor position
extern bool programRunning;       // Program execution state
extern bool programPaused;        // Pause state
```

**Design Patterns**:

- **State Machine**: Program execution states (stopped/running/paused)
- **Command Pattern**: Movement commands with parameters
- **Template Method**: Common movement logic with variable timing

### Display Manager (`src/display_manager.h/cpp`)

**Responsibilities**:

- OLED display initialization and control
- Status information rendering
- Menu display formatting
- Message display with timing

**Key Functions**:

```cpp
void setupDisplay();                             // Hardware initialization
void updateDisplay();                           // Refresh display content
void displayMessage(const char* msg, int duration);
void displayMenuItems(int selectedIndex);       // Menu rendering
```

**Display States**:

```cpp
extern unsigned long lastDisplayUpdate;         // Update timing
extern const unsigned long DISPLAY_UPDATE_INTERVAL;
```

**Design Patterns**:

- **Observer Pattern**: Display updates based on state changes
- **Composite Pattern**: Complex display layouts from simple elements
- **Strategy Pattern**: Different display modes (menu/status/message)

### Menu System (`src/menu_system.h/cpp`)

**Responsibilities**:

- Button input handling and debouncing
- Menu navigation logic
- Program selection and execution
- Mode switching (menu ↔ program execution)

**Key Functions**:

```cpp
void setupButton();                             // Input initialization
void checkButton();                            // Input processing with debouncing
void buildMenuItems();                         // Dynamic menu construction
void enterMenuMode();                          // Mode switching
void exitMenuMode();
```

**State Variables**:

```cpp
extern bool inMenuMode;                        // Current interface mode
extern int selectedMenuItem;                   // Navigation state
extern unsigned long lastButtonPress;         // Debouncing timing
```

**Design Patterns**:

- **State Machine**: Menu navigation states
- **Command Pattern**: Menu actions trigger operations
- **Builder Pattern**: Dynamic menu construction based on stored programs

### Configuration Manager (`src/config_manager.h/cpp`)

**Responsibilities**:

- EEPROM data organization and access
- Program storage and retrieval (loop and complex programs)
- Configuration persistence
- Data validation and integrity

**Key Functions**:

```cpp
void loadConfig();                             // System settings
void saveConfig();
bool loadLoopProgram(uint8_t id, LoopProgram* prog);
bool saveLoopProgram(uint8_t id, const LoopProgram* prog);
void loadProgramName(uint8_t id, char* name);
uint8_t getProgramType(uint8_t id);           // Program type detection
```

**Data Structures**:

```cpp
struct LoopProgram {
  uint16_t steps;     // Steps per direction
  uint32_t delayMs;   // Delay between steps
  uint8_t cycles;     // Number of cycles
};

struct ComplexProgram {
  uint8_t stepCount;
  struct {
    uint16_t position;
    uint32_t speedMs;
    uint16_t pauseMs;
  } steps[MAX_COMPLEX_STEPS];
};
```

**Design Patterns**:

- **Repository Pattern**: Abstract EEPROM access
- **Factory Pattern**: Program creation based on type
- **Memento Pattern**: State persistence and restoration

### Command Processor (`src/command_processor.h/cpp`)

**Responsibilities**:

- WebUSB communication protocol handling
- Binary command parsing and validation
- Response generation and transmission
- Error handling and logging

**Key Functions**:

```cpp
void processCommandCode(uint8_t cmdCode, char* data, int dataLen);
void sendProgramData();                        // Bulk data transmission
void handleConfigCommand(const uint8_t* data); // Configuration updates
void handleProgramCommand(const uint8_t* data); // Program management
```

**Protocol Design**:

```cpp
// Command structure
enum CommandCodes {
  CMD_CONFIG = 1,           // System configuration
  CMD_POS_WITH_SPEED = 15,  // Position movement
  CMD_LOOP_PROGRAM = 9,     // Simple programs
  CMD_PROGRAM = 10,         // Complex programs
  CMD_GET_ALL_DATA = 13,    // Data synchronization
  CMD_DEBUG_INFO = 14       // Connection health
};
```

**Design Patterns**:

- **Command Pattern**: Each command encapsulates operation
- **Strategy Pattern**: Different parsing strategies for command types
- **Chain of Responsibility**: Command processing pipeline

## Communication Architecture

### WebUSB Protocol Design

**Binary Format Benefits**:

- Memory efficiency (crucial for Arduino constraints)
- Fast parsing (no string manipulation overhead)
- Type safety (fixed-size fields prevent errors)
- Cross-platform compatibility (little-endian encoding)

**Protocol Stack**:

```
Application Layer  │ Movement Commands, Program Data
─────────────────  ├─────────────────────────────────
Transport Layer    │ Binary Protocol, Command Codes
─────────────────  ├─────────────────────────────────
Physical Layer     │ WebUSB, Arduino USB Stack
```

**Data Flow**:

```
Browser JavaScript ──[Binary Commands]──→ Arduino
                  ←──[Status/Data]───────
```

### Connection Management

**State Machine**:

```
┌─────────────┐    WebUSB Available    ┌──────────────────┐
│ Standalone  │ ───────────────────→   │ Programming Mode │
│    Mode     │ ←─────────────────────  │                  │
└─────────────┘    Connection Lost     └──────────────────┘
```

**Disconnection Detection**:

- **Arduino Side**: Timeout-based detection (3 seconds)
- **Browser Side**: Event-driven + ping-based health monitoring
- **Recovery**: Automatic fallback to standalone mode

## Memory Management

### EEPROM Organization

```
Address Range  │ Content              │ Size    │ Notes
───────────────┼──────────────────────┼─────────┼──────────────────
0x000 - 0x007  │ Configuration        │ 8 bytes │ System settings
0x008 - 0x027  │ Program Names        │ 32 bytes│ 8 chars × 4 programs
0x028 - 0x067  │ Loop Programs        │ 64 bytes│ 16 bytes × 4 programs
0x068 - 0x1FF  │ Complex Programs     │ 408 bytes│ Variable length
0x200 - 0x3FF  │ Reserved/Future      │ 512 bytes│ Expansion space
```

**Memory Constraints**:

- Arduino Uno: 1024 bytes EEPROM total
- Memory-optimized string handling (no String class)
- F() macro for flash storage of constants
- Aggressive removal of debug messages in production

### RAM Optimization

**Techniques Used**:

```cpp
// Flash string storage
const char* const messages[] PROGMEM = {
  "Program Running",
  "Position: ",
  "Speed: "
};

// Minimal buffer sizes
char nameBuffer[9];  // Exactly sized for 8-char names + null

// Avoid String class overhead
void appendInt(char* buffer, int value) {
  // Manual integer to string conversion
}
```

## Design Patterns Implementation

### State Machine Pattern

**Menu Navigation**:

```cpp
enum MenuState {
  MENU_BROWSE,     // Scrolling through programs
  MENU_EXECUTE,    // Running selected program
  MENU_PAUSED      // Program paused
};

void handleMenuState(MenuState state, ButtonEvent event) {
  switch(state) {
    case MENU_BROWSE:
      if(event == BUTTON_PRESS) enterExecuteMode();
      break;
    case MENU_EXECUTE:
      if(event == BUTTON_PRESS) pauseProgram();
      break;
    // ...
  }
}
```

### Command Pattern

**Movement Commands**:

```cpp
class MovementCommand {
  uint16_t targetPosition;
  uint32_t speedMs;

  void execute() {
    moveToPositionWithSpeed(targetPosition, speedMs);
  }
};
```

### Observer Pattern

**Status Updates**:

```cpp
void notifyPositionChange(long newPosition) {
  currentPosition = newPosition;
  updateDisplay();        // Observer 1: Display
  sendPositionUpdate();   // Observer 2: WebUSB client
}
```

## Error Handling Strategy

### Graceful Degradation

**Connection Loss**:

- WebUSB disconnection → Continue in standalone mode
- Display failure → Continue with basic operation
- EEPROM corruption → Use default values

**Input Validation**:

```cpp
bool validateSpeed(uint32_t speedMs) {
  if(speedMs < 1 || speedMs > MAX_SPEED_MS) {
    displayMessage("Invalid Speed", 2000);
    return false;
  }
  return true;
}
```

### Resource Management

**Memory Safety**:

- Bounds checking on all array access
- EEPROM address validation
- Buffer overflow prevention

**Hardware Safety**:

- Speed limiting to prevent mechanical damage
- Position bounds checking
- Thermal protection considerations

This architecture provides a robust, maintainable foundation for Motorillo's diverse functionality while respecting Arduino's resource constraints.
