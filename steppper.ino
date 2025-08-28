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

// MS1	MS2	MS3	Microstep Resolution
// Low	Low	Low	Full step
// High	Low	Low	Half step
// Low	High	Low	Quarter step
// High	High	Low	Eighth step
// High	High	High	Sixteenth step
const int ms1Pin = 4;
const int ms2Pin = 5;
const int ms3Pin = 6;

const int stepPin = 7;
const int dirPin = 8;
const int buttonPin = 9;



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
  char name[9];  // 8 characters + null terminator
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
  uint8_t microstepping;    // Microstepping mode (1, 2, 4, 8, 16)
  uint8_t reserved[8];      // Reserved for future use
};

// Movement program structure
struct MovementStep {
  uint16_t position;        // Target position in steps
  uint16_t speed;           // Speed for this movement
  uint16_t pauseMs;         // Pause after reaching position (ms)
};

// Program name structure (8 characters max + null terminator)
struct ProgramName {
  char name[9];             // 8 characters + null terminator
};

const uint16_t CONFIG_MAGIC = 0xA5C3;
const int MAX_PROGRAMS = 10;
const int MAX_STEPS_PER_PROGRAM = 20;
const int CONFIG_ADDR = 0;
const int PROGRAMS_ADDR = sizeof(SliderConfig);
const int PROGRAM_NAMES_ADDR = PROGRAMS_ADDR + (MAX_PROGRAMS * (sizeof(uint8_t) + MAX_STEPS_PER_PROGRAM * sizeof(MovementStep)));

SliderConfig config;
long currentPosition = 0;
bool programmingMode = false;

// Function declarations
void updateDisplay();
void displayMessage(const __FlashStringHelper* message, int duration = 1000);
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
    config.microstepping = 1; // Full step by default
    saveConfig();
  }
  // Set microstepping pins based on stored config
  setMicrostepping(config.microstepping);
}

void saveConfig() {
  EEPROM.put(CONFIG_ADDR, config);
}

