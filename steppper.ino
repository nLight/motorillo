#include <WebUSB.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
// Define pin connections
const int dirPin = 10;
const int stepPin = 16;
const int buttonPin = 9;  // Start/Stop button

const int stepsPerRevolution = 200;

// Button control variables
bool programRunning = false;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Menu system variables
bool inMenuMode = false;
int currentMenuIndex = 0;
int menuItemCount = 0;
bool buttonPressed = false;
unsigned long buttonPressTime = 0;
const unsigned long longPressThreshold = 1000; // 1 second for long press
const unsigned long shortPressThreshold = 50;   // Minimum press time
bool longPressDetected = false;

// Pause menu system
bool inPauseMenu = false;
int pauseMenuIndex = 0;
const int pauseMenuItems = 2; // RESUME, ABORT
bool programPaused = false;

// Menu items structure
struct MenuItem {
  String name;
  int type; // 0=program, 1=cycle, 2=settings
  int id;   // program ID or setting ID
};

MenuItem menuItems[12]; // Max 10 programs + cycle + settings

// OLED display configuration
#define SCREEN_WIDTH 96
#define SCREEN_HEIGHT 16
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Display variables
unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 500; // Update every 500ms

// WebUSB connection detection
unsigned long bootTime = 0;
const unsigned long serialWaitTime = 3000; // Wait 3 seconds for WebUSB connection
bool serialCheckComplete = false;

// Configuration structure stored in EEPROM
struct SliderConfig {
  uint16_t magic;           // Magic number to validate EEPROM data
  uint16_t totalSteps;      // Total steps for full slider travel
  uint16_t speed;           // Delay between steps (microseconds)
  uint16_t acceleration;    // Acceleration steps
  uint8_t programCount;     // Number of stored programs
  bool cycleMode;           // Enable cycle mode instead of programs
  uint16_t cycleLength;     // Rail length in steps
  uint16_t cycleSpeed1;     // Speed going forward
  uint16_t cycleSpeed2;     // Speed going backward
  uint16_t cyclePause1;     // Pause at start position (ms)
  uint16_t cyclePause2;     // Pause at end position (ms)
  uint8_t reserved[1];      // Reserved for future use
};

// Movement program structure
struct MovementStep {
  uint16_t position;        // Target position in steps
  uint16_t speed;           // Speed for this movement
  uint16_t pauseMs;         // Pause after reaching position (ms)
};

const uint16_t CONFIG_MAGIC = 0xA5C3;
const int MAX_PROGRAMS = 10;
const int MAX_STEPS_PER_PROGRAM = 20;
const int CONFIG_ADDR = 0;
const int PROGRAMS_ADDR = sizeof(SliderConfig);

SliderConfig config;
long currentPosition = 0;
bool programmingMode = false;

// Function declarations
void updateDisplay();
void displayMessage(String message, int duration = 1000);
void playBootAnimation();
void buildMenuItems();
void enterMenuMode();
void exitMenuMode();
void navigateMenu();
void selectMenuItem();
void displayMenu();
void enterPauseMenu();
void exitPauseMenu();
void navigatePauseMenu();
void selectPauseMenuItem();
void displayPauseMenu();

// EEPROM functions
void loadConfig() {
  EEPROM.get(CONFIG_ADDR, config);
  if (config.magic != CONFIG_MAGIC) {
    // Initialize default configuration
    config.magic = CONFIG_MAGIC;
    config.totalSteps = 2000;
    config.speed = 1000;
    config.acceleration = 50;
    config.programCount = 0;
    config.cycleMode = false;
    config.cycleLength = 1000;
    config.cycleSpeed1 = 1000;
    config.cycleSpeed2 = 1000;
    config.cyclePause1 = 1000;
    config.cyclePause2 = 1000;
    saveConfig();
  }
}

void saveConfig() {
  EEPROM.put(CONFIG_ADDR, config);
}

void saveProgram(uint8_t programId, MovementStep* steps, uint8_t stepCount) {
  int addr = PROGRAMS_ADDR + (programId * (sizeof(uint8_t) + MAX_STEPS_PER_PROGRAM * sizeof(MovementStep)));
  EEPROM.put(addr, stepCount);
  
  // Save each step individually
  int stepAddr = addr + sizeof(uint8_t);
  for (int i = 0; i < stepCount; i++) {
    EEPROM.put(stepAddr + (i * sizeof(MovementStep)), steps[i]);
  }
}

