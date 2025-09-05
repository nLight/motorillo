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

// Save a loop program (very efficient storage)
void saveLoopProgram(uint8_t programId, const char* name, uint16_t steps, uint32_t delayMs, uint8_t cycles, uint8_t speedUnit) {
  if (programId >= MAX_PROGRAMS) return;

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);

  // Save program header
  ProgramHeader header;
  header.type = PROGRAM_TYPE_LOOP;
  header.stepCount = cycles;  // For loops, stepCount = cycles
  strncpy(header.name, name, 8);
  header.name[8] = '\0';
  EEPROM.put(addr, header);

  // Save loop parameters (very compact!)
  LoopProgram loopProg;
  loopProg.steps = steps;
  loopProg.delayMs = delayMs;
  loopProg.cycles = cycles;
  loopProg.speedUnit = speedUnit;
  EEPROM.put(addr + sizeof(ProgramHeader), loopProg);

  // Update program count if necessary
  if (programId >= config.programCount) {
    config.programCount = programId + 1;
    saveConfig();
  }
}

// Load a loop program
bool loadLoopProgram(uint8_t programId, LoopProgram* program) {
  if (programId >= MAX_PROGRAMS) return false;

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);

  // Check program type
  ProgramHeader header;
  EEPROM.get(addr, header);
  if (header.type != PROGRAM_TYPE_LOOP) return false;

  // Load loop parameters
  EEPROM.get(addr + sizeof(ProgramHeader), *program);
  return true;
}

// Save a complex program (step-by-step)
void saveComplexProgram(uint8_t programId, const char* name, MovementStep* steps, uint8_t stepCount) {
  if (programId >= MAX_PROGRAMS || stepCount > MAX_STEPS_PER_PROGRAM) return;

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);

  // Save program header
  ProgramHeader header;
  header.type = PROGRAM_TYPE_COMPLEX;
  header.stepCount = stepCount;
  strncpy(header.name, name, 8);
  header.name[8] = '\0';
  EEPROM.put(addr, header);

  // Save movement steps
  int stepAddr = addr + sizeof(ProgramHeader);
  for (int i = 0; i < stepCount; i++) {
    EEPROM.put(stepAddr + (i * sizeof(MovementStep)), steps[i]);
  }

  // Update program count if necessary
  if (programId >= config.programCount) {
    config.programCount = programId + 1;
    saveConfig();
  }
}

// Load a complex program
uint8_t loadComplexProgram(uint8_t programId, MovementStep* steps) {
  if (programId >= MAX_PROGRAMS) return 0;

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);

  // Check program type
  ProgramHeader header;
  EEPROM.get(addr, header);
  if (header.type != PROGRAM_TYPE_COMPLEX) return 0;

  // Load movement steps
  int stepAddr = addr + sizeof(ProgramHeader);
  for (int i = 0; i < header.stepCount; i++) {
    EEPROM.get(stepAddr + (i * sizeof(MovementStep)), steps[i]);
  }

  return header.stepCount;
}

// Get program type
uint8_t getProgramType(uint8_t programId) {
  if (programId >= MAX_PROGRAMS) return 255; // Invalid

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);
  ProgramHeader header;
  EEPROM.get(addr, header);
  return header.type;
}

// Load program name
void loadProgramName(uint8_t programId, char* name) {
  if (programId >= MAX_PROGRAMS) {
    strcpy(name, "INVALID");
    return;
  }

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);
  ProgramHeader header;
  EEPROM.get(addr, header);

  // Check if we have a valid name
  if (header.name[0] != '\0' && header.name[0] >= 32 && header.name[0] <= 126) {
    strcpy(name, header.name);
  } else {
    // Generate default name
    sprintf(name, "PGM%d", programId + 1);
  }
}

// Save program name
void saveProgramName(uint8_t programId, const char* name) {
  if (programId >= MAX_PROGRAMS) return;

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);
  ProgramHeader header;
  EEPROM.get(addr, header);

  // Update name in header
  strncpy(header.name, name, 8);
  header.name[8] = '\0';
  EEPROM.put(addr, header);
}

// Legacy functions for backward compatibility
void saveProgram(uint8_t programId, MovementStep* steps, uint8_t stepCount) {
  saveComplexProgram(programId, "", steps, stepCount);
}

uint8_t loadProgram(uint8_t programId, MovementStep* steps) {
  return loadComplexProgram(programId, steps);
}