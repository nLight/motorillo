#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

#include "config_manager.h"

// OLED display configuration
const int SCREEN_WIDTH = 96;
const int SCREEN_HEIGHT = 16;
const int OLED_RESET = -1;
const int SCREEN_ADDRESS = 0x3C;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500; // Update every 500ms

// External variables
extern Adafruit_SSD1306 display;
extern unsigned long lastDisplayUpdate;
extern bool programmingMode;
extern long currentPosition;
extern bool programRunning;
extern bool programPaused;

// Function declarations
void setupDisplay();
void updateDisplay(long position = currentPosition);
void displayMessage(const __FlashStringHelper *message, int duration = 1000);
void displayMessage(const String message, int duration = 1000);
void playBootAnimation();
void displayMenu();
void displayPauseMenu();

#endif // DISPLAY_MANAGER_H
