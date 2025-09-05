#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Data structures
struct SliderConfig {
  uint16_t magic;           // Magic number to validate EEPROM data
  uint16_t totalSteps;      // Total steps for full slider travel
  uint32_t speed;           // Speed value (microseconds or milliseconds)
  uint8_t speedUnit;        // 0=microseconds, 1=milliseconds
  uint16_t acceleration;    // Acceleration steps
  uint8_t programCount;     // Number of stored programs
  uint8_t microstepping;    // Microstepping mode (1, 2, 4, 8, 16)
  uint8_t reserved[6];      // Reserved for future use
};

struct MovementStep {
  uint16_t position;        // Target position in steps
  uint32_t speed;           // Speed value (microseconds or milliseconds)
  uint8_t speedUnit;        // 0=microseconds, 1=milliseconds
  uint16_t pauseMs;         // Pause after reaching position (ms)
};

struct ProgramName {
  char name[9];             // 8 characters + null terminator
};

// Global configuration instance
extern SliderConfig config;

// Configuration constants
const uint16_t CONFIG_MAGIC = 0xA5C3;
const int MAX_PROGRAMS = 10;
const int MAX_STEPS_PER_PROGRAM = 20;
const int CONFIG_ADDR = 0;
const int PROGRAMS_ADDR = sizeof(SliderConfig);
const int PROGRAM_NAMES_ADDR = PROGRAMS_ADDR + (MAX_PROGRAMS * (sizeof(uint8_t) + MAX_STEPS_PER_PROGRAM * sizeof(MovementStep)));


// Function declarations
void loadConfig();
void saveConfig();
void saveProgram(uint8_t programId, MovementStep* steps, uint8_t stepCount);
uint8_t loadProgram(uint8_t programId, MovementStep* steps);
void saveProgramName(uint8_t programId, const char* name);
void loadProgramName(uint8_t programId, char* name);

#endif // CONFIG_MANAGER_H
