# Motorillo Documentation

Welcome to the comprehensive documentation for Motorillo, the precision stepper motor controller with WebUSB interface.

## 📖 Documentation Structure

### 🚀 Setup & Installation

Get Motorillo up and running on your system.

- **[Installation Guide](setup/installation.md)** - Complete setup from hardware to software
- **[Troubleshooting Guide](setup/troubleshooting.md)** - Common issues and solutions

### ✨ Features & Usage

Learn how to use all of Motorillo's capabilities.

- **[User Manual](features/user-manual.md)** - Complete operating guide for all users
- **[Manual Velocity Control](features/manual-velocity-control.md)** - Custom speed control system
- **[WebUSB Connection Management](features/webusb-connection.md)** - Connection handling and recovery
- **[Home Positioning System](features/home-positioning.md)** - Reference point management
- **[Extended Speed Range](features/extended-speed-range.md)** - Millisecond precision timing

### 👨‍💻 Development & Technical

Technical documentation for developers and advanced users.

- **[Architecture Overview](development/architecture.md)** - System design and module organization
- **[Command Protocol](development/command-protocol.md)** - Simplified WebUSB protocol design
- **[API Reference](development/api-reference.md)** - Complete WebUSB command reference

## 🎯 Quick Navigation

### New Users

1. Start with the **[Installation Guide](setup/installation.md)** to set up hardware and software
2. Read the **[User Manual](features/user-manual.md)** to learn basic operations
3. Check **[Troubleshooting](setup/troubleshooting.md)** if you encounter issues

### Existing Users

- **[Features Documentation](features/)** - Learn about specific capabilities
- **[API Reference](development/api-reference.md)** - WebUSB command protocol details
- **[Troubleshooting](setup/troubleshooting.md)** - Solutions to common problems

### Developers

1. **[Architecture](development/architecture.md)** - Understand the system design
2. **[Command Protocol](development/command-protocol.md)** - WebUSB communication protocol
3. **[API Reference](development/api-reference.md)** - Complete command reference

## 📋 Feature Overview

### Core Capabilities

- 🌐 **Browser Control**: Direct WebUSB communication - no drivers needed
- 📱 **Dual Interface**: Standalone OLED menu + web interface
- 💾 **Persistent Storage**: Save up to 5 programs in Arduino EEPROM
- ⚡ **Precision Control**: 1ms to 49+ days per step timing range
- 🔄 **Auto-Recovery**: Graceful USB disconnection handling

### Applications

- **📹 Camera Sliders**: Smooth motion for video production
- **⏱ Timelapse Photography**: Ultra-slow motion over hours/days
- **🏭 Automation**: Precise positioning for manufacturing
- **🎓 Education**: Learn motor control and WebUSB programming
- **🔬 Research**: Laboratory positioning systems

## 🛠 Technical Specifications

| Parameter          | Range                  | Notes                  |
| ------------------ | ---------------------- | ---------------------- |
| **Speed Range**    | 1ms - 4.3B ms per step | ~49.7 days maximum     |
| **Position Range** | 0 - 65,535 steps       | 16-bit resolution      |
| **Programs**       | 5 stored programs      | Persistent in EEPROM   |
| **Cycle Count**    | 1 - 255 cycles         | For loop programs      |
| **WebUSB**         | Chrome/Edge browsers   | No driver installation |

## 🔗 External Resources

- **[Arduino WebUSB Library](https://github.com/webusb/arduino)** - WebUSB implementation
- **[Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)** - OLED display library
- **[WebUSB Specification](https://webusb.github.io/webusb/)** - WebUSB standard documentation

## 🤝 Contributing

We welcome contributions to both code and documentation!

**For Documentation**:

- Clear, concise writing
- Practical examples and use cases
- Screenshots and diagrams where helpful
- Cross-references between related topics

**For Code**:

- Follow existing code style and patterns
- Add documentation for new features
- Test thoroughly on Arduino hardware
- Consider memory constraints (Arduino Uno: 32KB flash, 2KB RAM)

## 📄 License

This documentation is part of the Motorillo project, licensed under the MIT License. See the main [LICENSE](../LICENSE) file for details.

---

**Need Help?** Start with the [Installation Guide](setup/installation.md) or check [Troubleshooting](setup/troubleshooting.md) for common issues.

**Ready to Develop?** Begin with the [Architecture Overview](development/architecture.md) to understand the system design.