uint8_t loadProgram(uint8_t programId, MovementStep* steps) {
  int addr = PROGRAMS_ADDR + (programId * (sizeof(uint8_t) + MAX_STEPS_PER_PROGRAM * sizeof(MovementStep)));
  uint8_t stepCount;
  EEPROM.get(addr, stepCount);
  if (stepCount > MAX_STEPS_PER_PROGRAM) return 0;
  
  // Load each step individually
  int stepAddr = addr + sizeof(uint8_t);
  for (int i = 0; i < stepCount; i++) {
    EEPROM.get(stepAddr + (i * sizeof(MovementStep)), steps[i]);
  }
  return stepCount;
}

void setup() {
  // Always start Serial for WebUSB
  Serial.begin(9600);
  bootTime = millis();
  programmingMode = false; // Start in standalone mode, will switch if WebUSB connects
  // Declare pins as Outputs
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  
  // Configure button pin with internal pull-up
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialize I2C and display
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Display failed to initialize, continue without display
     Serial.write("Display failed to initialize, continue without display\r\n> ");
  }
  
  // Play cute boot animation
  playBootAnimation();

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
        Serial.write(F("Slider Ready\r\n> "));
        Serial.flush();
      }
    } else if (millis() - bootTime >= serialWaitTime) {
      // Timeout reached, finalize mode
      serialCheckComplete = true;
    }
  }
  
  // Continue to check for WebUSB connection even after boot
  if (serialCheckComplete && !programmingMode && Serial) {
    programmingMode = true;
    Serial.write(F("Connected!\r\n> "));
    Serial.flush();
  }
  
  // Check button state (with debouncing)
  checkButton();
  
  // Update display periodically
  if (millis() - lastDisplayUpdate > displayUpdateInterval) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  if (programmingMode && Serial && Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  } else if (!programmingMode && programRunning) {
    executeStoredProgram();
  } else {
    // Program stopped, just idle
    delay(50);
  }
}

void processCommand(String command) {
  if (command.startsWith("CONFIG")) {
    // Format: CONFIG,totalSteps,speed,acceleration
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    int thirdComma = command.indexOf(',', secondComma + 1);
    
    if (firstComma > 0 && secondComma > 0 && thirdComma > 0) {
      config.totalSteps = command.substring(firstComma + 1, secondComma).toInt();
      config.speed = command.substring(secondComma + 1, thirdComma).toInt();
      config.acceleration = command.substring(thirdComma + 1).toInt();
      saveConfig();
      displayMessage("Config Updated");
      Serial.write(F("Config updated\r\n> "));
    }
  } 
  else if (command.startsWith("PROGRAM")) {
    // Format: PROGRAM,id,stepCount,pos1,speed1,pause1,pos2,speed2,pause2,...
    int commaIndex = command.indexOf(',');
    int programId = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
    commaIndex = command.indexOf(',', commaIndex + 1);
    int stepCount = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
    
    if (stepCount <= MAX_STEPS_PER_PROGRAM) {
      MovementStep steps[MAX_STEPS_PER_PROGRAM];
      commaIndex = command.indexOf(',', commaIndex + 1);
      
      for (int i = 0; i < stepCount; i++) {
        steps[i].position = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
        commaIndex = command.indexOf(',', commaIndex + 1);
        steps[i].speed = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
        commaIndex = command.indexOf(',', commaIndex + 1);
        steps[i].pauseMs = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
        commaIndex = command.indexOf(',', commaIndex + 1);
      }
      
      saveProgram(programId, steps, stepCount);
      config.programCount = max(config.programCount, programId + 1);
      saveConfig();
      buildMenuItems(); // Rebuild menu with new programs
      displayMessage(F("Program Saved"));
      Serial.write(F("Program saved\r\n> "));
    }
  }
  else if (command.startsWith("RUN")) {
    // Format: RUN,programId
    int programId = command.substring(4).toInt();
    displayMessage(F("Running Program"));
    runProgram(programId);
  }
  else if (command.startsWith(F("CYCLE"))) {
    // Format: CYCLE,length,speed1,speed2,pause1,pause2
    int commaIndex = command.indexOf(',');
    config.cycleLength = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
    commaIndex = command.indexOf(',', commaIndex + 1);
    config.cycleSpeed1 = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
    commaIndex = command.indexOf(',', commaIndex + 1);
    config.cycleSpeed2 = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
    commaIndex = command.indexOf(',', commaIndex + 1);
    config.cyclePause1 = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
    commaIndex = command.indexOf(',', commaIndex + 1);
    config.cyclePause2 = command.substring(commaIndex + 1).toInt();
    config.cycleMode = true;
    saveConfig();
    displayMessage(F("Cycle Mode Set"));
    Serial.write(F("Cycle mode configured\r\n> "));
  }
  else if (command.startsWith(F("POS"))) {
    // Format: POS,position
    int position = command.substring(4).toInt();
    displayMessage(F("Moving..."));
    moveToPosition(position);
    updateDisplay();
    Serial.write(F("Moved to position\r\n> "));
  }
  else if (command.startsWith(F("START"))) {
    programRunning = true;
    displayMessage(F("Started"));
    Serial.write(F("Program started\r\n> "));
  }
  else if (command.startsWith(F("STOP"))) {
    programRunning = false;
    displayMessage(F("Stopped"));
    Serial.write(F("Program stopped\r\n> "));
  }
  else if (command.startsWith(F("HOME"))) {
    displayMessage(F("Homing..."));
    moveToPosition(0);
    updateDisplay();
    Serial.write(F("Homed\r\n> "));
  }
  else {
    Serial.write(F("Unknown command\r\n> "));
  }
  Serial.flush();
}

