#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>

// External variables
extern bool programmingMode;
extern long currentPosition;
extern bool programRunning;

// Function declarations
void processCommand(String command);
void processCommandCode(uint8_t cmdCode, char *data, int dataLen);

#endif // COMMAND_PROCESSOR_H
