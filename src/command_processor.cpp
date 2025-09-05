#include "command_processor.h"
#include "config_manager.h"
#include "motor_control.h"
#include "display_manager.h"

// Process incoming commands from WebUSB/Serial
void processCommand(String command) {
  // Direct character comparison for speed and memory
  char c = command.charAt(0);

  if (c == 'C' && command.charAt(1) == 'F') { // CONFIG or CFG
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    int thirdComma = command.indexOf(',', secondComma + 1);
    int fourthComma = command.indexOf(',', thirdComma + 1);
    int fifthComma = command.indexOf(',', fourthComma + 1);

    if (firstComma > 0 && secondComma > 0 && thirdComma > 0) {
      config.totalSteps = command.substring(firstComma + 1, secondComma).toInt();
      config.speed = command.substring(secondComma + 1, thirdComma).toInt();
      config.acceleration = command.substring(thirdComma + 1, fourthComma > 0 ? fourthComma : command.length()).toInt();

      // Optional microstepping parameter
      if (fourthComma > 0) {
        uint8_t newMicrostepping = command.substring(fourthComma + 1, fifthComma > 0 ? fifthComma : command.length()).toInt();
        if (newMicrostepping == 1 || newMicrostepping == 2 || newMicrostepping == 4 ||
            newMicrostepping == 8 || newMicrostepping == 16) {
          config.microstepping = newMicrostepping;
          setMicrostepping(config.microstepping);
        }
      }

      // Optional speed unit parameter (0=microseconds, 1=seconds)
      if (fifthComma > 0) {
        uint8_t speedUnit = command.substring(fifthComma + 1).toInt();
        if (speedUnit == 0 || speedUnit == 1) {
          config.speedUnit = speedUnit;
        }
      } else {
        // Default to microseconds for backward compatibility
        config.speedUnit = 0;
      }

      saveConfig();
      displayMessage(F("Config OK"));
    }
  }
  else if (c == 'C' && command.charAt(1) == 'Y') { // CYCLE - deprecated, use cycling programs instead
    displayMessage(F("Use CYCLE program"));
  }
  else if (c == 'P') { // PROGRAM or POS
    if (command.charAt(1) == 'R') { // PROGRAM
      int commaIndex = command.indexOf(',');
      int programId = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
      commaIndex = command.indexOf(',', commaIndex + 1);

      int nextCommaIndex = command.indexOf(',', commaIndex + 1);
      String programName = command.substring(commaIndex + 1, nextCommaIndex);
      commaIndex = nextCommaIndex;

      int stepCount = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();

      if (stepCount <= MAX_STEPS_PER_PROGRAM) {
        MovementStep steps[MAX_STEPS_PER_PROGRAM];
        commaIndex = command.indexOf(',', commaIndex + 1);

        for (int i = 0; i < stepCount; i++) {
          steps[i].position = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
          commaIndex = command.indexOf(',', commaIndex + 1);
          steps[i].speed = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
          commaIndex = command.indexOf(',', commaIndex + 1);

          // Check if speed unit is provided (optional, defaults to microseconds)
          int nextCommaIndex = command.indexOf(',', commaIndex + 1);
          if (nextCommaIndex != -1 && command.substring(commaIndex + 1, nextCommaIndex).indexOf('.') == -1) {
            // No decimal point found, check if next parameter is speed unit
            String pauseStr = command.substring(commaIndex + 1, nextCommaIndex);
            String nextParam = command.substring(nextCommaIndex + 1, command.indexOf(',', nextCommaIndex + 1));

            // If next parameter is 0 or 1 (speed units), this is speed unit format
            if (nextParam == "0" || nextParam == "1") {
              steps[i].pauseMs = pauseStr.toInt();
              commaIndex = nextCommaIndex;
              steps[i].speedUnit = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
              commaIndex = command.indexOf(',', commaIndex + 1);
            } else {
              // Old format - pause only, default to microseconds
              steps[i].pauseMs = pauseStr.toInt();
              steps[i].speedUnit = 0; // microseconds
              commaIndex = nextCommaIndex;
            }
          } else {
            // Old format - pause only, default to microseconds
            steps[i].pauseMs = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
            steps[i].speedUnit = 0; // microseconds
            commaIndex = command.indexOf(',', commaIndex + 1);
          }
        }

        saveProgram(programId, steps, stepCount);
        saveProgramName(programId, programName.c_str());
        config.programCount = max(config.programCount, programId + 1);
        saveConfig();
        displayMessage(F("Saved"));
      }
    }
    else if (command.charAt(1) == 'O') { // POS
      int position = command.substring(4).toInt();
      displayMessage(F("Move"));
      moveToPosition(position);
      updateDisplay();
    }
  }
  else if (c == 'R') { // RUN
    int programId = command.substring(4).toInt();
    displayMessage(F("Running"));
    runProgram(programId);
  }
  else if (c == 'S' && command.charAt(1) == 'T' && command.charAt(2) == 'A') { // START
    programRunning = true;
    displayMessage(F("Start"));
  }
  else if (c == 'S' && command.charAt(1) == 'T' && command.charAt(2) == 'O') { // STOP
    programRunning = false;
    displayMessage(F("Stop"));
  }
  else if (c == 'H') { // HOME
    displayMessage(F("Home"));
    moveToPosition(0);
    updateDisplay();
  }
  else if (c == 'S' && command.charAt(1) == 'E' && command.charAt(2) == 'T' &&
           command.charAt(3) == 'H' && command.charAt(4) == 'O' && command.charAt(5) == 'M' && command.charAt(6) == 'E') { // SETHOME
    displayMessage(F("Set Home"));
    currentPosition = 0;
    updateDisplay();
  }
  else {
    // Error response would be sent here, but we don't have Serial access in this module
  }
}
