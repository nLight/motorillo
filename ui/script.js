class SliderController {
  constructor() {
    this.port = null;
    this.connected = false;

    // Command codes for memory efficiency
    this.CMD_RUN = 3;
    this.CMD_START = 4;
    this.CMD_STOP = 5;
    this.CMD_SETHOME = 8;
    this.CMD_LOOP_PROGRAM = 9;
    this.CMD_DEBUG_INFO = 14; // Request debug information
    this.CMD_POS_WITH_SPEED = 15; // Position with custom speed (handles both move and home)

    this.init();
  }

  init() {
    // Check WebUSB support
    if (!navigator.usb) {
      this.log("WebUSB not supported in this browser. Use Chrome/Edge.");
      return;
    }

    // Set up event listeners
    this.setupEventListeners();

    // Try to reconnect to previously connected device
    this.autoConnect();
  }

  setupEventListeners() {
    // Connection
    document
      .getElementById("connectBtn")
      .addEventListener("click", () => this.connect());

    // Tabs
    document.querySelectorAll(".tab-btn").forEach((btn) => {
      btn.addEventListener("click", (e) =>
        this.switchTab(e.target.dataset.tab)
      );
    });

    // Manual controls
    document
      .getElementById("startBtn")
      .addEventListener("click", () => this.sendCommand(this.CMD_START));
    document
      .getElementById("stopBtn")
      .addEventListener("click", () => this.sendCommand(this.CMD_STOP));
    document
      .getElementById("setHomeBtn")
      .addEventListener("click", () => this.sendCommand(this.CMD_SETHOME));
    document
      .getElementById("homeBtn")
      .addEventListener("click", () => this.handleHome());
    document
      .getElementById("moveBtn")
      .addEventListener("click", () => this.handleMove());

    // Program builder - loop programs only
    document
      .getElementById("saveProgram")
      .addEventListener("click", () => this.saveProgram());
    document
      .getElementById("testProgram")
      .addEventListener("click", () => this.testProgram());

    // Initialize program storage - loop programs only
    this.programNames = {}; // Store program names locally
    this.loopPrograms = {}; // Store loop programs locally

    // Load saved program names from localStorage
    this.loadProgramNames();

    // Set up program slot change handler
    document.getElementById("programSlot").addEventListener("change", () => {
      this.loadProgramNameFromStorage();
    });

    // Load initial program data for the currently selected slot
    this.loadProgramNameFromStorage();

    // Load saved manual speed
    this.loadManualSpeed();

    // Save manual speed when changed
    document.getElementById("manualSpeed").addEventListener("change", () => {
      this.saveManualSpeed();
    });
  }

  async autoConnect() {
    try {
      const ports = await serial.getPorts();
      if (ports.length > 0) {
        await this.connectToPort(ports[0]);
      }
    } catch (error) {
      console.log("Auto-connect failed:", error);
    }
  }

  async connect() {
    try {
      if (this.connected && this.port) {
        // Disconnect if already connected
        await this.disconnect();
        return;
      }

      this.port = await serial.requestPort();
      await this.connectToPort(this.port);
    } catch (error) {
      this.log("Connection failed: " + error.message);
    }
  }

  async connectToPort(port) {
    try {
      await port.connect();
      this.port = port;
      this.connected = true;

      // Set up data listener
      this.setupDataListener();

      this.updateConnectionStatus(true);
      this.log("Connected to slider controller");

      // EEPROM data will be sent automatically by Arduino on connection
      // No need to request it
    } catch (error) {
      this.log(`Connection failed: ${error.message}`);
    }
  }

  async disconnect() {
    // Clear connection check interval
    if (this.connectionCheckInterval) {
      clearInterval(this.connectionCheckInterval);
      this.connectionCheckInterval = null;
    }

    if (this.port) {
      try {
        await this.port.disconnect();
        this.log("Disconnected from slider");
      } catch (error) {
        this.log("Disconnect error: " + error.message);
      }
    }
    this.port = null;
    this.connected = false;
    this.updateConnectionStatus(false);
  }

  handleDisconnection(reason) {
    if (this.connected) {
      this.log(`Connection lost: ${reason}`);
      this.connected = false;
      this.port = null;
      this.updateConnectionStatus(false);

      // Clear connection check interval
      if (this.connectionCheckInterval) {
        clearInterval(this.connectionCheckInterval);
        this.connectionCheckInterval = null;
      }
    }
  }

  async checkConnection() {
    if (!this.connected || !this.port) {
      return;
    }

    try {
      // Send a simple ping command to check if connection is alive
      // Use CMD_DEBUG_INFO as a lightweight ping
      await this.port.send(new Uint8Array([this.CMD_DEBUG_INFO]));
    } catch (error) {
      console.log("Connection check failed:", error);
      this.handleDisconnection("Connection check failed");
    }
  }

  setupDataListener() {
    // WebUSB serial interface data reception
    this.port.onReceive = (data) => {
      console.log("Received data:", data);
      this.handleIncomingData(data);
    };

    this.port.onReceiveError = (error) => {
      console.error("Receive error:", error);
      this.handleDisconnection("Receive error: " + error.message);
    };

    // Handle WebUSB disconnection events
    this.port.onDisconnect = () => {
      console.log("WebUSB disconnected");
      this.handleDisconnection("Device disconnected");
    };

    // Set up periodic connection check
    this.connectionCheckInterval = setInterval(() => {
      this.checkConnection();
    }, 2000); // Check every 2 seconds
  }

  handleIncomingData(data) {
    // Handle WebUSB serial interface data (DataView/ArrayBuffer)
    let textData;
    let isBinary = false;

    if (data instanceof DataView || data instanceof ArrayBuffer) {
      // Convert to Uint8Array for processing
      const uint8Data = new Uint8Array(
        data instanceof ArrayBuffer ? data : data.buffer
      );

      // Improved binary detection for bulk EEPROM data:
      // 1. Must be substantial size (bulk data is much larger than text messages)
      // 2. First byte should be reasonable program count (0-10)
      // 3. Should not be printable text throughout
      const firstByte = uint8Data[0];

      if (uint8Data.length > 20) {
        // Check if this could be bulk EEPROM data
        // Program count should be reasonable (0-10)
        const seemsLikeProgramCount = firstByte >= 0 && firstByte <= 10;

        // Check if it contains mostly non-printable or structured binary data
        // Look at a few bytes to see if it has the binary structure we expect
        let nonPrintableCount = 0;
        const sampleSize = Math.min(20, uint8Data.length);
        for (let i = 0; i < sampleSize; i++) {
          if (uint8Data[i] < 32 && uint8Data[i] !== 10 && uint8Data[i] !== 13) {
            nonPrintableCount++;
          }
        }

        // If it starts with reasonable program count and has binary characteristics
        if (
          seemsLikeProgramCount &&
          (nonPrintableCount > sampleSize * 0.3 || uint8Data.length > 100)
        ) {
          isBinary = true;
          this.handleBinaryData(uint8Data);
          return;
        }
      }

      // Convert to string for text processing
      const decoder = new TextDecoder();
      textData = decoder.decode(uint8Data);
    } else {
      textData = data;
    }

    // Process text data line by line
    const lines = textData.split("\n");
    for (const line of lines) {
      if (line.trim()) {
        this.log(line.trim());
        this.processTextData(line.trim());
      }
    }
  }

  processTextData(data) {
    // Handle program count
    if (data.startsWith("PROGRAMS:")) {
      const count = parseInt(data.substring(9));
      console.log(`Expecting ${count} programs`);
      // Clear existing programs
      this.loopPrograms = {};
      this.programNames = {};
      return;
    }

    // Handle individual programs: PROG:id,name,steps,delayMs
    // Note: cycles removed - programs now run infinitely
    if (data.startsWith("PROG:")) {
      const parts = data.substring(5).split(",");
      if (parts.length >= 4) {
        const programId = parseInt(parts[0]);
        const name = parts[1].trim();
        const steps = parseInt(parts[2]);
        const delayMs = parseInt(parts[3]);
        // cycles removed - programs run infinitely

        console.log(
          `Loaded program ${programId}: "${name}" (${steps}steps, ${delayMs}ms, infinite)`
        );

        this.loopPrograms[programId] = { steps, delay: delayMs };
        this.programNames[programId] = name;

        this.log(
          `Loaded Infinite Loop Program "${name}" from EEPROM (slot ${
            programId + 1
          })`
        );

        // Update the UI if this is the currently selected program
        const currentSlot = parseInt(
          document.getElementById("programSlot").value
        );
        if (programId === currentSlot) {
          document.getElementById("programName").value = name;
          document.getElementById("loopSteps").value = steps;
          document.getElementById("loopDelay").value = delayMs;
        }

        // Save to localStorage after each program
        this.saveProgramNames();
        localStorage.setItem(
          "sliderLoopPrograms",
          JSON.stringify(this.loopPrograms)
        );
      }
      return;
    }

    // Handle program execution messages
    if (
      data.includes("Starting loop program") ||
      data.includes("Cycle") ||
      data.includes("Loop program completed") ||
      data.includes("RUN command received") ||
      data.includes("Program type") ||
      data.includes("Running loop program") ||
      data.includes("ERROR") ||
      data.includes("WARNING") ||
      data.includes("Total time per direction")
    ) {
      this.log(`[Arduino] ${data}`);
    }
  }

  handleBinaryData(data) {
    if (data.length < 2) return;

    // data is already a Uint8Array, no conversion needed
    const bytes = data;

    // Check if this is bulk data (much longer than individual programs)
    if (data.length > 50) {
      this.handleBulkData(bytes);
      return;
    }

    const programId = bytes[0];
    this.log(`Received binary data for program ${programId}`);

    // Check if this is loop program data (15 bytes expected for loop programs without cycles)
    if (data.length === 15) {
      // 15 bytes (no cycles)
      this.handleLoopProgramBinary(bytes);
    }
  }

  handleLoopProgramBinary(bytes) {
    const programId = bytes[0];

    // Extract name (bytes 1-8)
    let name = "";
    for (let i = 1; i <= 8; i++) {
      if (bytes[i] !== 0 && bytes[i] !== 32) {
        name += String.fromCharCode(bytes[i]);
      }
    }
    name = name.trim() || `PGM${programId + 1}`;

    // Extract loop data (bytes 9-14, cycles removed)
    const view = new DataView(bytes.buffer, 9);
    const steps = view.getUint16(0, true);
    const delayMs = view.getUint32(2, true);
    // cycles removed - programs run infinitely

    // Store in local storage
    this.loopPrograms[programId] = { steps, delay: delayMs };
    this.programNames[programId] = name;
    this.saveProgramNames();

    // Load into UI
    document.getElementById("programName").value = name;
    document.getElementById("loopSteps").value = steps;
    document.getElementById("loopDelay").value = delayMs;
    // cycles field removed from UI

    this.log(
      `Loaded Infinite Loop Program "${name}" from EEPROM (slot ${
        programId + 1
      })`
    );
  }

  handleBulkData(bytes) {
    this.log("Received bulk EEPROM data");

    let offset = 0;

    // Validate minimum size
    if (bytes.length < 1) {
      this.log("ERROR: Bulk data too small");
      return;
    }

    // Parse program count (first byte)
    const programCount = bytes[offset];
    offset += 1;

    this.log(`Loading ${programCount} programs from EEPROM`);

    // Sanity check on program count
    if (programCount > 10) {
      this.log(`ERROR: Invalid program count ${programCount}, aborting parse`);
      return;
    }

    // Clear existing programs
    this.loopPrograms = {};
    this.programNames = {};

    // Parse each program (only loop programs)
    for (let i = 0; i < programCount; i++) {
      // Check if we have enough bytes left
      if (offset + 10 > bytes.length) {
        this.log(`ERROR: Not enough data for program ${i}, stopping parse`);
        break;
      }

      const programId = bytes[offset];
      const programType = bytes[offset + 1];

      // Extract name (bytes offset+2 to offset+9)
      let name = "";
      for (let j = 0; j < 8; j++) {
        const charCode = bytes[offset + 2 + j];
        if (charCode !== 0 && charCode !== 32) {
          name += String.fromCharCode(charCode);
        }
      }
      name = name.trim() || `PGM${programId + 1}`;

      this.programNames[programId] = name;

      offset += 10; // programId(1) + type(1) + name(8)

      if (programType === 0) {
        // Loop program - need 6 more bytes (removed cycles)
        if (offset + 6 > bytes.length) {
          this.log(`ERROR: Not enough data for loop program ${programId}`);
          break;
        }

        const loopView = new DataView(bytes.buffer, offset);
        const steps = loopView.getUint16(0, true);
        const delayMs = loopView.getUint32(2, true);
        // cycles removed - programs run infinitely

        this.loopPrograms[programId] = { steps, delay: delayMs };
        offset += 6; // steps(2) + delayMs(4)

        this.log(
          `Loaded Infinite Loop Program "${name}" (slot ${programId + 1})`
        );
      } else {
        this.log(
          `WARNING: Skipping unsupported program type ${programType} for "${name}"`
        );
        // For unknown program types, we can't safely skip them as we don't know the size
        this.log("ERROR: Cannot continue parsing due to unknown program type");
        break;
      }
    }

    // Save to localStorage
    this.saveProgramNames();
    localStorage.setItem(
      "sliderLoopPrograms",
      JSON.stringify(this.loopPrograms)
    );

    this.log("Bulk EEPROM loading complete");
  }

  async sendCommand(command, binaryData = null) {
    if (!this.connected || !this.port) {
      this.log("Not connected to slider");
      return false;
    }

    try {
      let data;

      if (binaryData) {
        // Binary command: command code + binary data
        const buffer = new Uint8Array(1 + binaryData.length);
        buffer[0] = command;
        buffer.set(binaryData, 1);
        data = buffer;
        this.log(`Sent binary: CMD=${command}, ${binaryData.length} bytes`);
      } else if (typeof command === "number") {
        // Simple command without data
        data = new Uint8Array([command]);
        this.log(`Sent: CMD=${command}`);
      } else {
        // Legacy string command
        const encoder = new TextEncoder();
        data = encoder.encode(command + "\n");
        this.log(`Sent: ${command}`);
      }

      await this.port.send(data);
      return true;
    } catch (error) {
      this.log("Send error: " + error.message);
      this.handleDisconnection("Send failed: " + error.message);
      return false;
    }
  }

  switchTab(tabName) {
    // Update tab buttons
    document
      .querySelectorAll(".tab-btn")
      .forEach((btn) => btn.classList.remove("active"));
    document.querySelector(`[data-tab="${tabName}"]`).classList.add("active");

    // Update tab content
    document
      .querySelectorAll(".tab-content")
      .forEach((content) => content.classList.remove("active"));
    document.getElementById(tabName).classList.add("active");
  }

  handleMove() {
    const position = parseInt(document.getElementById("targetPosition").value);
    const speed = parseInt(document.getElementById("manualSpeed").value);

    // Validate inputs
    if (isNaN(position)) {
      this.log("Error: Invalid position value");
      return;
    }
    if (isNaN(speed) || speed < 1 || speed > 4294967295) {
      this.log("Error: Speed must be between 1 and 4,294,967,295 milliseconds");
      return;
    }

    // Binary format: position(2), speed(4)
    const buffer = new ArrayBuffer(6);
    const view = new DataView(buffer);
    view.setUint16(0, position, true);
    view.setUint32(2, speed, true);

    this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
    this.log(`Moving to position ${position} at ${speed}ms per step`);
  }

  handleHome() {
    const speed = parseInt(document.getElementById("manualSpeed").value);

    // Validate speed
    if (isNaN(speed) || speed < 1 || speed > 4294967295) {
      this.log("Error: Speed must be between 1 and 4,294,967,295 milliseconds");
      return;
    }

    // Use position command with position 0 (home)
    const position = 0;

    // Binary format: position(2), speed(4)
    const buffer = new ArrayBuffer(6);
    const view = new DataView(buffer);
    view.setUint16(0, position, true);
    view.setUint32(2, speed, true);

    this.sendCommand(this.CMD_POS_WITH_SPEED, new Uint8Array(buffer));
    this.log(`Going home (position 0) at ${speed}ms per step`);
  }

  saveProgram() {
    const programSlot = document.getElementById("programSlot").value;
    const programName =
      document.getElementById("programName").value.trim() ||
      `PGM${parseInt(programSlot) + 1}`;

    // Limit program name to 8 characters
    const limitedName = programName.substring(0, 8).toUpperCase();

    // Save loop program (only type supported, runs infinitely)
    const steps = document.getElementById("loopSteps").value;
    const delay = document.getElementById("loopDelay").value;

    // Warn about very large delays
    const delayValue = parseInt(delay);
    if (delayValue > 10000) {
      // More than 10 seconds
      const estimatedTime = Math.round((parseInt(steps) * delayValue) / 1000);
      if (
        !confirm(
          `Warning: Large delay detected!\n\nApproximate time per direction: ${estimatedTime} seconds (${Math.round(
            estimatedTime / 60
          )} minutes)\n\nProgram will run infinitely until manually stopped.\n\nAre you sure you want to continue?`
        )
      ) {
        return;
      }
    }

    // Binary format: programId(1), name(8), steps(2), delayMs(4)
    // Note: cycles removed - programs now run infinitely
    const buffer = new ArrayBuffer(15);
    const view = new DataView(buffer);
    const encoder = new TextEncoder();

    view.setUint8(0, parseInt(programSlot));

    // Name (8 bytes, padded with spaces)
    const nameBytes = encoder.encode(limitedName.padEnd(8, " "));
    for (let i = 0; i < 8; i++) {
      view.setUint8(1 + i, nameBytes[i] || 32); // 32 = space
    }

    view.setUint16(9, parseInt(steps), true);
    view.setUint32(11, parseInt(delay), true);

    this.sendCommand(this.CMD_LOOP_PROGRAM, new Uint8Array(buffer));
    this.log(
      `Infinite Loop Program "${limitedName}" saved to slot ${
        parseInt(programSlot) + 1
      } (${steps} steps, ${delay}ms delay, runs until stopped)`
    );
  }

  testProgram() {
    const programSlot = parseInt(document.getElementById("programSlot").value);

    // Binary format: programId(1)
    const buffer = new ArrayBuffer(1);
    const view = new DataView(buffer);
    view.setUint8(0, programSlot);

    this.sendCommand(this.CMD_RUN, new Uint8Array(buffer));
    this.log(`Testing Program ${programSlot + 1}`);
  }

  updateConnectionStatus(connected) {
    const status = document.getElementById("status");
    const connectBtn = document.getElementById("connectBtn");

    if (connected) {
      status.textContent = "Connected";
      status.className = "status-connected";
      connectBtn.textContent = "Disconnect";
      connectBtn.onclick = () => this.disconnect();
    } else {
      status.textContent = "Disconnected";
      status.className = "status-disconnected";
      connectBtn.textContent = "Connect Slider";
      connectBtn.onclick = () => this.connect();
    }
  }

  loadProgramNameFromStorage() {
    const programSlot = parseInt(document.getElementById("programSlot").value);

    // Load program name
    const programName = this.programNames[programSlot] || "";
    document.getElementById("programName").value = programName;

    // Load program data if it exists
    const programData = this.loopPrograms[programSlot];
    if (programData) {
      document.getElementById("loopSteps").value = programData.steps;
      document.getElementById("loopDelay").value = programData.delay;
      console.log(
        `Loaded program ${programSlot}: "${programName}" (${programData.steps}steps, ${programData.delay}ms, infinite)`
      );
    } else {
      // Clear form fields if no program data exists
      document.getElementById("loopSteps").value = 1000;
      document.getElementById("loopDelay").value = 1000;
      console.log(`No data for program slot ${programSlot}, using defaults`);
    }
  }

  saveProgramNames() {
    const data = {
      names: this.programNames,
      loopPrograms: this.loopPrograms,
    };
    localStorage.setItem("sliderProgramData", JSON.stringify(data));
  }

  loadProgramNames() {
    const saved = localStorage.getItem("sliderProgramData");
    if (saved) {
      const data = JSON.parse(saved);
      this.programNames = data.names || {};
      this.loopPrograms = data.loopPrograms || {};
    }
  }

  saveManualSpeed() {
    const speed = document.getElementById("manualSpeed").value;
    localStorage.setItem("sliderManualSpeed", speed);
  }

  loadManualSpeed() {
    const savedSpeed = localStorage.getItem("sliderManualSpeed");
    if (savedSpeed) {
      document.getElementById("manualSpeed").value = savedSpeed;
    }
  }

  requestAllDataFromEEPROM() {
    // Request all EEPROM data in one bulk transfer
    this.sendCommand(this.CMD_GET_ALL_DATA, new Uint8Array(0));
  }

  log(message) {
    const console = document.getElementById("console-output");
    const timestamp = new Date().toLocaleTimeString();
    console.innerHTML += `<div>[${timestamp}] ${message}</div>`;
    console.scrollTop = console.scrollHeight;
  }
}

// Initialize the controller when page loads
document.addEventListener("DOMContentLoaded", () => {
  new SliderController();
});
