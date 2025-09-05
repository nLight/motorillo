#include "config_manager.h"

// Global configuration instance
SliderConfig config;

// Load configuration from EEPROM
void loadConfig() {
  EEPROM.get(CONFIG_ADDR, config);
  if (config.magic != CONFIG_MAGIC) {
    // Initialize default configuration
    config.magic = CONFIG_MAGIC;
    config.totalSteps = 2000;
    config.speed = 1000;
    config.speedUnit = 0;  // microseconds
    config.acceleration = 50;
    config.programCount = 0;
    config.microstepping = 1; // Full step by default
    saveConfig();
  }
}

// Save configuration to EEPROM
void saveConfig() {
  EEPROM.put(CONFIG_ADDR, config);
}

// Save a program to EEPROM
void saveProgram(uint8_t programId, MovementStep* steps, uint8_t stepCount) {
  int addr = PROGRAMS_ADDR + (programId * (sizeof(uint8_t) + MAX_STEPS_PER_PROGRAM * sizeof(MovementStep)));
  EEPROM.put(addr, stepCount);

  // Save each step individually
  int stepAddr = addr + sizeof(uint8_t);
  for (int i = 0; i < stepCount; i++) {
    EEPROM.put(stepAddr + (i * sizeof(MovementStep)), steps[i]);
  }
}

// Load a program from EEPROM
uint8_t loadProgram(uint8_t programId, MovementStep* steps) {
  int addr = PROGRAMS_ADDR + (programId * (sizeof(uint8_t) + MAX_STEPS_PER_PROGRAM * sizeof(MovementStep)));
  uint8_t stepCount;
  EEPROM.get(addr, stepCount);
  if (stepCount > MAX_STEPS_PER_PROGRAM) return 0;

  // Load each step individually
  int stepAddr = addr + sizeof(uint8_t);
  for (int i = 0; i < stepCount; i++) {
    EEPROM.get(stepAddr + (i * sizeof(MovementStep)), steps[i]);
  }
  return stepCount;
}

// Save program name to EEPROM
void saveProgramName(uint8_t programId, const char* name) {
  ProgramName programName;
  strncpy(programName.name, name, 8);
  programName.name[8] = '\0'; // Ensure null termination

  int addr = PROGRAM_NAMES_ADDR + (programId * sizeof(ProgramName));
  EEPROM.put(addr, programName);
}

// Load program name from EEPROM
void loadProgramName(uint8_t programId, char* name) {
  ProgramName programName;
  int addr = PROGRAM_NAMES_ADDR + (programId * sizeof(ProgramName));
  EEPROM.get(addr, programName);

  // Check if we have a valid name (not empty and has printable characters)
  if (programName.name[0] != '\0' && programName.name[0] >= 32 && programName.name[0] <= 126) {
    strcpy(name, programName.name);
  } else {
    // Generate default name if no custom name stored
    sprintf(name, "PGM%d", programId + 1);
  }
}