void moveToPosition(long targetPosition) {
  moveToPositionWithSpeed(targetPosition, config.speed);
}

void moveToPositionWithSpeed(long targetPosition, uint16_t speed) {
  long stepsToMove = abs(targetPosition - currentPosition);
  bool direction = targetPosition > currentPosition;
  
  digitalWrite(dirPin, direction ? HIGH : LOW);
  
  // Apply acceleration/deceleration
  for (long i = 0; i < stepsToMove; i++) {
    // Check for pause request during movement
    checkButton();
    if (programPaused) {
      // Update current position to where we actually are
      currentPosition += direction ? i : -i;
      return; // Exit movement if paused
    }
    
    uint16_t stepDelay = speed;
    
    // Acceleration phase
    if (i < config.acceleration) {
      stepDelay += (config.acceleration - i) * 10;
    }
    // Deceleration phase
    else if (i > stepsToMove - config.acceleration) {
      stepDelay += (config.acceleration - (stepsToMove - i)) * 10;
    }
    
    digitalWrite(stepPin, HIGH);
    
    // Handle both short and long delays with button checking
    if (stepDelay <= 16383) {
      delayMicroseconds(stepDelay);
    } else {
      // For long delays, break into smaller chunks to check button
      unsigned long delayStart = millis();
      unsigned long totalDelayMs = stepDelay / 1000;
      while (millis() - delayStart < totalDelayMs) {
        checkButton();
        if (programPaused) {
          currentPosition += direction ? i : -i;
          return;
        }
        delay(10); // Small delay chunk
      }
      delayMicroseconds(stepDelay % 1000);  // Remainder in microseconds
    }
    
    digitalWrite(stepPin, LOW);
    
    // Same delay after LOW pulse with button checking
    if (stepDelay <= 16383) {
      delayMicroseconds(stepDelay);
    } else {
      unsigned long delayStart = millis();
      unsigned long totalDelayMs = stepDelay / 1000;
      while (millis() - delayStart < totalDelayMs) {
        checkButton();
        if (programPaused) {
          currentPosition += direction ? i : -i;
          return;
        }
        delay(10);
      }
      delayMicroseconds(stepDelay % 1000);
    }
  }
  
  currentPosition = targetPosition;
}

void runProgram(uint8_t programId) {
  MovementStep steps[MAX_STEPS_PER_PROGRAM];
  uint8_t stepCount = loadProgram(programId, steps);
  
  for (int i = 0; i < stepCount; i++) {
    // Check if we should pause before starting this step
    if (programPaused) {
      return; // Exit if paused
    }
    
    moveToPosition(steps[i].position);
    
    // Check if we got paused during movement
    if (programPaused) {
      return; // Exit if paused during movement
    }
    
    // Handle pause time with pause checking
    if (steps[i].pauseMs > 0) {
      unsigned long pauseStart = millis();
      while (millis() - pauseStart < steps[i].pauseMs) {
        checkButton();
        if (programPaused) {
          return; // Exit if paused during pause
        }
        delay(10); // Small delay to prevent tight loop
      }
    }
  }
}

