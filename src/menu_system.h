#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h>

// Menu system constants
const int MAX_MENU_ITEMS = 12;
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long LONG_PRESS_THRESHOLD = 1000; // 1 second for long press
const unsigned long SHORT_PRESS_THRESHOLD = 50;   // Minimum press time

// Menu item structure
struct MenuItem {
  char name[9];  // 8 characters + null terminator
  int type; // 0=program, 1=cycle, 2=settings
  int id;   // program ID or setting ID
};

// External variables (defined in main sketch)
extern MenuItem menuItems[MAX_MENU_ITEMS];
extern int menuItemCount;
extern int currentMenuIndex;
extern bool inMenuMode;
extern bool inPauseMenu;
extern int pauseMenuIndex;
extern bool programRunning;
extern bool programPaused;

// Button control variables
extern const int buttonPin;
extern bool lastButtonState;
extern bool buttonState;
extern unsigned long lastDebounceTime;
extern bool buttonPressed;
extern unsigned long buttonPressTime;
extern bool longPressDetected;

// Function declarations
void setupButton();
void checkButton();
void buildMenuItems();
void enterMenuMode();
void exitMenuMode();
void navigateMenu();
void selectMenuItem();
void enterPauseMenu();
void exitPauseMenu();
void navigatePauseMenu();
void selectPauseMenuItem();

#endif // MENU_SYSTEM_H
