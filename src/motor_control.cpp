#include "motor_control.h"
#include "config_manager.h"
#include "display_manager.h"
#include "menu_system.h"

// External variables (defined in main sketch)
extern long currentPosition;
extern bool programPaused;
extern bool programRunning;

// Setup motor control pins
void setupMotorPins() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  // pinMode(MS3_PIN, OUTPUT);
}

// Set microstepping mode using MS pins
void setMicrostepping(uint8_t mode) {
  // TODO: TMC2209 1/16 step
  digitalWrite(MS1_PIN, LOW);
  digitalWrite(MS2_PIN, LOW);
  // switch (mode)
  // {
  // case FULL_STEP: // Full step: L,L,L
  //   digitalWrite(MS1_PIN, LOW);
  //   digitalWrite(MS2_PIN, LOW);
  //   digitalWrite(MS3_PIN, LOW);
  //   break;
  // case HALF_STEP: // Half step: H,L,L
  //   digitalWrite(MS1_PIN, HIGH);
  //   digitalWrite(MS2_PIN, LOW);
  //   digitalWrite(MS3_PIN, LOW);
  //   break;
  // case QUARTER_STEP: // Quarter step: L,H,L
  //   digitalWrite(MS1_PIN, LOW);
  //   digitalWrite(MS2_PIN, HIGH);
  //   digitalWrite(MS3_PIN, LOW);
  //   break;
  // case EIGHTH_STEP: // Eighth step: H,H,L
  //   digitalWrite(MS1_PIN, HIGH);
  //   digitalWrite(MS2_PIN, HIGH);
  //   digitalWrite(MS3_PIN, LOW);
  //   break;
  // case SIXTEENTH_STEP: // Sixteenth step: H,H,H
  //   digitalWrite(MS1_PIN, HIGH);
  //   digitalWrite(MS2_PIN, HIGH);
  //   digitalWrite(MS3_PIN, HIGH);
  //   break;
  // default: // Default to full step for invalid values
  //   digitalWrite(MS1_PIN, LOW);
  //   digitalWrite(MS2_PIN, LOW);
  //   digitalWrite(MS3_PIN, LOW);
  //   break;
  // }
}

// Motor control functions

// Move to position with specified speed (in milliseconds)
void moveToPositionWithSpeed(long targetPosition, uint32_t speedMs) {
  long stepsToMove = abs(targetPosition - currentPosition);
  bool direction = targetPosition > currentPosition;

  // Adjust for microstepping - need more pulses but faster timing to maintain
  // same speed
  long actualStepsToMove = stepsToMove * DEFAULT_MICROSTEPPING;
  uint32_t adjustedSpeedMs = speedMs / DEFAULT_MICROSTEPPING;

  digitalWrite(DIR_PIN, direction ? HIGH : LOW);

  // Simplified acceleration (removed complex calculations to save space)
  for (long i = 0; i < actualStepsToMove; i++) {
    // Check for pause/stop request during movement
    if (programPaused || !programRunning) {
      currentPosition += direction ? (i / DEFAULT_MICROSTEPPING)
                                   : -(i / DEFAULT_MICROSTEPPING);
      return;
    }

    digitalWrite(STEP_PIN, HIGH);
    delay(adjustedSpeedMs);
    digitalWrite(STEP_PIN, LOW);
    delay(adjustedSpeedMs);

    updateDisplay(i);
    checkButton();
  }

  currentPosition = targetPosition;
}

// Run a loop program (infinite forward/backward motion)
void runLoopProgram(uint8_t programId) {
  LoopProgram loopProg;
  if (!loadLoopProgram(programId, &loopProg)) {
    Serial.println("ERROR: Failed to load loop program");
    return; // Failed to load loop program
  }

  // Run infinite cycles until stopped or paused
  while (programRunning && !programPaused) {
    // Forward movement
    long targetPosition = currentPosition + loopProg.steps;
    moveToPositionWithSpeed(targetPosition, loopProg.delayMs);

    // Check if we should stop/pause after forward movement
    if (!programRunning || programPaused) {
      break;
    }

    // Brief pause at end of forward movement
    delay(100);

    // Backward movement
    targetPosition = currentPosition - loopProg.steps;
    moveToPositionWithSpeed(targetPosition, loopProg.delayMs);

    // Check if we should stop/pause after backward movement
    if (!programRunning || programPaused) {
      break;
    }

    // Brief pause at end of backward movement
    delay(100);
  }

  if (programPaused) {
    Serial.println("Program paused");
  } else {
    Serial.println("Program stopped");
  }
}

// Execute stored program (called from main loop)
void executeStoredProgram() {
  // Don't execute if paused
  if (programPaused) {
    delay(100);
    return;
  }

  if (config.programCount > 0) {
    // Run the selected program from menu, or first program if no menu selection
    int programToRun = 0;

    // If we came from menu selection, use the selected program
    if (menuItemCount > 0 && currentMenuIndex < menuItemCount) {
      MenuItem selectedItem = menuItems[currentMenuIndex];
      if (selectedItem.type == 0) { // It's a program
        programToRun = selectedItem.id;
      }
    }

    // Check program type and run appropriate function
    uint8_t programType = getProgramType(programToRun);
    if (programType == PROGRAM_TYPE_LOOP) {
      runLoopProgram(programToRun);
    } else {
      Serial.println(F("ERROR: Invalid program type"));
      return;
    }
  } else {
    // No program stored, just idle
    delay(100);
  }
}
