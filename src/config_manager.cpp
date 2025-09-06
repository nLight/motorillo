#include "config_manager.h"

// Global configuration instance
SliderConfig config;

// Load configuration from EEPROM
void loadConfig() {
  EEPROM.get(CONFIG_ADDR, config);

  if (config.magic != CONFIG_MAGIC) {
    // Initialize default configuration
    config.magic = CONFIG_MAGIC;
    config.programCount = 0;
    saveConfig();
  }
}

// Save configuration to EEPROM
void saveConfig() {
  EEPROM.put(CONFIG_ADDR, config);

  // Verify the save
  SliderConfig temp;
  EEPROM.get(CONFIG_ADDR, temp);
}

// Save a loop program (very efficient storage)
void saveLoopProgram(uint8_t programId, const char *name, LoopProgram program) {
  if (programId >= MAX_PROGRAMS) {
    Serial.println("ERROR: Program ID out of range");
    return;
  }

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);

  // Save program header
  ProgramHeader header;
  header.type = PROGRAM_TYPE_LOOP;
  header.cycles = program.cycles; // For loops, cycles field stores cycle count
  strncpy(header.name, name, 8);
  header.name[8] = '\0';
  EEPROM.put(addr, header);

  // Save loop parameters (very compact!)
  EEPROM.put(addr + sizeof(ProgramHeader), program);

  // Update program count if necessary
  if (programId >= config.programCount) {
    config.programCount = programId + 1;
    saveConfig();
  }
}

// Load a loop program
bool loadLoopProgram(uint8_t programId, LoopProgram *program) {
  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);

  // Check program type
  ProgramHeader header;
  EEPROM.get(addr, header);

  // Load loop parameters
  EEPROM.get(addr + sizeof(ProgramHeader), *program);
  return true;
}

// Get program type
uint8_t getProgramType(uint8_t programId) {
  if (programId >= MAX_PROGRAMS)
    return 255; // Invalid

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);
  ProgramHeader header;
  EEPROM.get(addr, header);
  return header.type;
}

// Load program name
void loadProgramName(uint8_t programId, char *name) {
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
void saveProgramName(uint8_t programId, const char *name) {
  if (programId >= MAX_PROGRAMS)
    return;

  int addr = PROGRAMS_ADDR + (programId * PROGRAM_SIZE);
  ProgramHeader header;
  EEPROM.get(addr, header);

  // Update name in header
  strncpy(header.name, name, 8);
  header.name[8] = '\0';
  EEPROM.put(addr, header);
}
