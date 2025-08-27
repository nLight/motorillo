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
        this.switchTab(e.target.dataset.tab),
      );
    });

    // Forms
    document
      .getElementById("configForm")
      .addEventListener("submit", (e) => this.handleConfigSubmit(e));
    document
      .getElementById("cycleForm")
      .addEventListener("submit", (e) => this.handleCycleSubmit(e));

    // Manual controls
    document
      .getElementById("startBtn")
      .addEventListener("click", () => this.sendCommand("START"));
    document
      .getElementById("stopBtn")
      .addEventListener("click", () => this.sendCommand("STOP"));
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

    // Initialize with one step
    this.addProgramStep();
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
    const acceleration = document.getElementById("acceleration").value;

    const command = `CONFIG,${totalSteps},${speed},${acceleration}`;
    this.sendCommand(command);
  }

  handleCycleSubmit(e) {
    e.preventDefault();
    const length = document.getElementById("cycleLength").value;
    const speed1 = document.getElementById("forwardSpeed").value;
    const speed2 = document.getElementById("backwardSpeed").value;
    const pause1 = document.getElementById("startPause").value;
    const pause2 = document.getElementById("endPause").value;

    const command = `CYCLE,${length},${speed1},${speed2},${pause1},${pause2}`;
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
            <label>Speed (μs): <input type="number" class="step-speed" value="1000" min="100" max="1000000"></label>
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

    let command = `PROGRAM,0,${steps.length}`;

    steps.forEach((step) => {
      const position = step.querySelector(".step-position").value;
      const speed = step.querySelector(".step-speed").value;
      const pause = step.querySelector(".step-pause").value;
      command += `,${position},${speed},${pause}`;
    });

    this.sendCommand(command);
  }

  testProgram() {
    this.sendCommand("RUN,0");
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
