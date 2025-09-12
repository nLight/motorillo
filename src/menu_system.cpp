#include "menu_system.h"
#include "config_manager.h"
#include "display_manager.h"

// Button pin definition
const int buttonPin = 9;

// Button control variables
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
bool buttonPressed = false;
unsigned long buttonPressTime = 0;
bool longPressDetected = false;

// Menu system variables
MenuItem menuItems[MAX_MENU_ITEMS];
int menuItemCount = 0;
int currentMenuIndex = 0;
bool inMenuMode = false;
bool inPauseMenu = false;
int pauseMenuIndex = 0;

// Setup button pin
void setupButton() { pinMode(buttonPin, INPUT_PULLUP); }

// Check button state (with debouncing)
void checkButton() {
  // Read button state
  int reading = digitalRead(buttonPin);

  // Check if button state changed (for debouncing)
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // If enough time has passed, consider it a valid state change
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      // Button pressed (LOW because of pull-up)
      if (buttonState == LOW) {
        buttonPressed = true;
        buttonPressTime = millis();
        longPressDetected = false;
      }
      // Button released
      else if (buttonPressed) {
        unsigned long pressDuration = millis() - buttonPressTime;
        buttonPressed = false;

        // Only process if minimum press time met
        if (pressDuration >= SHORT_PRESS_THRESHOLD) {
          if (pressDuration >= LONG_PRESS_THRESHOLD) {
            // Long press
            if (inPauseMenu) {
              selectPauseMenuItem();
            } else if (programRunning && !inMenuMode && !programPaused) {
              // Long press while program is actively running - enter pause menu
              enterPauseMenu();
            } else if (inMenuMode) {
              selectMenuItem();
            }
            // Note: No action for long press during pause
          } else {
            // Short press
            if (inPauseMenu) {
              navigatePauseMenu();
            } else if (inMenuMode) {
              navigateMenu();
            }
            // Note: No action for short press while program is running
            // Only long press can pause during execution
          }
        }
      }
    }
  }

  // Check for long press while button is still held
  if (buttonPressed && !longPressDetected &&
      (millis() - buttonPressTime) >= LONG_PRESS_THRESHOLD) {
    longPressDetected = true;
  }

  lastButtonState = reading;
}

// Build menu items based on stored programs
void buildMenuItems() {
  menuItemCount = 0;

  // Add stored programs
  for (int i = 0; i < config.programCount && i < MAX_PROGRAMS; i++) {
    loadProgramName(
        i, menuItems[menuItemCount].name); // Load directly into char array
    menuItems[menuItemCount].type = 0;
    menuItems[menuItemCount].id = i;
    menuItemCount++;
  }

  // Add settings/info item
  strcpy(menuItems[menuItemCount].name, "INFO");
  menuItems[menuItemCount].type = 2;
  menuItems[menuItemCount].id = 0;
  menuItemCount++;

  // Reset menu index if out of bounds
  if (currentMenuIndex >= menuItemCount) {
    currentMenuIndex = 0;
  }
}

// Enter menu mode
void enterMenuMode() {
  inMenuMode = true;
  currentMenuIndex = 0;
  buildMenuItems();
  displayMessage(F("MENU"), 200);
  updateDisplay();
}

// Exit menu mode
void exitMenuMode() {
  inMenuMode = false;
  updateDisplay();
}

// Navigate menu (short press)
void navigateMenu() {
  if (menuItemCount > 0) {
    currentMenuIndex = (currentMenuIndex + 1) % menuItemCount;
    updateDisplay();
  }
}

// Select menu item (long press)
void selectMenuItem() {
  if (menuItemCount == 0) {
    exitMenuMode();
    return;
  }

  MenuItem selectedItem = menuItems[currentMenuIndex];

  switch (selectedItem.type) {
  case 0: // Program
    exitMenuMode();
    displayMessage(F("RUN"), 200);
    programRunning = true;
    programPaused = false;
    break;

  case 2: // Info
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print(F("POS:"));
    display.print(currentPosition);

    display.display();
    delay(1500);
    enterMenuMode();
    break;
  }
}

// Enter pause menu
void enterPauseMenu() {
  inPauseMenu = true;
  pauseMenuIndex = 0;
  programPaused = true;
  displayMessage(F("PAUSE"), 200);
  updateDisplay();
}

// Exit pause menu
void exitPauseMenu() {
  inPauseMenu = false;
  updateDisplay();
}

// Navigate pause menu
void navigatePauseMenu() {
  pauseMenuIndex = (pauseMenuIndex + 1) % 2; // 2 items: RESUME, ABORT
  updateDisplay();
}

// Select pause menu item
void selectPauseMenuItem() {
  switch (pauseMenuIndex) {
  case 0:
    programPaused = false;
    displayMessage(F("RESUME"), 200);
    exitPauseMenu();
    break;

  case 1:
    programRunning = false;
    programPaused = false;
    displayMessage(F("ABORT"), 500);
    exitPauseMenu();
    enterMenuMode();
    break;
  }
}
