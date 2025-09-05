class SliderController {
  constructor() {
    this.port = null;
    this.connected = false;
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

    // Forms
    document
      .getElementById("configForm")
      .addEventListener("submit", (e) => this.handleConfigSubmit(e));

    // Manual controls
    document
      .getElementById("startBtn")
      .addEventListener("click", () => this.sendCommand("START"));
    document
      .getElementById("stopBtn")
      .addEventListener("click", () => this.sendCommand("STOP"));
    document
      .getElementById("setHomeBtn")
      .addEventListener("click", () => this.sendCommand("SETHOME"));
    document
      .getElementById("homeBtn")
      .addEventListener("click", () => this.sendCommand("HOME"));
    document
      .getElementById("moveBtn")
      .addEventListener("click", () => this.handleMove());

    // Program builder
    document
      .getElementById("addStep")
      .addEventListener("click", () => this.addProgramStep());
    document
      .getElementById("saveProgram")
      .addEventListener("click", () => this.saveProgram());
    document
      .getElementById("testProgram")
      .addEventListener("click", () => this.testProgram());
    document
      .getElementById("loadProgram")
      .addEventListener("click", () => this.loadProgram());
    document
      .getElementById("clearSteps")
      .addEventListener("click", () => this.clearSteps());

    // Initialize with one step
    this.addProgramStep();

    // Initialize program storage
    this.programs = {}; // Store programs locally for editing
    this.programNames = {}; // Store program names locally

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
      // Set up event handlers
      port.onReceive = (data) => {
        const decoder = new TextDecoder();
        const text = decoder.decode(data);
        this.log(`Received: ${text.trim()}`);
      };

      port.onReceiveError = (error) => {
        this.log("Receive error: " + error);
      };

      await port.connect();

      this.port = port;
      this.connected = true;
      this.updateConnectionStatus(true);
      this.log("Connected to slider successfully!");
    } catch (error) {
      this.log("Connection error: " + error.message);
      this.connected = false;
      this.updateConnectionStatus(false);
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

  async sendCommand(command) {
    if (!this.connected || !this.port) {
      this.log("Not connected to slider");
      return false;
    }

    try {
      const encoder = new TextEncoder();
      const data = encoder.encode(command + "\n");

      await this.port.send(data);
      this.log(`Sent: ${command}`);
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

  handleConfigSubmit(e) {
    e.preventDefault();
    const totalSteps = document.getElementById("totalSteps").value;
    const speed = document.getElementById("defaultSpeed").value;
    const speedUnit = document.getElementById("speedUnit").value;
    const acceleration = document.getElementById("acceleration").value;
    const microstepping = document.getElementById("microstepping").value;

    const command = `CFG,${totalSteps},${speed},${acceleration},${microstepping},${speedUnit}`;
    this.sendCommand(command);
  }

  handleMove() {
    const position = document.getElementById("targetPosition").value;
    // Use a simple position command since Arduino doesn't have MOVE
    this.sendCommand(`POS,${position}`);
  }

  addProgramStep() {
    const stepsList = document.getElementById("stepsList");
    const stepNum = stepsList.children.length;

    const stepDiv = document.createElement("div");
    stepDiv.className = "program-step";
    stepDiv.innerHTML = `
            <h4>Step ${stepNum + 1}</h4>
            <label>Position: <input type="number" class="step-position" value="0" min="0" max="20000"></label>
            <div class="speed-input-group">
                <label>Speed: <input type="number" class="step-speed" value="1000" min="1"></label>
                <select class="step-speed-unit">
                    <option value="0">μs</option>
                    <option value="1">ms</option>
                </select>
            </div>
            <label>Pause (ms): <input type="number" class="step-pause" value="0" min="0" max="10000"></label>
            <button onclick="this.parentElement.remove()">Remove</button>
        `;
    stepsList.appendChild(stepDiv);
  }

  saveProgram() {
    const steps = document.querySelectorAll(".program-step");
    if (steps.length === 0) {
      this.log("No program steps defined");
      return;
    }

    const programSlot = document.getElementById("programSlot").value;
    const programName =
      document.getElementById("programName").value.trim() ||
      `PGM${parseInt(programSlot) + 1}`;

    // Limit program name to 8 characters
    const limitedName = programName.substring(0, 8).toUpperCase();

    let command = `PROGRAM,${programSlot},${limitedName},${steps.length}`;

    // Build step data
    const stepData = [];
    steps.forEach((step) => {
      const position = step.querySelector(".step-position").value;
      const speed = step.querySelector(".step-speed").value;
      const speedUnit = step.querySelector(".step-speed-unit").value;
      const pause = step.querySelector(".step-pause").value;
      command += `,${position},${speed},${pause},${speedUnit}`;
      stepData.push({ position, speed, speedUnit, pause });
    });

    // Store program and name locally for future editing
    this.programs[programSlot] = stepData;
    this.programNames[programSlot] = limitedName;
    this.saveProgramNames();

    // Update the input field with the limited name
    document.getElementById("programName").value = limitedName;

    // Send to Arduino
    this.sendCommand(command);
    this.log(
      `Program "${limitedName}" saved to slot ${
        parseInt(programSlot) + 1
      } with ${steps.length} steps`
    );
  }

  testProgram() {
    const programSlot = document.getElementById("programSlot").value;
    this.sendCommand(`RUN,${programSlot}`);
    this.log(`Testing Program ${parseInt(programSlot) + 1}`);
  }

  loadProgram() {
    const programSlot = document.getElementById("programSlot").value;
    const program = this.programs[programSlot];
    const programName =
      this.programNames[programSlot] || `PGM${parseInt(programSlot) + 1}`;

    if (!program) {
      this.log(`No program data found for slot ${parseInt(programSlot) + 1}`);
      return;
    }

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
      stepDiv.querySelector(".step-speed-unit").value = step.speedUnit || 0; // Default to microseconds
      stepDiv.querySelector(".step-pause").value = step.pause;
    });

    this.log(
      `Loaded Program "${programName}" from slot ${
        parseInt(programSlot) + 1
      } with ${program.length} steps`
    );
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
    localStorage.setItem(
      "sliderProgramNames",
      JSON.stringify(this.programNames)
    );
  }

  loadProgramNames() {
    const saved = localStorage.getItem("sliderProgramNames");
    if (saved) {
      this.programNames = JSON.parse(saved);
    }
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
