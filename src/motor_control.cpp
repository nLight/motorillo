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

  // Apply acceleration/deceleration
  for (long i = 0; i < actualStepsToMove; i++) {
    // Check for pause request during movement
    if (programPaused) {
      // Update current position to where we actually are (convert back from microsteps)
      currentPosition += direction ? (i / config.microstepping) : -(i / config.microstepping);
      return; // Exit movement if paused
    }

    uint32_t stepDelay = adjustedSpeed;

    // Acceleration phase (adjusted for microstepping)
    long accelSteps = config.acceleration * config.microstepping;
    if (i < accelSteps) {
      stepDelay += ((accelSteps - i) * 10) / config.microstepping;
    }
    // Deceleration phase (adjusted for microstepping)
    else if (i > actualStepsToMove - accelSteps) {
      stepDelay += ((accelSteps - (actualStepsToMove - i)) * 10) / config.microstepping;
    }

    digitalWrite(STEP_PIN, HIGH);

    // Use longer HIGH pulse for more torque during slow movements
    uint32_t highPulseTime = stepDelay;
    if (stepDelay > 10000) { // For very slow movements, use longer pulse
      highPulseTime = min(stepDelay, (uint32_t)5000); // Cap at 5ms for safety
    }

    // Handle delays - break very long delays into chunks with button checking
    if (highPulseTime <= 16383) {
      delayMicroseconds(highPulseTime);
    } else {
      // For long delays, break into smaller chunks to check button
      unsigned long delayStart = millis();
      unsigned long totalDelayMs = highPulseTime / 1000;
      while (millis() - delayStart < totalDelayMs) {
        if (programPaused) {
          currentPosition += direction ? (i / config.microstepping) : -(i / config.microstepping);
          return;
        }
        delay(10); // Small delay chunk
      }
      delayMicroseconds(highPulseTime % 1000);  // Remainder in microseconds
    }

    digitalWrite(STEP_PIN, LOW);

    // Same delay after LOW pulse with button checking
    if (stepDelay <= 16383) {
      delayMicroseconds(stepDelay);
    } else {
      unsigned long delayStart = millis();
      unsigned long totalDelayMs = stepDelay / 1000;
      while (millis() - delayStart < totalDelayMs) {
        if (programPaused) {
          currentPosition += direction ? (i / config.microstepping) : -(i / config.microstepping);
          return;
        }
        delay(10);
      }
      delayMicroseconds(stepDelay % 1000);
    }
  }

  currentPosition = targetPosition;
}

// Run a stored program
void runProgram(uint8_t programId) {
  MovementStep steps[MAX_STEPS_PER_PROGRAM];
  uint8_t stepCount = loadProgram(programId, steps);

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

    // Check if this is a cycling program based on name
    char programName[9];
    loadProgramName(programToRun, programName);
    bool isCycling = (strncmp(programName, "CYCLE", 5) == 0) || (strncmp(programName, "LOOP", 4) == 0);

    runProgram(programToRun);

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
