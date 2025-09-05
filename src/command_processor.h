#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>

// External variables
extern bool programmingMode;
extern long currentPosition;
extern bool programRunning;

// Function declarations
void processCommand(String command);

#endif // COMMAND_PROCESSOR_H
