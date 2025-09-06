var serial = {};

(function () {
  "use strict";

  serial.getPorts = function () {
    return navigator.usb.getDevices().then((devices) => {
      return devices.map((device) => new serial.Port(device));
    });
  };

  serial.requestPort = function () {
    const filters = [
      { vendorId: 0x2341, productId: 0x8036 }, // Arduino Leonardo
      { vendorId: 0x2341, productId: 0x8037 }, // Arduino Micro
      { vendorId: 0x2341, productId: 0x804d }, // Arduino/Genuino Zero
      { vendorId: 0x2341, productId: 0x804e }, // Arduino/Genuino MKR1000
      { vendorId: 0x2341, productId: 0x804f }, // Arduino MKRZERO
      { vendorId: 0x2341, productId: 0x8050 }, // Arduino MKR FOX 1200
      { vendorId: 0x2341, productId: 0x8052 }, // Arduino MKR GSM 1400
      { vendorId: 0x2341, productId: 0x8053 }, // Arduino MKR WAN 1300
      { vendorId: 0x2341, productId: 0x8054 }, // Arduino MKR WiFi 1010
      { vendorId: 0x2341, productId: 0x8055 }, // Arduino MKR NB 1500
      { vendorId: 0x2341, productId: 0x8056 }, // Arduino MKR Vidor 4000
      { vendorId: 0x2341, productId: 0x8057 }, // Arduino NANO 33 IoT
      { vendorId: 0x239a }, // Adafruit Boards!
      { vendorId: 0x1b4f }, // Pro Micro
      { vendorId: 0x2341 }, // Generic Arduino
    ];
    return navigator.usb
      .requestDevice({ filters: filters })
      .then((device) => new serial.Port(device));
  };

  serial.Port = function (device) {
    this.device_ = device;
    this.interfaceNumber_ = 0; // Start with interface 0, will be detected
    this.endpointIn_ = 0; // Will be detected
    this.endpointOut_ = 0; // Will be detected
  };

  serial.Port.prototype.connect = function () {
    let readLoop = () => {
      this.device_.transferIn(this.endpointIn_, 64).then(
        (result) => {
          if (result.data && result.data.byteLength > 0) {
            this.onReceive(result.data);
          }
          // Add a small delay to reduce USB traffic
          setTimeout(readLoop, 10);
        },
        (error) => {
          this.onReceiveError(error);
          // Stop the loop on error to prevent spam
        },
      );
    };

    return this.device_
      .open()
      .then(() => {
        if (this.device_.configuration === null) {
          return this.device_.selectConfiguration(1);
        }
      })
      .then(() => {
        var configurationInterfaces = this.device_.configuration.interfaces;
        let webUsbInterfaceFound = false;
        
        // First try to find WebUSB interface (class 0xFF)
        configurationInterfaces.forEach((element) => {
          element.alternates.forEach((elementalt) => {
            if (elementalt.interfaceClass == 0xff) {
              this.interfaceNumber_ = element.interfaceNumber;
              elementalt.endpoints.forEach((elementendpoint) => {
                if (elementendpoint.direction == "out") {
                  this.endpointOut_ = elementendpoint.endpointNumber;
                }
                if (elementendpoint.direction == "in") {
                  this.endpointIn_ = elementendpoint.endpointNumber;
                }
              });
              webUsbInterfaceFound = true;
            }
          });
        });
        
        // If no WebUSB interface found, look for CDC-ACM interface (class 0x0A)
        if (!webUsbInterfaceFound) {
          configurationInterfaces.forEach((element) => {
            element.alternates.forEach((elementalt) => {
              if (elementalt.interfaceClass == 0x0a) {
                this.interfaceNumber_ = element.interfaceNumber;
                elementalt.endpoints.forEach((elementendpoint) => {
                  if (elementendpoint.direction == "out") {
                    this.endpointOut_ = elementendpoint.endpointNumber;
                  }
                  if (elementendpoint.direction == "in") {
                    this.endpointIn_ = elementendpoint.endpointNumber;
                  }
                });
                webUsbInterfaceFound = true;
              }
            });
          });
        }
        
        // If still nothing found, use first available interface
        if (!webUsbInterfaceFound && configurationInterfaces.length > 0) {
          const firstInterface = configurationInterfaces[0];
          this.interfaceNumber_ = firstInterface.interfaceNumber;
          if (firstInterface.alternates[0].endpoints.length > 0) {
            firstInterface.alternates[0].endpoints.forEach((endpoint) => {
              if (endpoint.direction == "out") {
                this.endpointOut_ = endpoint.endpointNumber;
              }
              if (endpoint.direction == "in") {
                this.endpointIn_ = endpoint.endpointNumber;
              }
            });
          }
        }
        
        console.log(`Using interface ${this.interfaceNumber_}, endpoints: in=${this.endpointIn_}, out=${this.endpointOut_}`);
      })
      .then(() => this.device_.claimInterface(this.interfaceNumber_))
      .then(() =>
        this.device_.selectAlternateInterface(this.interfaceNumber_, 0),
      )
      .then(() =>
        this.device_.controlTransferOut({
          requestType: "class",
          recipient: "interface",
          request: 0x22,
          value: 0x01,
          index: this.interfaceNumber_,
        }),
      )
      .then(() => {
        readLoop();
      });
  };

  serial.Port.prototype.disconnect = function () {
    return this.device_
      .controlTransferOut({
        requestType: "class",
        recipient: "interface",
        request: 0x22,
        value: 0x00,
        index: this.interfaceNumber_,
      })
      .then(() => this.device_.close());
  };

  serial.Port.prototype.send = function (data) {
    return this.device_.transferOut(this.endpointOut_, data);
  };
})();
