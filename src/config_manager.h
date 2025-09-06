#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Simplified configuration structure - only stores program count
struct SliderConfig {
  uint16_t magic;            // Magic number for validation
  uint8_t programCount;      // Number of stored programs
};

// Program types (only loop programs supported)
enum ProgramType {
  PROGRAM_TYPE_LOOP = 0     // Simple forward/backward loop
};

// Loop program structure (for forward/backward cycles)
struct LoopProgram {
  uint16_t steps;            // Number of steps to move forward/backward
  uint32_t delayMs;          // Delay between steps in milliseconds
  uint8_t cycles;            // Number of forward/backward cycles
};



// Unified program header
struct ProgramHeader {
  uint8_t type;               // ProgramType (0=loop only)
  uint8_t cycles;             // Number of cycles (for loop programs)
  char name[9];               // Program name (8 chars + null)
};

// Global configuration instance
extern SliderConfig config;

// Configuration constants
const uint16_t CONFIG_MAGIC = 0xA5C3;
const int MAX_PROGRAMS = 5;  // Reduced from 10 to fit in 1024-byte EEPROM
const int CONFIG_ADDR = 0;

// Default motor settings (no longer configurable)
const uint16_t DEFAULT_TOTAL_STEPS = 2000;
const uint32_t DEFAULT_SPEED_MS = 1000;
const uint8_t DEFAULT_ACCELERATION = 50;
const uint8_t DEFAULT_MICROSTEPPING = 1;

// Program storage - loop programs only!
// Loop programs: ~16 bytes each
const int PROGRAMS_ADDR = CONFIG_ADDR + sizeof(SliderConfig);
const int PROGRAM_SIZE = 128; // Fixed size per program slot
const int TOTAL_PROGRAM_STORAGE = MAX_PROGRAMS * PROGRAM_SIZE;


// Function declarations
void loadConfig();
void saveConfig();

// Loop program functions
void saveLoopProgram(uint8_t programId, const char* name, LoopProgram program);
bool loadLoopProgram(uint8_t programId, LoopProgram* program);
uint8_t getProgramType(uint8_t programId);
void loadProgramName(uint8_t programId, char* name);
void saveProgramName(uint8_t programId, const char* name);

#endif // CONFIG_MANAGER_H
