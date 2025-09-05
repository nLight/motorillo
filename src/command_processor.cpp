#include "command_processor.h"
#include "config_manager.h"
#include "motor_control.h"
#include "display_manager.h"

// External variables
extern bool programRunning;
extern long currentPosition;

// Process incoming commands from WebUSB/Serial
void processCommand(String command) {
  // Convert to char array for efficiency
  char cmd[64];
  command.toCharArray(cmd, sizeof(cmd));
  char* ptr = cmd;

  // Skip whitespace
  while (*ptr == ' ' || *ptr == '\t') ptr++;

  char c = *ptr;
  if (c == '\0') return; // Empty command

  if (c == 'C' && *(ptr+1) == 'F') { // CONFIG
    char* comma1 = strchr(ptr, ',');
    if (!comma1) return;
    char* comma2 = strchr(comma1 + 1, ',');
    if (!comma2) return;
    char* comma3 = strchr(comma2 + 1, ',');
    if (!comma3) return;

    config.totalSteps = atoi(comma1 + 1);
    config.speed = atoi(comma2 + 1);
    config.acceleration = atoi(comma3 + 1);

    char* comma4 = strchr(comma3 + 1, ',');
    if (comma4) {
      uint8_t newMicrostepping = atoi(comma4 + 1);
      if (newMicrostepping == 1 || newMicrostepping == 2 || newMicrostepping == 4 ||
          newMicrostepping == 8 || newMicrostepping == 16) {
        config.microstepping = newMicrostepping;
        setMicrostepping(config.microstepping);
      }
      char* comma5 = strchr(comma4 + 1, ',');
      if (comma5) {
        config.speedUnit = atoi(comma5 + 1);
      }
    }
    saveConfig();
    displayMessage(F("OK"));
  }
  else if (c == 'P' && *(ptr+1) == 'O') { // POS
    char* comma = strchr(ptr, ',');
    if (comma) {
      int position = atoi(comma + 1);
      displayMessage(F("Move"));
      moveToPosition(position);
    }
  }
  else if (c == 'R') { // RUN
    char* comma = strchr(ptr, ',');
    if (comma) {
      int programId = atoi(comma + 1);
      Serial.print(F("RUN command received for program "));
      Serial.println(programId);
      
      // Reset pause state before running
      programPaused = false;
      
      displayMessage(F("Running"));
      
      // Check program type and run appropriate function
      uint8_t programType = getProgramType(programId);
      Serial.print(F("Program type: "));
      Serial.println(programType);
      
      if (programType == PROGRAM_TYPE_LOOP) {
        Serial.println(F("Running loop program"));
        runLoopProgram(programId);
        displayMessage(F("Loop Done"));
      } else if (programType == PROGRAM_TYPE_COMPLEX) {
        Serial.println(F("Running complex program"));
        runProgram(programId);
        displayMessage(F("Program Done"));
      } else {
        Serial.println(F("Invalid program type"));
        displayMessage(F("Invalid Program"));
      }
    }
  }
  else if (c == 'S' && *(ptr+1) == 'T') {
    if (*(ptr+2) == 'A') { // START
      programRunning = true;
      displayMessage(F("Start"));
    } else if (*(ptr+2) == 'O') { // STOP
      programRunning = false;
      programPaused = false; // Also reset pause state
      displayMessage(F("Stop"));
    }
  }
  else if (c == 'H') {
    if (*(ptr+1) == 'O') { // HOME
      displayMessage(F("Home"));
      moveToPosition(0);
    }
  }
  else if (c == 'S' && *(ptr+1) == 'E' && *(ptr+2) == 'T' && *(ptr+3) == 'H') { // SETHOME
    displayMessage(F("Set Home"));
    currentPosition = 0;
  }
  else if (c == 'L' && *(ptr+1) == 'O' && *(ptr+2) == 'O' && *(ptr+3) == 'P') { // LOOP_PROGRAM
    Serial.println(F("LOOP_PROGRAM command received"));
    
    // Parse: LOOP_PROGRAM,slot,name,steps,delay,delayUnit,cycles
    char* comma1 = strchr(ptr, ',');
    if (!comma1) return;
    char* comma2 = strchr(comma1 + 1, ',');
    if (!comma2) return;
    char* comma3 = strchr(comma2 + 1, ',');
    if (!comma3) return;
    char* comma4 = strchr(comma3 + 1, ',');
    if (!comma4) return;
    char* comma5 = strchr(comma4 + 1, ',');
    if (!comma5) return;
    char* comma6 = strchr(comma5 + 1, ',');
    if (!comma6) return;

    uint8_t programId = atoi(comma1 + 1);
    char programName[9];
    strncpy(programName, comma2 + 1, 8);
    programName[8] = '\0';
    
    uint16_t steps = atoi(comma3 + 1);
    uint32_t delayMs = atoi(comma4 + 1);
    uint8_t delayUnit = atoi(comma5 + 1);
    uint8_t cycles = atoi(comma6 + 1);

    Serial.print(F("Parsed: ID="));
    Serial.print(programId);
    Serial.print(F(", Name="));
    Serial.print(programName);
    Serial.print(F(", Steps="));
    Serial.print(steps);
    Serial.print(F(", Delay="));
    Serial.print(delayMs);
    Serial.print(F(", Unit="));
    Serial.print(delayUnit);
    Serial.print(F(", Cycles="));
    Serial.println(cycles);

    // Convert delay to milliseconds if needed
    if (delayUnit == 0) { // microseconds
      delayMs = delayMs / 1000; // Convert to milliseconds
    }

    // Save the loop program
    LoopProgram loopProg;
    loopProg.steps = steps;
    loopProg.delayMs = delayMs;
    loopProg.cycles = cycles;
    loopProg.speedUnit = delayUnit;

    saveLoopProgram(programId, programName, steps, delayMs, cycles, delayUnit);
    Serial.println(F("Loop program saved"));
    displayMessage(F("Loop Saved"));
  }
}