// Set microstepping mode using MS pins
void setMicrostepping(uint8_t mode) {
  switch(mode) {
    case 1:   // Full step: L,L,L
      digitalWrite(ms1Pin, LOW);
      digitalWrite(ms2Pin, LOW);
      digitalWrite(ms3Pin, LOW);
      break;
    case 2:   // Half step: H,L,L
      digitalWrite(ms1Pin, HIGH);
      digitalWrite(ms2Pin, LOW);
      digitalWrite(ms3Pin, LOW);
      break;
    case 4:   // Quarter step: L,H,L
      digitalWrite(ms1Pin, LOW);
      digitalWrite(ms2Pin, HIGH);
      digitalWrite(ms3Pin, LOW);
      break;
    case 8:   // Eighth step: H,H,L
      digitalWrite(ms1Pin, HIGH);
      digitalWrite(ms2Pin, HIGH);
      digitalWrite(ms3Pin, LOW);
      break;
    case 16:  // Sixteenth step: H,H,H
      digitalWrite(ms1Pin, HIGH);
      digitalWrite(ms2Pin, HIGH);
      digitalWrite(ms3Pin, HIGH);
      break;
    default:  // Default to full step for invalid values
      digitalWrite(ms1Pin, LOW);
      digitalWrite(ms2Pin, LOW);
      digitalWrite(ms3Pin, LOW);
      break;
  }
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

void saveProgramName(uint8_t programId, const char* name) {
  ProgramName programName;
  strncpy(programName.name, name, 8);
  programName.name[8] = '\0'; // Ensure null termination
  
  int addr = PROGRAM_NAMES_ADDR + (programId * sizeof(ProgramName));
  EEPROM.put(addr, programName);
}

void loadProgramName(uint8_t programId, char* name) {
  ProgramName programName;
  int addr = PROGRAM_NAMES_ADDR + (programId * sizeof(ProgramName));
  EEPROM.get(addr, programName);
  
  // Check if we have a valid name (not empty and has printable characters)
  if (programName.name[0] != '\0' && programName.name[0] >= 32 && programName.name[0] <= 126) {
    strcpy(name, programName.name);
  } else {
    // Generate default name if no custom name stored
    sprintf(name, "PGM%d", programId + 1);
  }
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
  
  // Configure microstepping pins as outputs
  pinMode(ms1Pin, OUTPUT);
  pinMode(ms2Pin, OUTPUT);
  pinMode(ms3Pin, OUTPUT);

  // Initialize I2C and display
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Display failed to initialize, continue without display
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
        Serial.write(F("Ready\r\n> "));
        Serial.flush();
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
    Serial.write(F("OK!\r\n> "));
    Serial.flush();
  }
  
  // Check button state (with debouncing)
  if(!programmingMode) {
    checkButton();
  }
  
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
  // Direct character comparison for speed and memory
  char c = command.charAt(0);
  
  if (c == 'C' && command.charAt(1) == 'F') { // CONFIG or CFG
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    int thirdComma = command.indexOf(',', secondComma + 1);
    int fourthComma = command.indexOf(',', thirdComma + 1);
    
    if (firstComma > 0 && secondComma > 0 && thirdComma > 0) {
      config.totalSteps = command.substring(firstComma + 1, secondComma).toInt();
      config.speed = command.substring(secondComma + 1, thirdComma).toInt();
      config.acceleration = command.substring(thirdComma + 1, fourthComma > 0 ? fourthComma : command.length()).toInt();
      
      // Optional microstepping parameter
      if (fourthComma > 0) {
        uint8_t newMicrostepping = command.substring(fourthComma + 1).toInt();
        if (newMicrostepping == 1 || newMicrostepping == 2 || newMicrostepping == 4 || 
            newMicrostepping == 8 || newMicrostepping == 16) {
          config.microstepping = newMicrostepping;
          setMicrostepping(config.microstepping);
        }
      }
      
      saveConfig();
      displayMessage(F("Config OK"));
    }
  }
  else if (c == 'C' && command.charAt(1) == 'Y') { // CYCLE - deprecated, use cycling programs instead
    displayMessage(F("Use CYCLE program"));
  }
  else if (c == 'P') { // PROGRAM or POS
    if (command.charAt(1) == 'R') { // PROGRAM
      int commaIndex = command.indexOf(',');
      int programId = command.substring(commaIndex + 1, command.indexOf(',', commaIndex + 1)).toInt();
      commaIndex = command.indexOf(',', commaIndex + 1);
      
      int nextCommaIndex = command.indexOf(',', commaIndex + 1);
      String programName = command.substring(commaIndex + 1, nextCommaIndex);
      commaIndex = nextCommaIndex;
      
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
        saveProgramName(programId, programName.c_str());
        config.programCount = max(config.programCount, programId + 1);
        saveConfig();
        buildMenuItems();
        displayMessage(F("Saved"));
      }
    }
    else if (command.charAt(1) == 'O') { // POS
      int position = command.substring(4).toInt();
      displayMessage(F("Move"));
      moveToPosition(position);
      updateDisplay();
    }
  }
  else if (c == 'R') { // RUN
    int programId = command.substring(4).toInt();
    displayMessage(F("Running"));
    runProgram(programId);
  }
  else if (c == 'S' && command.charAt(1) == 'T' && command.charAt(2) == 'A') { // START
    programRunning = true;
    displayMessage(F("Start"));
  }
  else if (c == 'S' && command.charAt(1) == 'T' && command.charAt(2) == 'O') { // STOP
    programRunning = false;
    displayMessage(F("Stop"));
  }
  else if (c == 'H') { // HOME
    displayMessage(F("Home"));
    moveToPosition(0);
    updateDisplay();
  }
  else {
    Serial.write(F("Err\r\n> "));
  }
  Serial.flush();
}

void moveToPosition(long targetPosition) {
  moveToPositionWithSpeed(targetPosition, config.speed);
}

