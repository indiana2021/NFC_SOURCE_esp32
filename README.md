# NFC Multitool for ESP32-S3

A versatile NFC tool built for ESP32-S3-DevKitC-1 that provides:
- NFC card reading/writing
- Reader detection
- Card emulation (basic)
- Brute force key testing
- SD card storage

## Features

- Supports multiple NFC card types:
  - Mifare Classic
  - Mifare Ultralight
  - NTAG
- SD card storage for card dumps
- OLED display interface
- Button navigation
- Brute force common Mifare keys

## Hardware Requirements

- ESP32-S3-DevKitC-1 Development Board
- PN532 NFC module
- OLED display (SSD1306, 128x64)
- MicroSD card module
- Tactile buttons

## Pin Connections

| Component | ESP32-S3 Pin |
|-----------|-------------|
| PN532 IRQ | GPIO4 |
| PN532 RST | GPIO5 |
| SD CS     | GPIO10 |
| OLED SDA  | GPIO8 |
| OLED SCL  | GPIO9 |
| Button Up | GPIO2 |
| Button Down | GPIO3 |
| Button Select | GPIO1 |
| Button Back | GPIO0 |

## Installation

1. Install PlatformIO
2. Clone this repository
3. Connect hardware as specified above
4. Build and upload:
```bash
pio run -t upload
```

## Usage

Navigate menus using the buttons:
- Up/Down: Change selection
- Select: Choose option
- Back: Return to previous menu

## Libraries Used

- Adafruit PN532
- Adafruit GFX/SSD1306
- SPI
- Wire
- SD

## Project Structure

```
.
├── include/          # Header files
├── lib/             # Library dependencies
├── src/             # Main source code
│   └── main.cpp     # Primary application code
├── test/            # Test files
└── platformio.ini   # Build configuration
```

## License

MIT License
