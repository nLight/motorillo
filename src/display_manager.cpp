#include "display_manager.h"
#include "menu_system.h"

// OLED display instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Display variables
unsigned long lastDisplayUpdate = 0;

// Setup display
void setupDisplay() {
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Display failed to initialize, continue without display
  }

  // Play cute boot animation
  playBootAnimation();
}

// Update display periodically
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (inPauseMenu && !programmingMode) {
    // Pause menu display
    displayPauseMenu();
  } else if (inMenuMode && !programmingMode) {
    // Main menu display
    displayMenu();
  } else if (programmingMode) {
    // Programming mode display
    display.print(F("WebUSB"));
  } else {
    // Standalone mode display
    if (programRunning) {
      if (programPaused) {
        display.print(F("PAUSE "));
      } else {
        display.print(F("RUN "));
      }
    } else {
      display.print(F("STOP "));
    }

    display.print(F("POS:"));
    display.print(currentPosition);
  }

  display.display();
}

// Display a message on screen
void displayMessage(const __FlashStringHelper *message, int duration) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 4);
  display.print(message);
  display.display();

  if (duration > 0) {
    delay(duration);
    updateDisplay();
  }
}

// Play boot animation
void playBootAnimation() {
  display.clearDisplay();

  // Draw camera sliding animation
  for (int x = -16; x <= SCREEN_WIDTH; x += 3) {
    display.clearDisplay();

    // Draw rail/track line
    display.drawLine(0, 12, SCREEN_WIDTH - 1, 12, SSD1306_WHITE);
    display.drawLine(0, 13, SCREEN_WIDTH - 1, 13, SSD1306_WHITE);

    // Draw cute camera icon sliding on the rail
    if (x >= 0 && x < SCREEN_WIDTH - 16) {
      // Camera body (rectangle with rounded corners effect)
      display.drawRect(x, 6, 14, 8, SSD1306_WHITE);
      display.drawRect(x + 1, 7, 12, 6, SSD1306_WHITE);

      // Camera lens
      display.drawCircle(x + 7, 10, 2, SSD1306_WHITE);
      display.drawPixel(x + 7, 10, SSD1306_WHITE);

      // Camera viewfinder
      display.drawRect(x + 2, 6, 3, 2, SSD1306_WHITE);

      // Flash
      display.drawPixel(x + 11, 7, SSD1306_WHITE);
      display.drawPixel(x + 12, 7, SSD1306_WHITE);
    }

    // Add motion blur/trail effect
    if (x > 5) {
      for (int trail = 1; trail <= 3; trail++) {
        int trailX = x - trail * 4;
        if (trailX >= 0 && trailX < SCREEN_WIDTH - 16) {
          // Fading trail dots
          display.drawPixel(trailX + 7, 10, SSD1306_WHITE);
          if (trail <= 2) {
            display.drawPixel(trailX + 6, 10, SSD1306_WHITE);
            display.drawPixel(trailX + 8, 10, SSD1306_WHITE);
          }
        }
      }
    }

    display.display();
    delay(80);
  }

  // Final flourish - camera "flashes" and shows startup message
  for (int flash = 0; flash < 3; flash++) {
    display.clearDisplay();

    // Draw final rail
    display.drawLine(0, 12, SCREEN_WIDTH - 1, 12, SSD1306_WHITE);
    display.drawLine(0, 13, SCREEN_WIDTH - 1, 13, SSD1306_WHITE);

    if (flash % 2 == 0) {
      // Flash effect - invert screen briefly
      display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.display();
    delay(200);
  }

  delay(1000);
}

// Display main menu
void displayMenu() {
  if (menuItemCount == 0) {
    display.setCursor(0, 4);
    display.print(F("NO PGM"));
    return;
  }

  display.setCursor(0, 0);
  display.print(F(">"));
  display.print(menuItems[currentMenuIndex].name);

  display.setCursor(0, 8);
  display.print(currentMenuIndex + 1);
  display.print(F("/"));
  display.print(menuItemCount);

  display.setCursor(50, 8);
  display.print(F("S:> L:OK"));
}

// Display pause menu
void displayPauseMenu() {
  display.setCursor(0, 0);
  display.print(F("PAUSE"));

  display.setCursor(0, 8);
  display.print(F(">"));

  switch (pauseMenuIndex) {
  case 0:
    display.print(F("RESUME"));
    break;
  case 1:
    display.print(F("ABORT"));
    break;
  }

  display.setCursor(60, 8);
  display.print(pauseMenuIndex + 1);
  display.print(F("/2"));
}
