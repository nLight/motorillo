#include "command_processor.h"
#include "config_manager.h"
#include "display_manager.h"
#include "motor_control.h"

// Command codes for memory efficiency
enum CommandCode {
  CMD_RUN = 3,
  CMD_START = 4,
  CMD_STOP = 5,
  CMD_SETHOME = 8,
  CMD_LOOP_PROGRAM = 9,
  CMD_DEBUG_INFO = 14, // Debug info
  CMD_POS_WITH_SPEED =
      15 // Position with custom speed (handles both move and home)
};

// Process numeric command codes (binary format for maximum efficiency)
void processCommandCode(uint8_t cmdCode, char *data, int dataLen) {
  switch (cmdCode) {
  case CMD_RUN: {
    // Binary format: programId(1)
    uint8_t programId = *(uint8_t *)data;

    programPaused = false;
    displayMessage(F("Running"));

    uint8_t programType = getProgramType(programId);

    if (programType == PROGRAM_TYPE_LOOP) {
      runLoopProgram(programId);
      displayMessage(F("Done"));
    } else {
      displayMessage(F("Invalid Program"));
    }
    break;
  }
  case CMD_START:
    programRunning = true;
    displayMessage(F("Start"));
    break;
  case CMD_STOP:
    programRunning = false;
    programPaused = false;
    displayMessage(F("Stop"));
    break;
  case CMD_SETHOME:
    displayMessage(F("Set Home"));
    currentPosition = 0;
    break;
  case CMD_LOOP_PROGRAM: {
    // Binary format: programId(1), name(8), steps(2), delayMs(4)
    // Note: cycles removed - programs now run infinitely
    uint8_t programId = *(uint8_t *)data;
    char programName[9];
    memcpy(programName, data + 1, 8);
    programName[8] = '\0';

    uint16_t steps = *(uint16_t *)(data + 9);
    uint32_t delayMs = *(uint32_t *)(data + 11);

    LoopProgram loopProg;
    loopProg.steps = steps;
    loopProg.delayMs = delayMs;

    saveLoopProgram(programId, programName, loopProg);
    displayMessage(F("Program Saved"));
    break;
  }
  case CMD_DEBUG_INFO: {
    // Simple ping response for connection checking
    Serial.println("PONG");
    break;
  }
  case CMD_POS_WITH_SPEED: {
    // Binary format: position(2), speed(4)
    uint16_t position = *(uint16_t *)data;
    uint32_t speedMs = *(uint32_t *)(data + 2);
    display.print(F("Move\n"));
    display.print("P: " + String(position) + ", S: " + speedMs);
    programRunning = true;
    moveToPositionWithSpeed(position, speedMs);
    break;
  }
  default:
    displayMessage(F("Unknown Cmd"));
    Serial.println("Unknown Command"); // End with newline
    break;
  }
}
