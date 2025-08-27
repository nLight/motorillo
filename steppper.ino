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
const int buttonPin = 2;  // Start/Stop button

const int stepsPerRevolution = 200;

// Button control variables
bool programRunning = false;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

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
        Serial.write("Slider Ready. Connect WebUSB to program.\r\n> ");
        Serial.flush();
        displayMessage("WebUSB Ready", 1000);
      }
    } else if (millis() - bootTime >= serialWaitTime) {
      // Timeout reached, finalize mode
      serialCheckComplete = true;
      if (!programmingMode) {
        displayMessage("Standalone Mode", 1000);
      }
    }
  }
  
  // Continue to check for WebUSB connection even after boot
  if (serialCheckComplete && !programmingMode && Serial) {
    programmingMode = true;
    Serial.write("WebUSB Connected!\r\n> ");
    Serial.flush();
    displayMessage("WebUSB Connected!", 1000);
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
      Serial.write("Config updated\r\n> ");
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
      displayMessage("Program Saved");
      Serial.write("Program saved\r\n> ");
    }
  }
  else if (command.startsWith("RUN")) {
    // Format: RUN,programId
    int programId = command.substring(4).toInt();
    displayMessage("Running Program");
    runProgram(programId);
  }
  else if (command.startsWith("CYCLE")) {
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
    displayMessage("Cycle Mode Set");
    Serial.write("Cycle mode configured\r\n> ");
  }
  else if (command.startsWith("POS")) {
    // Format: POS,position
    int position = command.substring(4).toInt();
    displayMessage("Moving...");
    moveToPosition(position);
    updateDisplay();
    Serial.write("Moved to position\r\n> ");
  }
  else if (command.startsWith("START")) {
    programRunning = true;
    displayMessage("Started");
    Serial.write("Program started\r\n> ");
  }
  else if (command.startsWith("STOP")) {
    programRunning = false;
    displayMessage("Stopped");
    Serial.write("Program stopped\r\n> ");
  }
  else if (command.startsWith("HOME")) {
    displayMessage("Homing...");
    moveToPosition(0);
    updateDisplay();
    Serial.write("Homed\r\n> ");
  }
  else {
    Serial.write("Unknown command\r\n> ");
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
    
    // Handle both short and long delays
    if (stepDelay <= 16383) {
      delayMicroseconds(stepDelay);
    } else {
      delay(stepDelay / 1000);  // Convert to milliseconds
      delayMicroseconds(stepDelay % 1000);  // Remainder in microseconds
    }
    
    digitalWrite(stepPin, LOW);
    
    // Same delay after LOW pulse
    if (stepDelay <= 16383) {
      delayMicroseconds(stepDelay);
    } else {
      delay(stepDelay / 1000);
      delayMicroseconds(stepDelay % 1000);
    }
  }
  
  currentPosition = targetPosition;
}

void runProgram(uint8_t programId) {
  MovementStep steps[MAX_STEPS_PER_PROGRAM];
  uint8_t stepCount = loadProgram(programId, steps);
  
  for (int i = 0; i < stepCount; i++) {
    moveToPosition(steps[i].position);
    if (steps[i].pauseMs > 0) {
      delay(steps[i].pauseMs);
    }
  }
}

void executeCycleMode() {
  static bool goingForward = true;
  
  if (goingForward) {
    // Move to end position
    moveToPositionWithSpeed(config.cycleLength, config.cycleSpeed1);
    if (config.cyclePause2 > 0) {
      delay(config.cyclePause2);
    }
    goingForward = false;
  } else {
    // Move back to start position
    moveToPositionWithSpeed(0, config.cycleSpeed2);
    if (config.cyclePause1 > 0) {
      delay(config.cyclePause1);
    }
    goingForward = true;
  }
}

void executeStoredProgram() {
  if (config.cycleMode) {
    executeCycleMode();
  } else if (config.programCount > 0) {
    // Run the first stored program in a loop
    runProgram(0);
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
        programRunning = !programRunning;
        
        if (programRunning) {
          displayMessage("Started", 500);
        } else {
          displayMessage("Stopped", 500);
        }
        
        if (Serial) {
          if (programRunning) {
            Serial.write("Button: Program started\r\n> ");
          } else {
            Serial.write("Button: Program stopped\r\n> ");
          }
          Serial.flush();
        }
      }
    }
  }
  
  lastButtonState = reading;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (programmingMode) {
    // Programming mode display
    display.print("PROG ");
    if (config.cycleMode) {
      display.print("CYCLE L:");
      display.print(config.cycleLength);
    } else {
      display.print("STEPS P:");
      display.print(config.programCount);
    }
  } else {
    // Standalone mode display
    if (programRunning) {
      display.print("RUN ");
    } else {
      display.print("STOP ");
    }
    
    if (config.cycleMode) {
      display.print("CYCLE ");
      display.print(currentPosition);
      display.print("/");
      display.print(config.cycleLength);
    } else {
      display.print("POS:");
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
    
    // Center the startup text
    display.setTextSize(1);
    display.setCursor(7, 4);
    display.print("SLIDER READY!");
    
    display.display();
    delay(200);
  }
  
  // Clear and show ready message
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 4);
  display.print("READY ^.^");
  display.display();
  delay(1000);
}
