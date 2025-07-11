# nfcGOD - The Ultimate NFC Hacking Tool

🚀 **Unlock the Power of NFC Like Never Before!** 🚀

nfcGOD is your all-in-one NFC hacking Swiss Army knife, packed with powerful features to explore, analyze, and manipulate NFC/RFID tags. Whether you're a security researcher, maker, or just curious about NFC technology, this device will blow your mind!

## 🔥 Killer Features

- **Brute Force Attack Mode** - Crack Mifare Classic keys like a pro with our optimized attack algorithm
- **Universal Tag Reader** - Supports all major NFC types (Mifare Classic, Ultralight, NTAG)
- **Card Emulation** - Clone and emulate cards with ease
- **Data Visualization** - Beautiful OLED display shows all tag details
- **SD Card Storage** - Save and manage unlimited card dumps
- **Web Interface** - Control the device and view data from your browser.
- **Portable Design** - Fits in your pocket but packs enterprise-grade power

## 💪 Why You'll Love It

- **Hack Fast** - Our optimized brute force can crack keys up to 3x faster than other tools
- **Learn NFC Security** - Perfect for understanding RFID vulnerabilities
- **No Laptop Needed** - Fully standalone operation with intuitive button controls
- **Open Source** - Customize and extend to your heart's content

## 🛠️ Technical Specs

- ESP32-S3 powerhouse with 240MHz dual-core processor
- Adafruit PN532 NFC module with broad compatibility
- 128x64 OLED display for crisp visuals
- MicroSD card slot for massive storage
- Tactile buttons for easy one-handed operation

## 🎯 Perfect For

- Security researchers analyzing RFID systems
- Makers creating NFC-based projects
- Students learning about wireless security
- Anyone who wants to explore the invisible world of NFC!

## ⚡ Get Started

1. Flash the firmware using PlatformIO
2. Insert a formatted microSD card
3. Power up and start exploring!

```bash
pio run --target upload
```

Join the NFC revolution today! What will YOU discover?

---

| Button Back       | `GPIO17`     | Return to Previous Menu      |
| **On-board LED**  | `GPIO38`     | Status Indicator             |

## 🚀 Getting Started

### Prerequisites
- [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/platformio-ide).

### Installation
1. **Clone the Repository**:
   ```bash
   git clone <repository-url>
   ```
2. **Assemble Hardware**: Connect all the components as specified in the [Pinout Connections](#-pinout-connections) table.
3. **Open in PlatformIO**: Open the cloned repository folder in VS Code. PlatformIO should automatically recognize it as a project.
4. **Build and Upload**:
   - Connect the ESP32-S3 to your computer.
   - Use the PlatformIO controls to build and upload the project:
     ```bash
     pio run -t upload
     ```
   - To monitor the serial output for debugging:
     ```bash
     pio device monitor
     ```

## 📖 Major Functions

The firmware is organized into a simple, menu-driven system accessible via the hardware buttons and a web interface.

- **Read Card**:
  - Detects and identifies a nearby NFC card.
  - Reads the card's data (UID, type, and content).
  - Saves a binary dump of the card to the SD card, named after the card's UID (e.g., `04A1B2C3.nfc`).
  - Displays a summary of the card's properties.

- **Write Card**:
  - Lists all saved `.nfc` files from the SD card.
  - Allows you to select a file to write to a blank or rewritable card.
  - *Note: Currently supports writing to Mifare Classic cards.*

- **Emulate Card**:
  - Lists saved `.nfc` files.
  - Emulates the selected card, allowing it to be read by an external NFC reader.

- **Brute Force**:
  - Targets a Mifare Classic card.
  - Iterates through a list of common, default, and known weak keys to try and authenticate with each sector.
  - Reports any found keys and saves them to a log file on the SD card.

- **Card Manager**:
  - Lists all saved card files on the SD card.
  - Allows you to select and delete files to manage storage.

- **Settings**:
  - **Brightness**: Adjust the OLED screen brightness.
  - **Debug**: Toggle debug mode (future feature).
  - **Format SD**: Deletes all saved card files from the `/cards/` directory.
  - **About**: Displays project information.

- **Web Interface**:
  - When the device boots, it starts a Wi-Fi Access Point with the SSID `nfcGOD` and password `money`.
  - Connect to this network and navigate to `http://192.168.4.1` in your browser.
  - The web UI allows you to:
    - View the current device mode.
    - Trigger a card read and see the UID and data dump.
    - Change the device's mode of operation.

## 📝 Project To-Dos

This project is under active development. Here are some of the planned features and improvements:
- [ ] **Enhanced Card Support**: Add full support for ISO14443-4A cards and other standards.
- [ ] **Advanced Emulation**: Implement more sophisticated emulation modes, including UID change for compatible cards.
- [ ] **GUI Overhaul**: Improve the user interface with better graphics and more detailed information displays.
- [ ] **Key Management**: Add a feature to manage and use custom keys for Mifare Classic authentication.
- [x] **Wi-Fi/Bluetooth Integration**: Add remote control or data transfer capabilities over Wi-Fi or BLE. (Basic implementation complete)
- [ ] **PCAP Logging**: Save raw NFC traffic to a `.pcap` file for analysis in tools like Wireshark.
- [ ] **Save/Load Brute-Force State**: Allow pausing and resuming long-running brute-force attacks.

## 📚 Libraries Used

This project relies on the following excellent libraries:
- **Adafruit PN532**: For interfacing with the NFC module.
- **Adafruit GFX & SSD1306**: For rendering graphics and text on the OLED display.
- **ESPAsyncWebServer & AsyncTCP**: For the web interface.
- **SPI, Wire, SD**: Core Arduino libraries for communication protocols.

## License

This project is licensed under the **MIT License**. See the `LICENSE` file for details.
