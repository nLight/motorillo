#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "menu_system.h"
#include <Arduino.h>

// Pin definitions
const int MS1_PIN = 4;
const int MS2_PIN = 5;
const int MS3_PIN = 6;
const int STEP_PIN = 7;
const int DIR_PIN = 8;

// Microstepping modes
enum MicrostepMode {
  FULL_STEP = 1,
  HALF_STEP = 2,
  QUARTER_STEP = 4,
  EIGHTH_STEP = 8,
  SIXTEENTH_STEP = 16
};

// External variables from menu system
extern MenuItem menuItems[];
extern int menuItemCount;
extern int currentMenuIndex;

// Yield callback function pointer type
typedef void (*YieldCallback)();

// Function declarations
void setupMotorPins();
void setMicrostepping(uint8_t mode);
void setYieldCallback(YieldCallback callback); // Set yield callback
void yieldingDelay(
    uint32_t delayMs); // Non-blocking delay with callback yielding
void moveToPositionWithSpeed(long targetPosition, uint32_t speedMs);
void runProgram(uint8_t programId);
void runLoopProgram(uint8_t programId);
void executeStoredProgram();

#endif // MOTOR_CONTROL_H
