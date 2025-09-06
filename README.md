# Motorillo 🎬

> **Precision stepper motor controller with WebUSB interface for camera sliders, timelapses, and automation projects.**

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-00979D?style=flat&logo=Arduino&logoColor=white)](https://www.arduino.cc/)
[![WebUSB](https://img.shields.io/badge/WebUSB-Enabled-4285F4?style=flat&logo=googlechrome&logoColor=white)](https://webusb.github.io/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

## ✨ Features

- 🌐 **Browser Control**: Direct WebUSB communication - no drivers needed
- 📱 **Dual Interface**: Standalone OLED menu + web interface
- 💾 **Persistent Storage**: Save up to 5 programs in EEPROM
- ⚡ **Precision Control**: Customizable speed (1ms to 4.3M ms per step)
- 🔄 **Auto-Recovery**: Graceful USB disconnection handling
- 🎯 **Infinite Programs**: Forward/backward motion until manually stopped
- 📊 **Real-time Feedback**: Live position and status updates

## 🚀 Quick Start

### Hardware Requirements

| Component          | Specification                               |
| ------------------ | ------------------------------------------- |
| **Arduino Board**  | Leonardo/Micro/Zero (WebUSB compatible)     |
| **Stepper Motor**  | 200 steps/revolution with compatible driver |
| **Driver**         | A4988, TMC2209                              |
| **Display**        | SSD1306 OLED 96×16 pixels (I2C)             |
| **Control Button** | Momentary push button                       |

### Connections

```
Arduino Leonardo/Micro:
├── Pin 10 → Stepper DIR
├── Pin 16 → Stepper STEP
├── Pin 2  → Control Button (internal pullup)
├── SDA/SCL → OLED Display (I2C)
└── USB → Computer (WebUSB + Power)
```

### Installation

1. **Clone the repository:**

   ```bash
   git clone https://github.com/yourusername/motorillo.git
   cd motorillo
   ```

2. **Upload firmware:**

   - Open `steppper.ino` in Arduino IDE
   - Install required libraries (see [Setup Guide](docs/setup/installation.md))
   - Upload to your Arduino board

3. **Access web interface:**
   - Serve the `webusb/` folder on localhost
   - Connect via Chrome/Edge browser
   - Click "Connect Slider" to establish WebUSB connection

## 📖 Documentation

- 📋 **[Setup Guide](docs/setup/)** - Hardware assembly and software installation
- 🛠 **[User Manual](docs/features/)** - Operating modes and program creation
- 👨‍💻 **[Development](docs/development/)** - Architecture and API reference
- 🔧 **[Troubleshooting](docs/setup/troubleshooting.md)** - Common issues and solutions

## 💡 Use Cases

- **📹 Camera Sliders**: Smooth motion for video production
- **⏱ Timelapse Photography**: Ultra-slow motion over hours/days
- **🏭 Automation**: Precise positioning for manufacturing
- **🎓 Education**: Learn motor control and WebUSB programming
- **🔬 Research**: Laboratory positioning systems

## 🎮 Control Modes

### Standalone Mode (OLED + Button)

- Navigate stored programs with push button
- View current position and status
- Start/stop/pause program execution

### WebUSB Mode (Browser Interface)

- Create and save motion programs
- Manual jog controls with custom speed
- Real-time monitoring and control
- Bulk program management

## 🔧 Technical Specifications

| Parameter          | Range                           | Default    |
| ------------------ | ------------------------------- | ---------- |
| **Speed Range**    | 1ms - 4,294,967,295ms per step  | 1000ms     |
| **Max Programs**   | 5 stored programs               | -          |
| **Position Range** | 0 - 65,535 steps                | 2000 steps |
| **Loop Programs**  | Infinite until manually stopped | -          |
| **Memory Usage**   | ~1KB EEPROM                     | -          |

## 🏗 Architecture

```
┌─────────────────┐    ┌──────────────────┐
│   Web Browser   │◄──►│   Pro Micro      │
│   (WebUSB UI)   │    │   (Motorillo)    │
└─────────────────┘    └──────────────────┘
                                │
                        ┌───────┴───────┐
                        │ Stepper Motor │
                        │   + Driver    │
                        └───────────────┘
```

**Modular Firmware:**

- `motor_control` - Movement and positioning
- `command_processor` - WebUSB communication
- `config_manager` - EEPROM storage
- `display_manager` - OLED interface
- `menu_system` - Standalone navigation

## 🤝 Contributing

We welcome contributions! Please see our [Development Guide](docs/development/) for:

- Code architecture overview
- Building and testing procedures
- Pull request guidelines

## 📚 Complete Documentation

This README provides a quick overview. For comprehensive documentation:

- **[Setup & Installation](docs/setup/)** - Hardware wiring, software installation, troubleshooting
- **[Features & Usage](docs/features/)** - User manual, speed control, WebUSB features
- **[Development](docs/development/)** - Architecture, API reference, protocol details

Start with the [Documentation Index](docs/README.md) for easy navigation.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built with Arduino framework and WebUSB standard
- OLED support via Adafruit libraries
- Inspired by the maker community's creativity

---

**Ready to automate your motion?** [Get started with the setup guide](docs/setup/) →
