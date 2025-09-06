#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <WebUSB.h>
#include <Wire.h>

// Include our modular headers
#include "src/command_processor.h"
#include "src/config_manager.h"
#include "src/display_manager.h"
#include "src/menu_system.h"
#include "src/motor_control.h"

/**
 * Creating an instance of WebUSBSerial will add an additional USB interface to
 * the device that is marked as vendor-specific (rather than USB CDC-ACM) and
 * is therefore accessible to the browser.
 *
 * The URL here provides a hint to the browser about what page the user should
 * navigate to to interact with the device.
 */
WebUSB WebUSBSerial(1 /* https:// */, "localhost:8000");

#define Serial WebUSBSerial

// Global variables
long currentPosition = 0;
bool programmingMode = false;

// WebUSB connection detection
unsigned long bootTime = 0;
const unsigned long serialWaitTime =
    3000; // Wait 3 seconds for WebUSB connection
bool serialCheckComplete = false;

// Main program variables
bool programRunning = false;
bool programPaused = false;

void setup() {
  // Always start Serial for WebUSB
  Serial.begin(9600);
  bootTime = millis();
  programmingMode =
      false; // Start in standalone mode, will switch if WebUSB connects

  // Setup all modules
  setupMotorPins();
  setupButton();
  setupDisplay();

  loadConfig();

  // Build menu items based on stored programs
  buildMenuItems();

  // Update display with initial status
  updateDisplay();
}

void loop() {
  // Check for WebUSB connection during boot period
  if (!serialCheckComplete) {
    if (Serial && (millis() - bootTime < serialWaitTime)) {
      // WebUSB is available, switch to programming mode
      if (!programmingMode) {
        programmingMode = true;
      }
    } else if (millis() - bootTime >= serialWaitTime) {
      // Timeout reached, finalize mode
      serialCheckComplete = true;
      // If still in standalone mode, enter menu by default
      if (!programmingMode && !inMenuMode) {
        enterMenuMode();
      }
    }
  }

  // Continue to check for WebUSB connection even after boot
  if (serialCheckComplete && !programmingMode && Serial) {
    programmingMode = true;
    // Exit menu mode when switching to programming mode
    if (inMenuMode) {
      exitMenuMode();
    }
    Serial.write("Ok\n");
    Serial.flush();
  }

  // Check button state (with debouncing)
  if (!programmingMode) {
    checkButton();
  }

  // Update display periodically
  if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  if (programmingMode && Serial && Serial.available()) {
    // Check if this is a binary command (first byte < 20)
    uint8_t firstByte = Serial.peek();

    if (firstByte < 20) {
      // Binary command - read command code
      uint8_t cmdCode = Serial.read();

      // Read all available data as binary payload
      uint8_t data[64];
      int dataLen = 0;
      delay(1); // Small delay to ensure all data is available

      while (Serial.available() && dataLen < sizeof(data)) {
        data[dataLen++] = Serial.read();
      }

      // Process the binary command
      processCommandCode(cmdCode, (char *)data, dataLen);
    } else {
      // Text command - read until newline
      String command = Serial.readStringUntil('\n');
      command.trim();
      processCommand(command);
    }
  } else if (!programmingMode && programRunning) {
    executeStoredProgram();
  } else {
    // Program stopped, just idle
    delay(50);
  }
}