void moveToPositionWithSpeed(long targetPosition, uint16_t speed) {
  long stepsToMove = abs(targetPosition - currentPosition);
  bool direction = targetPosition > currentPosition;
  
  // Adjust for microstepping - need more pulses but faster timing to maintain same speed
  long actualStepsToMove = stepsToMove * config.microstepping;
  uint16_t adjustedSpeed = speed / config.microstepping;
  
  digitalWrite(dirPin, direction ? HIGH : LOW);
  
  // Apply acceleration/deceleration
  for (long i = 0; i < actualStepsToMove; i++) {
    // Check for pause request during movement
    checkButton();
    if (programPaused) {
      // Update current position to where we actually are (convert back from microsteps)
      currentPosition += direction ? (i / config.microstepping) : -(i / config.microstepping);
      return; // Exit movement if paused
    }
    
    uint16_t stepDelay = adjustedSpeed;
    
    // Acceleration phase (adjusted for microstepping)
    long accelSteps = config.acceleration * config.microstepping;
    if (i < accelSteps) {
      stepDelay += ((accelSteps - i) * 10) / config.microstepping;
    }
    // Deceleration phase (adjusted for microstepping)  
    else if (i > actualStepsToMove - accelSteps) {
      stepDelay += ((accelSteps - (actualStepsToMove - i)) * 10) / config.microstepping;
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
          currentPosition += direction ? (i / config.microstepping) : -(i / config.microstepping);
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
  if (buttonPressed && !longPressDetected && (millis() - buttonPressTime) >= longPressThreshold) {
    longPressDetected = true;
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
    display.print(F("P:"));
    display.print(config.programCount);
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

void displayMessage(const __FlashStringHelper* message, int duration = 1000) {
  Serial.write(message);
  Serial.write(F("\r\n> "));
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

void playBootAnimation() {
  display.clearDisplay();
  
  // Draw camera sliding animation
  for (int x = -16; x <= SCREEN_WIDTH; x += 3) {
    display.clearDisplay();
    
    // Draw rail/track line
    display.drawLine(0, 12, SCREEN_WIDTH-1, 12, SSD1306_WHITE);
    display.drawLine(0, 13, SCREEN_WIDTH-1, 13, SSD1306_WHITE);
    
    // Draw cute camera icon sliding on the rail
    if (x >= 0 && x < SCREEN_WIDTH - 16) {
      // Camera body (rectangle with rounded corners effect)
      display.drawRect(x, 6, 14, 8, SSD1306_WHITE);
      display.drawRect(x+1, 7, 12, 6, SSD1306_WHITE);
      
      // Camera lens
      display.drawCircle(x+7, 10, 2, SSD1306_WHITE);
      display.drawPixel(x+7, 10, SSD1306_WHITE);
      
      // Camera viewfinder
      display.drawRect(x+2, 6, 3, 2, SSD1306_WHITE);
      
      // Flash
      display.drawPixel(x+11, 7, SSD1306_WHITE);
      display.drawPixel(x+12, 7, SSD1306_WHITE);
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
    display.drawLine(0, 12, SCREEN_WIDTH-1, 12, SSD1306_WHITE);
    display.drawLine(0, 13, SCREEN_WIDTH-1, 13, SSD1306_WHITE);
    
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

// Menu system functions
void buildMenuItems() {
  menuItemCount = 0;
  
  // Add stored programs
  for (int i = 0; i < config.programCount && i < MAX_PROGRAMS; i++) {
    loadProgramName(i, menuItems[menuItemCount].name); // Load directly into char array
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

void enterMenuMode() {
  inMenuMode = true;
  currentMenuIndex = 0;
  buildMenuItems();
  displayMessage(F("MENU"), 200);
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
      displayMessage(F("RUN"), 200);
      programRunning = true;
      programPaused = false;
      break;
      
    case 2: // Info
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      
      display.setCursor(0, 0);
      display.print(F("PGM:"));
      display.print(config.programCount);
      
      display.setCursor(0, 8);
      display.print(F("POS:"));
      display.print(currentPosition);
      
      display.display();
      delay(1500);
      exitMenuMode();
      break;
  }
}

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

// Pause menu functions
void enterPauseMenu() {
  inPauseMenu = true;
  pauseMenuIndex = 0;
  programPaused = true;
  displayMessage(F("PAUSE"), 200);
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
    case 0:
      programPaused = false;
      exitPauseMenu();
      displayMessage(F("RESUME"), 200);
      break;
      
    case 1:
      programRunning = false;
      programPaused = false;
      inPauseMenu = false;
      displayMessage(F("ABORT"), 500);
      break;
  }
}

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
