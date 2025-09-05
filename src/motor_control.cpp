#include "motor_control.h"
#include "config_manager.h"
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
  pinMode(MS3_PIN, OUTPUT);
}

// Set microstepping mode using MS pins
void setMicrostepping(uint8_t mode) {
  switch(mode) {
    case FULL_STEP:   // Full step: L,L,L
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
      break;
    case HALF_STEP:   // Half step: H,L,L
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
      break;
    case QUARTER_STEP:  // Quarter step: L,H,L
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, LOW);
      break;
    case EIGHTH_STEP:  // Eighth step: H,H,L
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, LOW);
      break;
    case SIXTEENTH_STEP:  // Sixteenth step: H,H,H
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, HIGH);
      break;
    default:  // Default to full step for invalid values
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
      break;
  }
}

// Helper function to convert speed to microseconds
uint32_t convertSpeedToMicroseconds(uint32_t speedValue, uint8_t speedUnit) {
  if (speedUnit == 1) { // milliseconds
    // Convert milliseconds to microseconds (1 millisecond = 1,000 microseconds)
    return speedValue * 1000UL; // Use UL suffix for proper 32-bit math
  } else { // microseconds
    return speedValue;
  }
}

// Move to position using default speed
void moveToPosition(long targetPosition) {
  uint32_t speedInMicroseconds = convertSpeedToMicroseconds(config.speed, config.speedUnit);
  moveToPositionWithSpeed(targetPosition, speedInMicroseconds);
}

// Move to position with specified speed
void moveToPositionWithSpeed(long targetPosition, uint32_t speed) {
  long stepsToMove = abs(targetPosition - currentPosition);
  bool direction = targetPosition > currentPosition;

  // Adjust for microstepping - need more pulses but faster timing to maintain same speed
  long actualStepsToMove = stepsToMove * config.microstepping;
  uint32_t adjustedSpeed = speed / config.microstepping;

  digitalWrite(DIR_PIN, direction ? HIGH : LOW);

  // Simplified acceleration (removed complex calculations to save space)
  for (long i = 0; i < actualStepsToMove; i++) {
    // Check for pause request during movement
    if (programPaused) {
      currentPosition += direction ? (i / config.microstepping) : -(i / config.microstepping);
      return;
    }

    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(adjustedSpeed);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(adjustedSpeed);
  }

  currentPosition = targetPosition;
}

// Run a stored program (legacy complex program)
void runProgram(uint8_t programId) {
  MovementStep steps[MAX_STEPS_PER_PROGRAM];
  uint8_t stepCount = loadComplexProgram(programId, steps);

  for (int i = 0; i < stepCount; i++) {
    // Check if we should pause before starting this step
    if (programPaused) {
      return; // Exit if paused
    }

    // Convert step speed to microseconds and move
    uint32_t stepSpeedInMicroseconds = convertSpeedToMicroseconds(steps[i].speed, steps[i].speedUnit);
    moveToPositionWithSpeed(steps[i].position, stepSpeedInMicroseconds);

    // Check if we got paused during movement
    if (programPaused) {
      return; // Exit if paused during movement
    }

    // Handle pause time with pause checking
    if (steps[i].pauseMs > 0) {
      unsigned long pauseStart = millis();
      while (millis() - pauseStart < steps[i].pauseMs) {
        if (programPaused) {
          return; // Exit if paused during pause
        }
        delay(10); // Small delay to prevent tight loop
      }
    }
  }
}

// Run a loop program (forward/backward cycles)
void runLoopProgram(uint8_t programId) {
  LoopProgram loopProg;
  if (!loadLoopProgram(programId, &loopProg)) {
    Serial.println(F("ERROR: Failed to load loop program"));
    return; // Failed to load loop program
  }

  Serial.print(F("Starting loop program: "));
  Serial.print(loopProg.steps);
  Serial.print(F(" steps, "));
  Serial.print(loopProg.cycles);
  Serial.println(F(" cycles"));

  // Convert delay to microseconds for consistent timing
  uint32_t delayMicroseconds = convertSpeedToMicroseconds(loopProg.delayMs, 1); // Always milliseconds for loops

  for (int cycle = 0; cycle < loopProg.cycles; cycle++) {
    // Check if we should pause before starting this cycle
    if (programPaused) {
      Serial.println(F("Program paused"));
      return; // Exit if paused
    }

    Serial.print(F("Cycle "));
    Serial.print(cycle + 1);
    Serial.print(F("/"));
    Serial.print(loopProg.cycles);
    Serial.println(F(" - Forward"));

    // Move forward
    long targetPosition = currentPosition + loopProg.steps;
    moveToPositionWithSpeed(targetPosition, delayMicroseconds);

    // Check if we got paused during forward movement
    if (programPaused) {
      Serial.println(F("Program paused during forward movement"));
      return; // Exit if paused during movement
    }

    // Brief pause at end of forward movement
    delay(100);

    Serial.print(F("Cycle "));
    Serial.print(cycle + 1);
    Serial.println(F(" - Backward"));

    // Move backward
    targetPosition = currentPosition - loopProg.steps;
    moveToPositionWithSpeed(targetPosition, delayMicroseconds);

    // Check if we got paused during backward movement
    if (programPaused) {
      Serial.println(F("Program paused during backward movement"));
      return; // Exit if paused during movement
    }

    // Brief pause at end of backward movement
    delay(100);
  }

  Serial.println(F("Loop program completed"));
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
    } else if (programType == PROGRAM_TYPE_COMPLEX) {
      runProgram(programToRun);
    }

    // Check if this is a cycling program based on name
    char programName[9];
    loadProgramName(programToRun, programName);
    bool isCycling = (strncmp(programName, "CYCLE", 5) == 0) || (strncmp(programName, "LOOP", 4) == 0);

    // If it's a cycling program, restart it immediately
    if (isCycling) {
      delay(500); // Brief pause between cycles
    } else {
      delay(1000); // Brief pause between program cycles
    }
  } else {
    // No program stored, just idle
    delay(100);
  }
}