void executeCycleMode() {
  static bool goingForward = true;
  
  if (programPaused) {
    return; // Don't execute if paused
  }
  
  if (goingForward) {
    // Move to end position
    moveToPositionWithSpeed(config.cycleLength, config.cycleSpeed1);
    if (programPaused) return; // Check if paused during movement
    
    if (config.cyclePause2 > 0) {
      unsigned long pauseStart = millis();
      while (millis() - pauseStart < config.cyclePause2) {
        checkButton();
        if (programPaused) return;
        delay(10);
      }
    }
    goingForward = false;
  } else {
    // Move back to start position
    moveToPositionWithSpeed(0, config.cycleSpeed2);
    if (programPaused) return; // Check if paused during movement
    
    if (config.cyclePause1 > 0) {
      unsigned long pauseStart = millis();
      while (millis() - pauseStart < config.cyclePause1) {
        checkButton();
        if (programPaused) return;
        delay(10);
      }
    }
    goingForward = true;
  }
}

void executeStoredProgram() {
  // Don't execute if paused
  if (programPaused) {
    delay(100);
    return;
  }
  
  if (config.cycleMode) {
    executeCycleMode();
  } else if (config.programCount > 0) {
    // Run the selected program from menu, or first program if no menu selection
    int programToRun = 0;
    
    // If we came from menu selection, use the selected program
    if (menuItemCount > 0 && currentMenuIndex < menuItemCount) {
      MenuItem selectedItem = menuItems[currentMenuIndex];
      if (selectedItem.type == 0) { // It's a program
        programToRun = selectedItem.id;
      }
    }
    
    runProgram(programToRun);
    delay(1000); // Brief pause between program cycles
  } else {
    // No program stored, just idle
    delay(100);
  }
}

void checkButton() {
  // Read button state
  int reading = digitalRead(buttonPin);
  
  // Check if button state changed (for debouncing)
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // If enough time has passed, consider it a valid state change
  if ((millis() - lastDebounceTime) > debounceDelay) {
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
        if (pressDuration >= shortPressThreshold) {
          if (pressDuration >= longPressThreshold) {
            // Long press
            if (programmingMode) {
              // In programming mode, just toggle like before
              programRunning = !programRunning;
              displayMessage(programRunning ? F("Started") : F("Stopped"), 500);
            } else {
              // In standalone mode
              if (inPauseMenu) {
                selectPauseMenuItem();
              } else if (programRunning && !inMenuMode) {
                // Long press while program is running - enter pause menu
                enterPauseMenu();
              } else if (inMenuMode) {
                selectMenuItem();
              } else {
                enterMenuMode();
              }
            }
          } else {
            // Short press
            if (programmingMode) {
              // In programming mode, just toggle like before
              programRunning = !programRunning;
              displayMessage(programRunning ? "Started" : "Stopped", 500);
            } else {
              // In standalone mode
              if (inPauseMenu) {
                navigatePauseMenu();
              } else if (inMenuMode) {
                navigateMenu();
              } else if (programRunning) {
                // Short press while running - stop the program
                programRunning = false;
                programPaused = false;
                displayMessage(F("Stopped"), 500);
              } else {
                // Not running, enter menu
                enterMenuMode();
              }
            }
          }
          
          // Log button action if WebUSB connected
          if (Serial) {
            String action = (pressDuration >= longPressThreshold) ? "Long" : "Short";
            Serial.write(("Button " + action + " press\r\n> ").c_str());
            Serial.flush();
          }
        }
      }
    }
  }
  
  // Check for long press while button is still held
  if (buttonPressed && !longPressDetected && (millis() - buttonPressTime) >= longPressThreshold) {
    longPressDetected = true;
    // Visual feedback for long press detection
    if (!programmingMode && programRunning) {
      displayMessage(F("PAUSE"), 200);
    }
  }
  
  lastButtonState = reading;
}

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
    display.print(F("PROG "));
    if (config.cycleMode) {
      display.print(F("CYCLE L:"));
      display.print(config.cycleLength);
    } else {
      display.print(F("STEPS P:"));
      display.print(config.programCount);
    }
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
    
    if (config.cycleMode) {
      display.print(F("CYCLE "));
      display.print(currentPosition);
      display.print(F("/"));
      display.print(config.cycleLength);
    } else {
      display.print(F("POS:"));
      display.print(currentPosition);
    }
  }

  display.display();
}

void displayMessage(String message, int duration = 1000) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 4);
  
  // Center text for single line messages
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 4);
  
  display.print(message);
  display.display();
  
  if (duration > 0) {
    delay(duration);
    updateDisplay();
  }
}

