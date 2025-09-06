class SliderController {
  constructor() {
    this.port = null;
    this.connected = false;

    // Command codes for memory efficiency
    this.CMD_POS = 2;
    this.CMD_RUN = 3;
    this.CMD_START = 4;
    this.CMD_STOP = 5;
    this.CMD_HOME = 6;
    this.CMD_SETHOME = 8;
    this.CMD_LOOP_PROGRAM = 9;
    this.CMD_GET_ALL_DATA = 13; // Request all EEPROM data
    this.CMD_DEBUG_INFO = 14; // Request debug information

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
      .addEventListener("click", () => this.sendCommand(this.CMD_HOME));
    document
      .getElementById("moveBtn")
      .addEventListener("click", () => this.handleMove());

    // Program builder
    document
      .getElementById("saveProgram")
      .addEventListener("click", () => this.saveProgram());
    document
      .getElementById("testProgram")
      .addEventListener("click", () => this.testProgram());
    document
      .getElementById("loadAllFromEEPROM")
      .addEventListener("click", () => this.requestAllDataFromEEPROM());
    document
      .getElementById("clearSteps")
      .addEventListener("click", () => this.clearSteps());

    // Initialize program storage
    this.programs = {}; // Store programs locally for editing
    this.programNames = {}; // Store program names locally
    this.loopPrograms = {}; // Store loop programs locally

    // Load saved program names from localStorage
    this.loadProgramNames();

    // Set up program slot change handler
    document.getElementById("programSlot").addEventListener("change", () => {
      this.loadProgramNameFromStorage();
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

      // Load all EEPROM data in bulk
      setTimeout(() => {
        this.requestAllDataFromEEPROM();
      }, 1000); // Wait 1 second for Arduino to be ready
    } catch (error) {
      this.log(`Connection failed: ${error.message}`);
    }
  }

  async disconnect() {
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

  setupDataListener() {
    // WebUSB serial interface data reception
    this.port.onReceive = (data) => {
      console.log("Received data:", data);
      this.handleIncomingData(data);
    };

    this.port.onReceiveError = (error) => {
      console.error("Receive error:", error);
    };
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

      // Check if this looks like binary data (first byte < 32 or very long)
      if (
        uint8Data.length > 0 &&
        (uint8Data[0] < 32 || uint8Data.length > 50)
      ) {
        isBinary = true;
        this.handleBinaryData(uint8Data);
        return;
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

    const bytes = new Uint8Array(data.length);
    for (let i = 0; i < data.length; i++) {
      bytes[i] = data.charCodeAt(i);
    }

    // Check if this is bulk data (much longer than individual programs)
    if (data.length > 50) {
      this.handleBulkData(bytes);
      return;
    }

    const programId = bytes[0];
    this.log(`Received binary data for program ${programId}`);

    // Check if this is loop program data (16 bytes expected for loop programs)
    if (data.length === 17) {
      // 16 bytes + newline
      this.handleLoopProgramBinary(bytes);
    }
    // Check if this is complex program data (variable length)
    else if (data.length >= 11) {
      // minimum: programId(1) + name(8) + stepCount(1) + at least 1 step(8)
      this.handleComplexProgramBinary(bytes);
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

    // Extract loop data (bytes 9-15)
    const view = new DataView(bytes.buffer, 9);
    const steps = view.getUint16(0, true);
    const delayMs = view.getUint32(2, true);
    const cycles = bytes[15];

    // Store in local storage
    this.loopPrograms[programId] = { steps, delay: delayMs, cycles };
    this.programNames[programId] = name;
    this.saveProgramNames();

    // Load into UI
    this.switchProgramType("loop");
    document.getElementById("programName").value = name;
    document.getElementById("loopSteps").value = steps;
    document.getElementById("loopDelay").value = delayMs;
    document.getElementById("loopCycles").value = cycles;

    this.log(
      `Loaded Loop Program "${name}" from EEPROM (slot ${programId + 1})`
    );
  }

  handleComplexProgramBinary(bytes) {
    const programId = bytes[0];

    // Extract name (bytes 1-8)
    let name = "";
    for (let i = 1; i <= 8; i++) {
      if (bytes[i] !== 0 && bytes[i] !== 32) {
        name += String.fromCharCode(bytes[i]);
      }
    }
    name = name.trim() || `PGM${programId + 1}`;

    const stepCount = bytes[9];

    // Extract steps (starting from byte 10)
    const steps = [];
    for (let i = 0; i < stepCount; i++) {
      const offset = 10 + i * 8;
      const view = new DataView(bytes.buffer, offset);
      const position = view.getUint16(0, true);
      const speed = view.getUint32(2, true);
      const pause = view.getUint16(6, true);
      steps.push({ position, speed, pause });
    }

    // Store in local storage
    this.programs[programId] = steps;
    this.programNames[programId] = name;
    this.saveProgramNames();

    // Load into UI
    this.switchProgramType("complex");
    document.getElementById("programName").value = name;

    // Clear existing steps
    this.clearSteps();

    // Load steps
    steps.forEach((step, index) => {
      this.addProgramStep();
      const stepDiv = document.querySelectorAll(".program-step")[index];
      stepDiv.querySelector(".step-position").value = step.position;
      stepDiv.querySelector(".step-speed").value = step.speed;
      stepDiv.querySelector(".step-pause").value = step.pause;
    });

    this.log(
      `Loaded Complex Program "${name}" from EEPROM (slot ${
        programId + 1
      }, ${stepCount} steps)`
    );
  }

  handleBulkData(bytes) {
    this.log("Received bulk EEPROM data");

    let offset = 0;

    // Parse program count (first byte now)
    const programCount = bytes[offset];
    offset += 1;

    this.log(`Loading ${programCount} programs from EEPROM`);

    // Clear existing programs
    this.programs = {};
    this.loopPrograms = {};
    this.programNames = {};

    // Parse each program
    for (let i = 0; i < programCount; i++) {
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
        // Loop program
        const loopView = new DataView(bytes.buffer, offset);
        const steps = loopView.getUint16(0, true);
        const delayMs = loopView.getUint32(2, true);
        const cycles = bytes[offset + 6];

        this.loopPrograms[programId] = { steps, delay: delayMs, cycles };
        offset += 7; // steps(2) + delayMs(4) + cycles(1)

        this.log(`Loaded Loop Program "${name}" (slot ${programId + 1})`);
      } else if (programType === 1) {
        // Complex program
        const stepCount = bytes[offset];
        offset += 1;

        const steps = [];
        for (let j = 0; j < stepCount; j++) {
          const stepView = new DataView(bytes.buffer, offset);
          const position = stepView.getUint16(0, true);
          const speed = stepView.getUint32(2, true);
          const pause = stepView.getUint16(6, true);
          steps.push({ position, speed, pause });
          offset += 8;
        }

        this.programs[programId] = steps;
        this.log(
          `Loaded Complex Program "${name}" (slot ${
            programId + 1
          }, ${stepCount} steps)`
        );
      }
    }

    // Save to localStorage
    this.saveProgramNames();
    localStorage.setItem("sliderPrograms", JSON.stringify(this.programs));
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

    // Binary format: position(2)
    const buffer = new ArrayBuffer(2);
    const view = new DataView(buffer);
    view.setUint16(0, position, true);

    this.sendCommand(this.CMD_POS, new Uint8Array(buffer));
  }

  addProgramStep() {
    const stepsList = document.getElementById("stepsList");
    const stepNum = stepsList.children.length;

    const stepDiv = document.createElement("div");
    stepDiv.className = "program-step";
    stepDiv.innerHTML = `
            <h4>Step ${stepNum + 1}</h4>
            <label>Position: <input type="number" class="step-position" value="0" min="0" max="20000"></label>
            <label>Speed (ms): <input type="number" class="step-speed" value="1000" min="1"></label>
            <label>Pause (ms): <input type="number" class="step-pause" value="0" min="0" max="10000"></label>
            <button onclick="this.parentElement.remove()">Remove</button>
        `;
    stepsList.appendChild(stepDiv);
  }

  switchProgramType(type) {
    this.currentProgramType = type;

    // Update button states
    document
      .querySelectorAll(".program-type-btn")
      .forEach((btn) => btn.classList.remove("active"));
    document.getElementById(`${type}ProgramBtn`).classList.add("active");

    // Show/hide sections
    document.getElementById("complexProgramSection").style.display =
      type === "complex" ? "block" : "none";
    document.getElementById("loopProgramSection").style.display =
      type === "loop" ? "block" : "none";

    this.log(`Switched to ${type} program mode`);
  }

  saveProgram() {
    const programSlot = document.getElementById("programSlot").value;
    const programName =
      document.getElementById("programName").value.trim() ||
      `PGM${parseInt(programSlot) + 1}`;

    // Limit program name to 8 characters
    const limitedName = programName.substring(0, 8).toUpperCase();

    if (this.currentProgramType === "loop") {
      // Save loop program
      const steps = document.getElementById("loopSteps").value;
      const delay = document.getElementById("loopDelay").value;
      const cycles = document.getElementById("loopCycles").value;

      // Warn about very large delays
      const delayValue = parseInt(delay);
      if (delayValue > 10000) {
        // More than 10 seconds
        const totalTime = Math.round((parseInt(steps) * delayValue) / 1000);
        if (
          !confirm(
            `Warning: Large delay detected!\n\nTotal time per direction: ${totalTime} seconds (${Math.round(
              totalTime / 60
            )} minutes)\n\nAre you sure you want to continue?`
          )
        ) {
          return;
        }
      }

      // Binary format: programId(1), name(8), steps(2), delayMs(4), cycles(1)
      const buffer = new ArrayBuffer(16);
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
      view.setUint8(15, parseInt(cycles));

      this.sendCommand(this.CMD_LOOP_PROGRAM, new Uint8Array(buffer));
      this.log(
        `Loop Program "${limitedName}" saved to slot ${
          parseInt(programSlot) + 1
        } (${steps} steps, ${delay}ms delay, ${cycles} cycles)`
      );
    } else {
      // Save complex program (existing logic)
      const steps = document.querySelectorAll(".program-step");
      if (steps.length === 0) {
        this.log("No program steps defined");
        return;
      }

      // Binary format: programId(1), name(8), stepCount(1), steps(stepCount*8)
      const stepCount = steps.length;
      const buffer = new ArrayBuffer(10 + stepCount * 8);
      const view = new DataView(buffer);
      const encoder = new TextEncoder();

      view.setUint8(0, parseInt(programSlot));

      // Name (8 bytes, padded with spaces)
      const nameBytes = encoder.encode(limitedName.padEnd(8, " "));
      for (let i = 0; i < 8; i++) {
        view.setUint8(1 + i, nameBytes[i] || 32);
      }

      view.setUint8(9, stepCount);

      // Steps data
      const stepData = [];
      for (let i = 0; i < stepCount; i++) {
        const step = steps[i];
        const position = parseInt(step.querySelector(".step-position").value);
        const speed = parseInt(step.querySelector(".step-speed").value);
        const pause = parseInt(step.querySelector(".step-pause").value);

        const offset = 10 + i * 8;
        view.setUint16(offset, position, true);
        view.setUint32(offset + 2, speed, true);
        view.setUint16(offset + 6, pause, true);

        stepData.push({ position, speed, pause });
      }

      // Store program and name locally for future editing
      this.programs[programSlot] = stepData;
      this.programNames[programSlot] = limitedName;
      this.saveProgramNames();

      // Update the input field with the limited name
      document.getElementById("programName").value = limitedName;

      // Send to Arduino
      this.sendCommand(this.CMD_PROGRAM, new Uint8Array(buffer));
      this.log(
        `Program "${limitedName}" saved to slot ${
          parseInt(programSlot) + 1
        } with ${steps.length} steps`
      );
    }
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

  loadProgram() {
    const programSlot = document.getElementById("programSlot").value;
    const programName =
      this.programNames[programSlot] || `PGM${parseInt(programSlot) + 1}`;

    // Check if we have a loop program stored locally
    if (this.loopPrograms[programSlot]) {
      // Load loop program
      this.switchProgramType("loop");
      const loopData = this.loopPrograms[programSlot];

      document.getElementById("programName").value = programName;
      document.getElementById("loopSteps").value = loopData.steps;
      document.getElementById("loopDelay").value = loopData.delay;
      document.getElementById("loopCycles").value = loopData.cycles;

      this.log(
        `Loaded Loop Program "${programName}" from local storage (slot ${
          parseInt(programSlot) + 1
        })`
      );
      return;
    }

    // Check if we have a complex program stored locally
    if (this.programs[programSlot]) {
      // Load complex program
      const program = this.programs[programSlot];

      // Switch to complex program mode
      this.switchProgramType("complex");

      // Clear existing steps
      this.clearSteps();

      // Load program name
      document.getElementById("programName").value = programName;

      // Load steps from stored program
      program.forEach((step, index) => {
        this.addProgramStep();
        const stepDiv = document.querySelectorAll(".program-step")[index];
        stepDiv.querySelector(".step-position").value = step.position;
        stepDiv.querySelector(".step-speed").value = step.speed;
        stepDiv.querySelector(".step-pause").value = step.pause;
      });

      this.log(
        `Loaded Complex Program "${programName}" from local storage (slot ${
          parseInt(programSlot) + 1
        } with ${program.length} steps)`
      );
      return;
    }

    // No local data found, try to load from Arduino EEPROM
    this.log(
      `No local data found for slot ${
        parseInt(programSlot) + 1
      }, requesting from Arduino EEPROM...`
    );
    this.requestProgramFromEEPROM(programSlot);
  }

  clearSteps() {
    const stepsList = document.getElementById("stepsList");
    stepsList.innerHTML = "";
    this.log("Cleared all steps");
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
    const programSlot = document.getElementById("programSlot").value;
    const programName = this.programNames[programSlot] || "";
    document.getElementById("programName").value = programName;
  }

  saveProgramNames() {
    const data = {
      names: this.programNames,
      complexPrograms: this.programs,
      loopPrograms: this.loopPrograms,
    };
    localStorage.setItem("sliderProgramData", JSON.stringify(data));
  }

  loadProgramNames() {
    const saved = localStorage.getItem("sliderProgramData");
    if (saved) {
      const data = JSON.parse(saved);
      this.programNames = data.names || {};
      this.programs = data.complexPrograms || {};
      this.loopPrograms = data.loopPrograms || {};
    }
  }

  requestProgramFromEEPROM(programSlot) {
    // Request program data from Arduino EEPROM
    const buffer = new ArrayBuffer(1);
    const view = new DataView(buffer);
    view.setUint8(0, parseInt(programSlot));

    // Try to get loop program first
    this.sendCommand(this.CMD_GET_LOOP_PROGRAM, new Uint8Array(buffer));

    // Also try to get complex program
    this.sendCommand(this.CMD_GET_PROGRAM, new Uint8Array(buffer));
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
