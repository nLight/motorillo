#include "command_processor.h"
#include "config_manager.h"
#include "motor_control.h"
#include "display_manager.h"

// Command codes for memory efficiency
enum CommandCode
{
  CMD_CONFIG = 1,
  CMD_POS = 2,
  CMD_RUN = 3,
  CMD_START = 4,
  CMD_STOP = 5,
  CMD_HOME = 6,
  // CMD_GET_CONFIG = 7, removed
  CMD_SETHOME = 8,
  CMD_LOOP_PROGRAM = 9,
  CMD_PROGRAM = 10,
  // CMD_GET_LOOP_PROGRAM = 11, // removed
  // CMD_GET_PROGRAM = 12, // removed
  CMD_GET_ALL_DATA = 13, // New: Bulk EEPROM load
  CMD_DEBUG_INFO = 14    // New: Debug info
};

// Process numeric command codes (binary format for maximum efficiency)
void processCommandCode(uint8_t cmdCode, char *data, int dataLen)
{
  displayMessage(cmdCode);
  switch (cmdCode)
  {
  case CMD_CONFIG:
  {
    // Binary format: totalSteps(2), speed(4), acceleration(1), microstepping(1)
    if (dataLen < 8)
      return;

    uint16_t totalSteps = *(uint16_t *)data;
    uint32_t speed = *(uint32_t *)(data + 2);
    uint8_t acceleration = *(uint8_t *)(data + 6);
    uint8_t microstepping = *(uint8_t *)(data + 7);

    config.totalSteps = totalSteps;
    config.speed = speed;
    config.acceleration = acceleration;

    if (microstepping == 1 || microstepping == 2 || microstepping == 4 ||
        microstepping == 8 || microstepping == 16)
    {
      config.microstepping = microstepping;
      setMicrostepping(config.microstepping);
    }

    saveConfig();
    displayMessage(F("OK"));
    break;
  }
  case CMD_POS:
  {
    // Binary format: position(2)
    uint16_t position = *(uint16_t *)data;
    displayMessage(F("Move"));
    moveToPositionWithSpeed(position, config.speed);
    break;
  }
  case CMD_RUN:
  {
    // Binary format: programId(1)
    uint8_t programId = *(uint8_t *)data;

    Serial.print(F("RUN command received for program "));
    Serial.println(programId);

    programPaused = false;
    displayMessage(F("Running"));

    uint8_t programType = getProgramType(programId);
    Serial.print(F("Program type: "));
    Serial.println(programType);

    if (programType == PROGRAM_TYPE_LOOP)
    {
      Serial.println(F("Running program"));
      runLoopProgram(programId);
      displayMessage(F("Done"));
    }
    else if (programType == PROGRAM_TYPE_COMPLEX)
    {
      Serial.println(F("Running program"));
      runProgram(programId);
      displayMessage(F("Done"));
    }
    else
    {
      Serial.println(F("Invalid Program"));
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
  case CMD_HOME:
    displayMessage(F("Home"));
    moveToPositionWithSpeed(0, config.speed);
    break;
  case CMD_SETHOME:
    displayMessage(F("Set Home"));
    currentPosition = 0;
    break;
  case CMD_LOOP_PROGRAM:
  {
    // Binary format: programId(1), name(8), steps(2), delayMs(4), cycles(1)
    uint8_t programId = *(uint8_t *)data;
    char programName[9];
    memcpy(programName, data + 1, 8);
    programName[8] = '\0';

    uint16_t steps = *(uint16_t *)(data + 9);
    uint32_t delayMs = *(uint32_t *)(data + 11);
    uint8_t cycles = *(uint8_t *)(data + 15);

    Serial.print(F("Parsed: ID="));
    Serial.print(programId);
    Serial.print(F(", Name="));
    Serial.print(programName);
    Serial.print(F(", Steps="));
    Serial.print(steps);
    Serial.print(F(", Delay="));
    Serial.print(delayMs);
    Serial.print(F(", Cycles="));
    Serial.println(cycles);

    LoopProgram loopProg;
    loopProg.steps = steps;
    loopProg.delayMs = delayMs;
    loopProg.cycles = cycles;

    saveLoopProgram(programId, programName, loopProg);
    Serial.println(F("Program saved"));
    displayMessage(F("Program Saved"));
    break;
  }
  case CMD_PROGRAM:
  {
    // Binary format: programId(1), name(8), stepCount(1), steps(stepCount*8)
    uint8_t programId = *(uint8_t *)data;
    char programName[9];
    memcpy(programName, data + 1, 8);
    programName[8] = '\0';

    uint8_t stepCount = *(uint8_t *)(data + 9);

    if (stepCount > MAX_STEPS_PER_PROGRAM || dataLen < 10 + stepCount * 8)
      return;

    MovementStep steps[MAX_STEPS_PER_PROGRAM];
    for (int i = 0; i < stepCount; i++)
    {
      uint8_t *stepData = (uint8_t *)(data + 10 + i * 8);
      steps[i].position = *(uint16_t *)stepData;
      steps[i].speed = *(uint32_t *)(stepData + 2);
      steps[i].pauseMs = *(uint16_t *)(stepData + 6);
    }

    saveComplexProgram(programId, programName, steps, stepCount);
    Serial.println(F("Program Saved"));
    displayMessage(F("Program Saved"));
    break;
  }
  case CMD_GET_ALL_DATA:
  {
    // Send all EEPROM data in binary format for bulk loading
    // Format: config(8) + programCount(1) + programs(variable)

    // Send config first
    Serial.write((uint8_t *)&config.totalSteps, 2);
    Serial.write((uint8_t *)&config.speed, 4);
    Serial.write((uint8_t *)&config.acceleration, 1);
    Serial.write((uint8_t *)&config.microstepping, 1);

    // Count valid programs
    uint8_t programCount = 0;
    for (uint8_t i = 0; i < MAX_PROGRAMS; i++)
    {
      uint8_t progType = getProgramType(i);
      if (progType == PROGRAM_TYPE_LOOP || progType == PROGRAM_TYPE_COMPLEX)
      {
        programCount++;
      }
    }

    Serial.write(programCount);

    // Send each program
    for (uint8_t i = 0; i < MAX_PROGRAMS; i++)
    {
      uint8_t programType = getProgramType(i);
      if (programType != PROGRAM_TYPE_LOOP && programType != PROGRAM_TYPE_COMPLEX)
        continue;

      // Program header: programId(1), type(1), name(8)
      Serial.write(i);
      Serial.write(programType);

      char name[9];
      loadProgramName(i, name);
      Serial.write(name, 8);

      if (programType == PROGRAM_TYPE_LOOP)
      {
        LoopProgram loopProg;
        if (loadLoopProgram(i, &loopProg))
        {
          // Loop program data: steps(2), delayMs(4), cycles(1)
          Serial.write((uint8_t *)&loopProg.steps, 2);
          Serial.write((uint8_t *)&loopProg.delayMs, 4);
          Serial.write((uint8_t *)&loopProg.cycles, 1);
        }
      }
      else if (programType == PROGRAM_TYPE_COMPLEX)
      {
        // Complex program data: stepCount(1) + steps(stepCount*8)
        MovementStep steps[MAX_STEPS_PER_PROGRAM];
        uint8_t stepCount = loadComplexProgram(i, steps);
        Serial.write(stepCount);

        for (int j = 0; j < stepCount; j++)
        {
          Serial.write((uint8_t *)&steps[j].position, 2);
          Serial.write((uint8_t *)&steps[j].speed, 4);
          Serial.write((uint8_t *)&steps[j].pauseMs, 2);
        }
      }
    }

    Serial.println(); // End with newline
    break;
  }
  }
}

// Process incoming commands from WebUSB/Serial
void processCommand(String command)
{
  // Check if this is a binary command (first char is command code < 20)
  // or text command (starts with a letter)
  if (command.length() > 0 && (uint8_t)command[0] < 20)
  {
    // Binary command
    int len = command.length();
    char cmd[64];
    command.toCharArray(cmd, sizeof(cmd));

    // First byte is command code
    uint8_t cmdCode = (uint8_t)cmd[0];
    processCommandCode(cmdCode, cmd + 1, len - 1);
  }
}