void playBootAnimation() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 4);
  display.print(F("READY ^.^"));
  display.display();
  delay(1000);
}

// Menu system functions
void buildMenuItems() {
  menuItemCount = 0;
  
  // Add stored programs
  for (int i = 0; i < config.programCount && i < MAX_PROGRAMS; i++) {
    menuItems[menuItemCount].name = "PGM" + String(i + 1);
    menuItems[menuItemCount].type = 0; // program type
    menuItems[menuItemCount].id = i;
    menuItemCount++;
  }
  
  // Add cycle mode if configured
  if (config.cycleMode) {
    menuItems[menuItemCount].name = "CYCLE";
    menuItems[menuItemCount].type = 1; // cycle type
    menuItems[menuItemCount].id = 0;
    menuItemCount++;
  }
  
  // Add settings/info item
  menuItems[menuItemCount].name = "INFO";
  menuItems[menuItemCount].type = 2; // settings type
  menuItems[menuItemCount].id = 0;
  menuItemCount++;
  
  // Reset menu index if out of bounds
  if (currentMenuIndex >= menuItemCount) {
    currentMenuIndex = 0;
  }
}

void enterMenuMode() {
  inMenuMode = true;
  currentMenuIndex = 0;
  buildMenuItems(); // Refresh menu items
  displayMessage(F("MENU"), 300);
  updateDisplay();
}

void exitMenuMode() {
  inMenuMode = false;
  updateDisplay();
}

void navigateMenu() {
  if (menuItemCount > 0) {
    currentMenuIndex = (currentMenuIndex + 1) % menuItemCount;
    updateDisplay();
  }
}

void selectMenuItem() {
  if (menuItemCount == 0) {
    exitMenuMode();
    return;
  }
  
  MenuItem selectedItem = menuItems[currentMenuIndex];
  
  switch (selectedItem.type) {
    case 0: // Program
      exitMenuMode();
      displayMessage(F("RUN"), 300);
      programRunning = true;
      programPaused = false; // Reset pause state
      // Set the selected program as the active one
      // We'll modify executeStoredProgram to use currentMenuIndex
      break;
      
    case 1: // Cycle
      exitMenuMode();
      displayMessage(F("RUN CYCLE"), 300);
      programRunning = true;
      programPaused = false; // Reset pause state
      break;
      
    case 2: // Info/Settings
      // Show system info
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("PGM:");
      display.print(config.programCount);
      display.print(" CYC:");
      display.print(config.cycleMode ? "Y" : "N");
      display.setCursor(0, 8);
      display.print("POS:");
      display.print(currentPosition);
      display.display();
      delay(2000);
      exitMenuMode();
      break;
  }
}

void displayMenu() {
  if (menuItemCount == 0) {
    display.setCursor(0, 4);
    display.print(F("NO PROGRAMS"));
    return;
  }
  
  // Show current menu item with selection indicator
  display.setCursor(0, 0);
  display.print(F(">"));
  display.print(menuItems[currentMenuIndex].name);
  
  // Show menu position indicator
  display.setCursor(0, 8);
  display.print(String(currentMenuIndex + 1));
  display.print(F("/"));
  display.print(menuItemCount);
  
  // Show navigation hints
  display.setCursor(50, 8);
  display.print(F("S:> L:OK"));
}

// Pause menu functions
void enterPauseMenu() {
  inPauseMenu = true;
  pauseMenuIndex = 0;
  programPaused = true;
  displayMessage(F("PAUSED"), 300);
  updateDisplay();
}

void exitPauseMenu() {
  inPauseMenu = false;
  updateDisplay();
}

void navigatePauseMenu() {
  pauseMenuIndex = (pauseMenuIndex + 1) % pauseMenuItems;
  updateDisplay();
}

void selectPauseMenuItem() {
  switch (pauseMenuIndex) {
    case 0: // RESUME
      programPaused = false;
      exitPauseMenu();
      displayMessage(F("RESUMED"), 300);
      break;
      
    case 1: // ABORT
      programRunning = false;
      programPaused = false;
      exitPauseMenu();
      enterMenuMode(); // Return to main menu
      displayMessage(F("ABORTED"), 300);
      break;
  }
}

void displayPauseMenu() {
  display.setCursor(0, 0);
  display.print(F("PAUSED"));
  
  // Show current pause menu item with selection indicator
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
  
  // Show menu position
  display.setCursor(60, 8);
  display.print(String(pauseMenuIndex + 1));
  display.print(F("/2"));
}
