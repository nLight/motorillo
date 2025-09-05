#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Configuration structure
struct SliderConfig {
  uint16_t magic;            // Magic number for validation
  uint16_t totalSteps;       // Total steps for full slider travel
  uint32_t speed;            // Default speed value (milliseconds)
  uint8_t acceleration;      // Acceleration steps
  uint8_t programCount;      // Number of stored programs
  uint8_t microstepping;     // Microstepping mode
};

// Program types
enum ProgramType {
  PROGRAM_TYPE_LOOP = 0,     // Simple forward/backward loop
  PROGRAM_TYPE_COMPLEX = 1   // Individual movement steps
};

// Loop program structure (for forward/backward cycles)
struct LoopProgram {
  uint16_t steps;            // Number of steps to move forward/backward
  uint32_t delayMs;          // Delay between steps in milliseconds
  uint8_t cycles;            // Number of forward/backward cycles
};

// Complex program structure (existing format)
struct MovementStep {
  uint16_t position;          // Target position in steps
  uint32_t speed;             // Speed value (milliseconds)
  uint16_t pauseMs;           // Pause after reaching position (ms)
};

// Unified program header
struct ProgramHeader {
  uint8_t type;               // ProgramType (0=loop, 1=complex)
  uint8_t stepCount;          // Number of steps (for complex) or cycles (for loop)
  char name[9];               // Program name (8 chars + null)
};

// Global configuration instance
extern SliderConfig config;

// Configuration constants
const uint16_t CONFIG_MAGIC = 0xA5C3;
const int MAX_PROGRAMS = 5;  // Reduced from 10 to fit in 1024-byte EEPROM
const int MAX_STEPS_PER_PROGRAM = 10; // For complex programs
const int CONFIG_ADDR = 0;

// Program storage - much more efficient now!
// Loop programs: ~16 bytes each
// Complex programs: header + steps (up to ~120 bytes each)
const int PROGRAMS_ADDR = CONFIG_ADDR + sizeof(SliderConfig);
const int PROGRAM_SIZE = 128; // Fixed size per program slot
const int TOTAL_PROGRAM_STORAGE = MAX_PROGRAMS * PROGRAM_SIZE;


// Function declarations
void loadConfig();
void saveConfig();

// New efficient program functions
void saveLoopProgram(uint8_t programId, const char* name, LoopProgram program);
bool loadLoopProgram(uint8_t programId, LoopProgram* program);
void saveComplexProgram(uint8_t programId, const char* name, MovementStep* steps, uint8_t stepCount);
uint8_t loadComplexProgram(uint8_t programId, MovementStep* steps);
uint8_t getProgramType(uint8_t programId);
void loadProgramName(uint8_t programId, char* name);
void saveProgramName(uint8_t programId, const char* name);

// Legacy functions (for backward compatibility)
void saveProgram(uint8_t programId, MovementStep* steps, uint8_t stepCount);
uint8_t loadProgram(uint8_t programId, MovementStep* steps);

#endif // CONFIG_MANAGER_H
