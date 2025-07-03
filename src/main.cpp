#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_PN532.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin definitions for ESP32-S3-DevKitC-1
// Safe GPIO pins that avoid strapping, flash, and restricted pins
#define PN532_IRQ   (4)   // GPIO4 - safe digital pin
#define PN532_RESET (5)   // GPIO5 - safe digital pin
#define SD_CS       (23)  // GPIO23 - safe SPI CS pin
#define OLED_RESET  (-1)  // Use -1 for shared reset via I2C

// Button pins - using safe GPIOs with internal pullups
#define BTN_UP      (17)   // GPIO17 - safe input pin
#define BTN_DOWN    (18)   // GPIO18 - safe input pin
#define BTN_SELECT  (15)  // GPIO15 - safe input pin
#define BTN_BACK    (16)  // GPIO16 - safe input pin

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// I2C pins for ESP32-S3 (default pins)
#define SDA_PIN 21
#define SCL_PIN 22

// Initialize components
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Card data structure
struct CardData {
  uint8_t uid[10];
  uint8_t uidLength;
  uint8_t data[1024];
  uint16_t dataLength;
  uint8_t cardType;
  char filename[32];
  bool isValid;
};

// Function Prototypes
void countCards();
void displayMainMenu();
void handleInput();
void handleMainMenu();
void handleReadCard();
void handleWriteCard();
void handleEmulateCard();
void handleBruteForce();
void handleCardManager();
void handleSettings();
int getMaxMenuItems();
void displayCurrentMenu();
void selectMenuItem();
void startReadCard();
void startWriteCard();
void startEmulateCard();
void startBruteForce();
void startCardManager();
void startSettings();
void displaySettingsMenu();
void displayCardDetected(uint8_t* uid, uint8_t uidLength);
bool readCardData();
void saveCardToSD();
void displayReadSuccess();
void displayReadError();
bool readMifareClassic();
bool readMifareUltralight();
bool readNTAG();
String getCardTypeName(uint8_t cardType);
String getIssuerName(uint8_t* uid, uint8_t uidLength);
bool loadCardFromSD(const char* filename, CardData* card);
void displayBruteForceStarted();
void performBruteForceStep();
void displayBruteForceProgress();
void displayBruteForceResults();
void printKey(const uint8_t* key);
void saveBruteForceResults();
bool detectExternalReader();
bool writeMifareClassic(uint8_t* uid, uint8_t uidLength, CardData* card);
void listSDFiles(String* files, int* count, const char* extension, int maxFiles);
void showLoading(const char* msg, uint16_t duration_ms);


// Menu system
enum MenuState {
  MAIN_MENU,
  READ_CARD,
  WRITE_CARD,
  EMULATE_CARD,
  BRUTE_FORCE,
  CARD_MANAGER,
  SETTINGS
};

MenuState currentMenu = MAIN_MENU;
int menuSelection = 0;
bool cardPresent = false;

// Settings
bool debugMode = true;
uint8_t displayContrast = 255; // 0-255

// Common Mifare Classic keys for brute force
const uint8_t commonKeys[][6] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Default key
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Null key
  {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, // NXP default
  {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}, // Common variant
  {0x4D, 0x3A, 0x99, 0xC3, 0x51, 0xDD}, // Hotel key
  {0x1A, 0x98, 0x2C, 0x7E, 0x45, 0x9A}, // Access control
  {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, // Transport key
  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}, // Test key
  {0x71, 0x4C, 0x5C, 0x88, 0x6E, 0x97}, // Door lock
  {0x58, 0x7E, 0xE5, 0xF9, 0x35, 0x0F}, // Generic access
  {0xA0, 0x47, 0x8C, 0xC3, 0x90, 0x91}, // Parking
  {0x53, 0x3C, 0xB6, 0xC7, 0x23, 0xF6}, // Library system
  {0x8F, 0xD0, 0xA4, 0xF2, 0x56, 0xE9}  // Campus card
};

const uint8_t numCommonKeys = sizeof(commonKeys) / sizeof(commonKeys[0]);

// Brute force state
struct BruteForceState {
  bool isActive;
  uint8_t currentKeyIndex;
  uint8_t currentSector;
  uint8_t targetUID[10];
  uint8_t targetUIDLength;
  uint8_t foundKeys[16][6]; // Store found keys for each sector
  bool keyFound[16];
  uint32_t startTime;
  uint16_t totalAttempts;
  uint8_t successfulSectors;
} bruteForce;

CardData currentCard;
CardData emulationCard;

// Button handling
bool btnUpPressed = false;
bool btnDownPressed = false;
bool btnSelectPressed = false;
bool btnBackPressed = false;
bool btnUpLastState = HIGH;
bool btnDownLastState = HIGH;
bool btnSelectLastState = HIGH;
bool btnBackLastState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// File management
const char* CARD_DIR = "/cards/";
int totalCards = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("NFC Multitool Starting...");
  
  // Initialize button pins with internal pullups
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 allocation failed");
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.println("nfcGOD");
  display.setTextSize(1);
  display.setCursor(25, 35);
  display.println("by your name");
  display.display();
  delay(2000);
  showLoading("Initializing...", 1000);
  
  // Setup CS pin for SD card
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize SD card
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SD_CS, LOW);
  bool sdOk = SD.begin(SD_CS);
  digitalWrite(SD_CS, HIGH);
  SPI.endTransaction();

  if (!sdOk) {
    Serial.println("SD Card initialization failed!");
    display.println("SD Card Failed!");
    display.display();
    delay(2000);
  } else {
    Serial.println("SD Card initialized");
    // Create cards directory if it doesn't exist
    if (!SD.exists(CARD_DIR)) {
      SD.mkdir(CARD_DIR);
    }
    countCards();
  }
  
  // Initialize PN532
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found");
    display.println("PN532 Not Found!");
    display.display();
    while(1);
  }
  
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Configure PN532 for all card types
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0xFF);
  
  display.println("PN532 Ready!");
  display.display();
  delay(1000);
  
  displayMainMenu();
}

void loop() {
  handleInput();
  
  switch(currentMenu) {
    case MAIN_MENU:
      handleMainMenu();
      break;
    case READ_CARD:
      handleReadCard();
      break;
    case WRITE_CARD:
      handleWriteCard();
      break;
    case EMULATE_CARD:
      handleEmulateCard();
      break;
    case BRUTE_FORCE:
      handleBruteForce();
      break;
    case CARD_MANAGER:
      handleCardManager();
      break;
    case SETTINGS:
      handleSettings();
      break;
  }
  
  delay(100);
}

void resetButtons() {
  btnUpPressed = false;
  btnDownPressed = false;
  btnSelectPressed = false;
  btnBackPressed = false;
}

void handleInput() {
  // Read current button states
  bool btnUpState = digitalRead(BTN_UP);
  bool btnDownState = digitalRead(BTN_DOWN);
  bool btnSelectState = digitalRead(BTN_SELECT);
  bool btnBackState = digitalRead(BTN_BACK);
  
  unsigned long currentTime = millis();
  
  // Debounce and detect button presses (LOW = pressed due to pullup)
  if (currentTime - lastDebounceTime > debounceDelay) {
    
    // UP button
    if (btnUpState == LOW && btnUpLastState == HIGH) {
      btnUpPressed = true;
      lastDebounceTime = currentTime;
    }
    btnUpLastState = btnUpState;
    
    // DOWN button  
    if (btnDownState == LOW && btnDownLastState == HIGH) {
      btnDownPressed = true;
      lastDebounceTime = currentTime;
    }
    btnDownLastState = btnDownState;
    
    // SELECT button
    if (btnSelectState == LOW && btnSelectLastState == HIGH) {
      btnSelectPressed = true;
      lastDebounceTime = currentTime;
    }
    btnSelectLastState = btnSelectState;
    
    // BACK button
    if (btnBackState == LOW && btnBackLastState == HIGH) {
      btnBackPressed = true;
      lastDebounceTime = currentTime;
    }
    btnBackLastState = btnBackState;
  }
  
  // Process button presses
  if (btnUpPressed) {
    btnUpPressed = false;
    menuSelection = (menuSelection > 0) ? menuSelection - 1 : getMaxMenuItems() - 1;
    displayCurrentMenu();
  }
  
  if (btnDownPressed) {
    btnDownPressed = false;
    menuSelection = (menuSelection < getMaxMenuItems() - 1) ? menuSelection + 1 : 0;
    displayCurrentMenu();
  }
  
  if (btnSelectPressed) {
    btnSelectPressed = false;
    selectMenuItem();
  }
  
  if (btnBackPressed) {
    // btnBackPressed is reset in the new function
    if (currentMenu != MAIN_MENU) {
      currentMenu = MAIN_MENU;
      menuSelection = 0;
      resetButtons();
      displayMainMenu();
    }
  }
}

int getMaxMenuItems() {
  switch(currentMenu) {
    case MAIN_MENU:
      return 6;
    case CARD_MANAGER:
      return totalCards > 0 ? totalCards : 1;
    case SETTINGS:
      return 4;
    default:
      return 1;
  }
}

// Icon definitions (8x8 pixels)
const unsigned char read_icon[] PROGMEM = {
    0x00, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x00
};
const unsigned char write_icon[] PROGMEM = {
    0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x00, 0x00
};
const unsigned char emulate_icon[] PROGMEM = {
    0x3c, 0x42, 0x99, 0xa5, 0xa5, 0x99, 0x42, 0x3c
};
const unsigned char brute_icon[] PROGMEM = {
    0x00, 0x00, 0x7e, 0x08, 0x08, 0x7e, 0x00, 0x00
};
const unsigned char manager_icon[] PROGMEM = {
    0x00, 0x3e, 0x4a, 0x4a, 0x4a, 0x3e, 0x00, 0x00
};
const unsigned char settings_icon[] PROGMEM = {
    0x00, 0x1c, 0x22, 0x7f, 0x22, 0x1c, 0x00, 0x00
};

const unsigned char* menu_icons[] = {
    read_icon, write_icon, emulate_icon, brute_icon, manager_icon, settings_icon
};

void displayMainMenu() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("======= nfcGOD =======");
  
  const char* menuItems[] = {"Read Card", "Write Card", "Emulate", "Brute Force", "Manager", "Settings"};
  
  for(int i = 0; i < 6; i++) {
    if(i == menuSelection) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.drawBitmap(20, 10 + i * 9, menu_icons[i], 8, 8, 1);
    display.setCursor(35, 10 + i * 9);
    display.println(menuItems[i]);
  }
  
  display.setCursor(0, 56);
  display.print("Cards: ");
  display.println(totalCards);
  
  display.display();
}

void displayCurrentMenu() {
  switch(currentMenu) {
    case MAIN_MENU:
      displayMainMenu();
      break;
    case SETTINGS:
      displaySettingsMenu();
      break;
    // Add other menu displays as needed
  }
}

void selectMenuItem() {
  // Reset button state on menu action
  resetButtons();

  switch(currentMenu) {
    case MAIN_MENU:
      switch(menuSelection) {
        case 0:
          currentMenu = READ_CARD;
          startReadCard();
          break;
        case 1:
          currentMenu = WRITE_CARD;
          startWriteCard();
          break;
        case 2:
          currentMenu = EMULATE_CARD;
          startEmulateCard();
          break;
        case 3:
          currentMenu = BRUTE_FORCE;
          startBruteForce();
          break;
        case 4:
          currentMenu = CARD_MANAGER;
          startCardManager();
          break;
        case 5:
          currentMenu = SETTINGS;
          startSettings();
          break;
      }
      break;
  }
}

void handleMainMenu() {
  // Check for card presence
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    if (!cardPresent) {
      cardPresent = true;
      displayCardDetected(uid, uidLength);
    }
  } else {
    cardPresent = false;
  }
}

void displayCardDetected(uint8_t* uid, uint8_t uidLength) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("CARD DETECTED!");
  display.println();
  display.print("UID: ");
  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) display.print("0");
    display.print(uid[i], HEX);
    if (i < uidLength - 1) display.print(":");
  }
  display.println();
  display.println();
  display.println("Press SELECT to read");
  display.display();
}

void startReadCard() {
  showLoading("Initializing NFC...", 200);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=== READ CARD ===");
  display.println();
  display.println("Place card near");
  display.println("reader...");
  display.display();
}

void handleReadCard() {
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    // Copy UID
    currentCard.uidLength = uidLength;
    memcpy(currentCard.uid, uid, uidLength);
    
    // Determine card type and read data
    if (readCardData()) {
      saveCardToSD();
      displayReadSuccess();
    } else {
      displayReadError();
    }
    
    delay(2000);
    resetButtons();
    currentMenu = MAIN_MENU;
    displayMainMenu();
  }
}

bool readCardData() {
  // Try to authenticate and read Mifare Classic
  if (readMifareClassic()) {
    currentCard.cardType = 1; // Mifare Classic
    return true;
  }
  
  // Try to read Mifare Ultralight
  if (readMifareUltralight()) {
    currentCard.cardType = 2; // Mifare Ultralight
    return true;
  }
  
  // Try to read NTAG
  if (readNTAG()) {
    currentCard.cardType = 3; // NTAG
    return true;
  }
  
  // ISO14443-4 (Type A) is not supported in this version.
  
  return false;
}

bool readMifareClassic() {
  // Mifare Classic 1K/4K reading logic
  uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  uint8_t keyB[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  
  currentCard.dataLength = 0;
  
  // Try to read all sectors with default keys
  for (uint8_t sector = 0; sector < 16; sector++) {
    for (uint8_t block = 0; block < 4; block++) {
      uint8_t blockNum = sector * 4 + block;
      uint8_t blockData[16];
      
      if (nfc.mifareclassic_AuthenticateBlock(currentCard.uid, currentCard.uidLength, blockNum, 0, keyA)) {
        if (nfc.mifareclassic_ReadDataBlock(blockNum, blockData)) {
          memcpy(&currentCard.data[currentCard.dataLength], blockData, 16);
          currentCard.dataLength += 16;
        }
      }
    }
  }
  
  return currentCard.dataLength > 0;
}

bool writeMifareClassic(uint8_t* uid, uint8_t uidLength, CardData* card) {
  uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // Default key
  
  // Don't write to sector 0, block 0 (manufacturer block)
  for (uint16_t i = 16; i < card->dataLength; i += 16) {
    uint8_t blockNum = i / 16;
    
    // Skip trailer blocks
    if ((blockNum + 1) % 4 == 0) continue;

    // Authenticate
    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNum, 0, keyA)) {
      Serial.print("Auth failed for block "); Serial.println(blockNum);
      return false;
    }
    
    // Write data
    if (!nfc.mifareclassic_WriteDataBlock(blockNum, &card->data[i])) {
      Serial.print("Write failed for block "); Serial.println(blockNum);
      return false;
    }
  }
  return true;
}

bool readMifareUltralight() {
  // Mifare Ultralight reading logic
  currentCard.dataLength = 0;
  
  for (uint8_t page = 0; page < 16; page++) {
    uint8_t pageData[4];
    if (nfc.mifareultralight_ReadPage(page, pageData)) {
      memcpy(&currentCard.data[currentCard.dataLength], pageData, 4);
      currentCard.dataLength += 4;
    } else {
      break;
    }
  }
  
  return currentCard.dataLength > 0;
}

bool readNTAG() {
  // NTAG reading logic (similar to Ultralight but different page count)
  currentCard.dataLength = 0;
  
  for (uint8_t page = 0; page < 45; page++) { // NTAG213 has 45 pages
    uint8_t pageData[4];
    if (nfc.mifareultralight_ReadPage(page, pageData)) {
      memcpy(&currentCard.data[currentCard.dataLength], pageData, 4);
      currentCard.dataLength += 4;
    } else {
      break;
    }
  }
  
  return currentCard.dataLength > 0;
}

void saveCardToSD() {
  // Generate filename based on UID
  String filename = CARD_DIR;
  for (uint8_t i = 0; i < currentCard.uidLength; i++) {
    if (currentCard.uid[i] < 0x10) filename += "0";
    filename += String(currentCard.uid[i], HEX);
  }
  filename += ".nfc";
  
  File cardFile = SD.open(filename.c_str(), FILE_WRITE);
  if (cardFile) {
    // Write card header
    cardFile.write(currentCard.uidLength);
    cardFile.write(currentCard.uid, currentCard.uidLength);
    cardFile.write(currentCard.cardType);
    cardFile.write((uint8_t)(currentCard.dataLength & 0xFF));
    cardFile.write((uint8_t)(currentCard.dataLength >> 8));
    
    // Write card data
    cardFile.write(currentCard.data, currentCard.dataLength);
    
    cardFile.close();
    
    strcpy(currentCard.filename, filename.c_str());
    currentCard.isValid = true;
    
    countCards();
    
    Serial.println("Card saved: " + filename);
  }
}

String getIssuerName(uint8_t* uid, uint8_t uidLength) {
  if (uidLength > 0) {
    switch (uid[0]) {
      case 0x04: return "NXP";
      case 0x05: return "Infineon";
      case 0x07: return "Texas Instruments";
      default: return "Unknown";
    }
  }
  return "Unknown";
}

void displayReadSuccess() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("--- CARD DETAILS ---");
  
  display.print("Type: ");
  display.println(getCardTypeName(currentCard.cardType));

  display.print("UID: ");
  for (uint8_t i = 0; i < currentCard.uidLength; i++) {
    if (currentCard.uid[i] < 0x10) display.print("0");
    display.print(currentCard.uid[i], HEX);
  }
  display.println();

  display.print("Issuer: ");
  display.println(getIssuerName(currentCard.uid, currentCard.uidLength));

  display.print("Size: ");
  display.print(currentCard.dataLength);
  display.println(" bytes");

  display.println();
  display.println("Saved to SD card");
  display.display();
}

void displayReadError() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("READ FAILED!");
  display.println();
  display.println("Unsupported card");
  display.println("or read error");
  display.display();
}

String getCardTypeName(uint8_t cardType) {
  switch(cardType) {
    case 1: return "Mifare Classic";
    case 2: return "Mifare UL";
    case 3: return "NTAG";
    case 4: return "ISO14443-4A";
    default: return "Unknown";
  }
}

void startWriteCard() {
  showLoading("Preparing write...", 300);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=== WRITE CARD ===");
  display.println();
  display.println("Select card from");
  display.println("SD to write...");
  display.display();
}

void listSDFiles(String* files, int* count, const char* extension, int maxFiles) {
  *count = 0;
  File root = SD.open(CARD_DIR);
  if (!root) return;

  File entry;
  while ((entry = root.openNextFile()) && (*count < maxFiles)) {
    String n = entry.name();
    if (!entry.isDirectory() && n.endsWith(extension)) {
      files[*count] = n;
      (*count)++;
    }
    entry.close();
  }
  root.close();
}

void handleWriteCard() {
  static int sel = 0;
  static String files[16];
  static int fileCount = 0;
  static bool fileSelected = false;
  static CardData cardToWrite;

  // --- Step 1: File Selection ---
  if (!fileSelected) {
    showLoading("Loading card list...", 200);
    // Populate file list if empty
    if (fileCount == 0) {
      listSDFiles(files, &fileCount, ".nfc", 16);
    }

    // Draw file list
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("SELECT CARD TO WRITE");
    if (fileCount == 0) {
      display.println("No files found!");
    } else {
      for (int i = 0; i < fileCount; i++) {
        display.setCursor(0, 16 + i*8);
        display.print(i == sel ? "> " : "  ");
        display.println(files[i]);
      }
    }
    display.display();

    // Handle navigation
    if (btnUpPressed)    sel = (sel + fileCount - 1) % fileCount;
    if (btnDownPressed)  sel = (sel + 1) % fileCount;
    if (btnBackPressed)  { fileCount = sel = 0; resetButtons(); currentMenu = MAIN_MENU; displayMainMenu(); return; }

    // Handle selection
    if (btnSelectPressed) {
      if (fileCount > 0) {
        String filepath = String(CARD_DIR) + files[sel];
        if (loadCardFromSD(filepath.c_str(), &cardToWrite)) {
          fileSelected = true;
          // Reset button press to avoid double-triggering
          btnSelectPressed = false; 
        } else {
          display.clearDisplay();
          display.println("Load failed!");
          display.display();
          delay(1000);
        }
      }
    }
    return; // Wait for user to select a file
  }

  // --- Step 2: Write to Card ---
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Place blank card");
  display.println("on reader...");
  display.display();

  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  // Wait for a card
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("Found card to write to.");
    
    bool success = false;
    // For now, only implement Mifare Classic writing
    if (cardToWrite.cardType == 1) { 
      success = writeMifareClassic(uid, uidLength, &cardToWrite);
    } else {
      display.println("Unsupported type");
    }

    if (success) {
      display.clearDisplay();
      display.println("WRITE SUCCESS!");
      display.display();
    } else {
      display.clearDisplay();
      display.println("WRITE FAILED!");
      display.display();
    }
    
    delay(2000);
    // Reset state for next time
    fileCount = sel = 0;
    fileSelected = false;
    resetButtons();
    currentMenu = MAIN_MENU;
    displayMainMenu();
  }
  
  // Handle back press during write wait
  if (btnBackPressed) {
      fileCount = sel = 0;
      fileSelected = false;
      resetButtons();
      currentMenu = MAIN_MENU; 
      displayMainMenu(); 
      return;
  }
}

void startBruteForce() {
  showLoading("Preparing brute force...", 500);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=== BRUTE FORCE ===");
  display.println();
  display.println("Place Mifare Classic");
  display.println("card on reader...");
  display.println();
  display.println("Will try common keys");
  display.println("for all sectors");
  display.display();
  
  // Initialize brute force state
  bruteForce.isActive = false;
  bruteForce.currentKeyIndex = 0;
  bruteForce.currentSector = 0;
  bruteForce.totalAttempts = 0;
  bruteForce.successfulSectors = 0;
  
  for(int i = 0; i < 16; i++) {
    bruteForce.keyFound[i] = false;
  }
}

/**
 * Handle brute force menu option.
 *
 * If the brute force attack is not running, check for a card
 * presence and start the brute force attack if a card is
 * detected. If the brute force attack is running, continue
 * the attack.
 */
void handleBruteForce() {
  if (!bruteForce.isActive) {
    // Check for card presence
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      // Store target card info
      bruteForce.targetUIDLength = uidLength;
      memcpy(bruteForce.targetUID, uid, uidLength);
      
      // Start brute force attack
      bruteForce.isActive = true;
      bruteForce.startTime = millis();
      
      displayBruteForceStarted();
    }
  } else {
    // Continue brute force attack
    performBruteForceStep();
  }
}

void displayBruteForceStarted() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("BRUTE FORCE ACTIVE");
  display.println();
  display.print("UID: ");
  for (uint8_t i = 0; i < bruteForce.targetUIDLength; i++) {
    if (bruteForce.targetUID[i] < 0x10) display.print("0");
    display.print(bruteForce.targetUID[i], HEX);
  }
  display.println();
  display.println();
  display.println("Attacking sectors...");
  display.println("Press BACK to stop");
  display.display();
}

/**
 * Continue the brute force attack on the current sector.
 *
 * This function will try the current key on the current sector, and if
 * successful, store the key and move to the next sector. If all keys have
 * been tried for this sector without success, move to the next sector.
 *
 * If all sectors have been done, stop the brute force attack and display the
 * results.
 */
void performBruteForceStep() {
  // Check if we're done with all sectors
  if (bruteForce.currentSector >= 16) {
    bruteForce.isActive = false;
    displayBruteForceResults();
    return;
  }
  
  // Skip if we already found the key for this sector
  if (bruteForce.keyFound[bruteForce.currentSector]) {
    bruteForce.currentSector++;
    bruteForce.currentKeyIndex = 0;
    return;
  }
  
  // Try current key on current sector
  uint8_t blockNum = bruteForce.currentSector * 4; // First block of sector
  
  if (nfc.mifareclassic_AuthenticateBlock(bruteForce.targetUID, bruteForce.targetUIDLength, 
                                         blockNum, 0, (uint8_t*)commonKeys[bruteForce.currentKeyIndex])) {
    // Key found!
    memcpy(bruteForce.foundKeys[bruteForce.currentSector], commonKeys[bruteForce.currentKeyIndex], 6);
    bruteForce.keyFound[bruteForce.currentSector] = true;
    bruteForce.successfulSectors++;
    
    Serial.print("Key found for sector ");
    Serial.print(bruteForce.currentSector);
    Serial.print(": ");
    printKey(commonKeys[bruteForce.currentKeyIndex]);
    
    // Move to next sector
    bruteForce.currentSector++;
    bruteForce.currentKeyIndex = 0;
  } else {
    // Try next key
    bruteForce.currentKeyIndex++;
    
    // If we've tried all keys for this sector, move to next sector
    if (bruteForce.currentKeyIndex >= numCommonKeys) {
      bruteForce.currentSector++;
      bruteForce.currentKeyIndex = 0;
    }
  }
  
  bruteForce.totalAttempts++;
  
  // Update display every 10 attempts
  if (bruteForce.totalAttempts % 10 == 0) {
    displayBruteForceProgress();
  }
}

/**
 * Update the display with the current progress of a brute force attack.
 *
 * This function will display the current sector being attacked, the
 * current key index, the number of sectors cracked so far, and the
 * total number of attempts made.
 */
void displayBruteForceProgress() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("BRUTE FORCE ACTIVE");
  display.println();
  display.print("Sector: ");
  display.print(bruteForce.currentSector);
  display.print("/16");
  display.println();
  display.print("Key: ");
  display.print(bruteForce.currentKeyIndex + 1);
  display.print("/");
  display.println(numCommonKeys);
  display.print("Found: ");
  display.println(bruteForce.successfulSectors);
  display.println();
  display.print("Attempts: ");
  display.println(bruteForce.totalAttempts);
  display.display();
}

/**
 * Display the results of a brute force attack on the display.
 *
 * This function will display the number of sectors cracked, the total
 * number of attempts, and the time taken for the brute force attack.
 * It will also print detailed results to the serial console and save
 * the results to an SD card.
 */
void displayBruteForceResults() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("BRUTE FORCE DONE!");
  display.println();
  display.print("Sectors cracked: ");
  display.print(bruteForce.successfulSectors);
  display.println("/16");
  display.print("Total attempts: ");
  display.println(bruteForce.totalAttempts);
  
  uint32_t elapsed = (millis() - bruteForce.startTime) / 1000;
  display.print("Time: ");
  display.print(elapsed);
  display.println("s");
  
  display.println();
  display.println("Press BACK for menu");
  display.display();
  
  // Print detailed results to serial
  Serial.println("\n=== BRUTE FORCE RESULTS ===");
  for(int i = 0; i < 16; i++) {
    if(bruteForce.keyFound[i]) {
      Serial.print("Sector ");
      Serial.print(i);
      Serial.print(": ");
      printKey(bruteForce.foundKeys[i]);
    }
  }
  
  // Save results to SD card
  saveBruteForceResults();
}

/**
 * Print a Mifare Classic key to the serial console in hexadecimal format.
 * Each byte of the key is printed in two-digit hexadecimal format, separated
 * by colons.
 * @param key 6-byte array containing the Mifare Classic key
 */
void printKey(const uint8_t* key) {
  for(int i = 0; i < 6; i++) {
    if(key[i] < 0x10) Serial.print("0");
    Serial.print(key[i], HEX);
    if(i < 5) Serial.print(":");
  }
  Serial.println();
}

/**
 * Save the results of a brute force operation to an SD card.
 *
 * This function saves the details of a brute force attack that was performed
 * on a Mifare Classic card. It generates a filename based on the current
 * time in seconds and writes the UID of the card and the keys found for
 * each sector to the file. Each line in the file contains a sector number
 * followed by the corresponding key in hexadecimal format, separated by
 * colons.
 */

void saveBruteForceResults() {
  // e.g. "/cards/brute_123456.txt"
  String fn = String(CARD_DIR) + "brute_" + String(millis()/1000) + ".txt";
  File f = SD.open(fn.c_str(), FILE_WRITE);
  if (!f) return;

  // write header
  f.print("UID: ");
  for (uint8_t i = 0; i < bruteForce.targetUIDLength; i++) {
    if (bruteForce.targetUID[i] < 0x10) f.print("0");
    f.print(bruteForce.targetUID[i], HEX);
    if (i < bruteForce.targetUIDLength-1) f.print(":");
  }
  f.println();
  f.println("Sector:Key");
  // write each cracked sector
  for (uint8_t s = 0; s < 16; s++) {
    if (bruteForce.keyFound[s]) {
      f.print(s); f.print(": ");
      for (uint8_t b = 0; b < 6; b++) {
        if (bruteForce.foundKeys[s][b] < 0x10) f.print("0");
        f.print(bruteForce.foundKeys[s][b], HEX);
        if (b<5) f.print(":");
      }
      f.println();
    }
  }
  f.close();
}

/**
 * @brief Display the Emulate Card menu
 *
 * This function displays the menu for the "Emulate Card" option. It shows a
 * list of .nfc files found on the SD card and lets the user select one. Once a
 * file is selected, it is loaded into memory and the ESP32 starts to emulate a
 * card.
 */
void startEmulateCard() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=== READER DETECT ===");
  display.println();
  display.println("Select card to");
  display.println("detect reader for...");
  display.display();
}

/**
 * @brief Handle the Emulate Card menu option
 *
 * This function handles the "Emulate Card" menu option. It first shows a list of
 * .nfc files found on the SD card and lets the user select one. Once a file is
 * selected, it is loaded into memory and the ESP32 starts to emulate a card
 * with that data. The user is then shown a screen saying "Emulating card..."
 * and the device waits for an external reader to detect it. If a reader is
 * detected, the user is shown a success message. If no reader is detected after
 * a short time, the user is shown a failure message. Finally, the device returns
 * to the main menu.
 */
void handleEmulateCard() {
  static int sel = 0;
  static String files[16];
  static int fileCount = 0;
  static bool fileSelected = false;

  // --- Step 1: File Selection ---
  if (!fileSelected) {
    if (fileCount == 0) {
      listSDFiles(files, &fileCount, ".nfc", 16);
    }

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("SELECT CARD TO EMULATE");
    if (fileCount == 0) {
      display.println("No files found!");
    } else {
      for (int i = 0; i < fileCount; i++) {
        display.setCursor(0, 16 + i*8);
        display.print(i == sel ? "> " : "  ");
        display.println(files[i]);
      }
    }
    display.display();

    if (btnUpPressed)    sel = (sel + fileCount - 1) % fileCount;
    if (btnDownPressed)  sel = (sel + 1) % fileCount;
    if (btnBackPressed)  { fileCount = sel = 0; currentMenu = MAIN_MENU; displayMainMenu(); return; }

    if (btnSelectPressed) {
      if (fileCount > 0) {
        fileSelected = true;
        btnSelectPressed = false; 
      }
    }
    return;
  }

  // --- Step 2: Emulate Card ---
  CardData cardToEmulate;
  String filepath = String(CARD_DIR) + files[sel];
  if (!loadCardFromSD(filepath.c_str(), &cardToEmulate)) {
    display.clearDisplay();
    display.println("Load failed!");
    display.display();
    delay(1000);
    fileCount = sel = 0;
    fileSelected = false;
    resetButtons();
    currentMenu = MAIN_MENU;
    displayMainMenu();
    return;
  }

  // The tgInitAsTarget command is blocking and waits for an external reader.
  // We need to handle the display and button presses differently.
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Emulating card...");
  display.println(files[sel]);
  display.println();
  display.println("Press BACK to stop.");
  display.display();

  // Start reader detection
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Detecting reader...");
  display.display();
  
  bool readerDetected = false;
  while (digitalRead(BTN_BACK) == HIGH) {
    if (detectExternalReader()) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Reader detected!");
      display.display();
      readerDetected = true;
      delay(1000);
      break;
    }
    delay(50);
  }

  if (!readerDetected) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("No reader detected");
    display.display();
    delay(1000);
  }

  // Reset state and return to menu
  fileCount = sel = 0;
  fileSelected = false;
  resetButtons();
  currentMenu = MAIN_MENU;
  displayMainMenu();
}

void startCardManager() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("=== CARD MANAGER ===");
  display.println();
  display.print("Total cards: ");
  display.println(totalCards);
  display.println();
  display.println("Navigate with u/d");
  display.display();
}

/**
 * @brief Handle the card manager menu state.
 *
 * This function is the main event loop for the card manager menu.
 * It draws the list of cards, handles navigation, and handles the
 * deletion of cards on the SELECT button press.
 */
void handleCardManager() {
  static int sel = 0;
  static String files[16];
  static int fileCount = 0;

  if (fileCount == 0) {
    listSDFiles(files, &fileCount, ".nfc", 16);
  }

  // draw list
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("CARD MANAGER");
  for (int i = 0; i < fileCount; i++) {
    display.setCursor(0, 16 + i*8);
    display.print(i == sel ? "> " : "  ");
    display.println(files[i]);
  }
  display.display();

  // nav
  if (btnUpPressed)    sel = (sel + fileCount - 1) % fileCount;
  if (btnDownPressed)  sel = (sel + 1) % fileCount;
  if (btnBackPressed)  { fileCount = sel = 0; resetButtons(); currentMenu = MAIN_MENU; displayMainMenu(); return; }

  // delete on SELECT
  if (btnSelectPressed) {
    String path = String(CARD_DIR) + files[sel];
    SD.remove(path.c_str());
    countCards();    // refresh the main-menu tally
    fileCount = sel = 0;
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Deleted!");
    display.display();
    delay(1000);
    resetButtons();
    currentMenu = MAIN_MENU; displayMainMenu();
  }
}

void displaySettingsMenu() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("====== SETTINGS ======");
    
    const char* menuItems[] = {"Brightness", "Debug", "Format SD", "About"};
    
    for (int i = 0; i < 4; i++) {
        if (i == menuSelection) {
            display.print("> ");
        } else {
            display.print("  ");
        }
        display.print(menuItems[i]);

        if (i == 0) { // Brightness
            display.print(": ");
            display.print(displayContrast);
        } else if (i == 1) { // Debug
            display.print(debugMode ? ": ON" : ": OFF");
        }
        display.println();
    }
    display.display();
}

void startSettings() {
  menuSelection = 0;
  displaySettingsMenu();
}

void handleSettings() {
  if (btnSelectPressed) {
    resetButtons();
    switch (menuSelection) {
      case 0: // Brightness
        // Cycle through brightness levels
        displayContrast = (displayContrast == 255) ? 64 : (displayContrast + 64);
        if(displayContrast == 0) displayContrast = 1;
        if(displayContrast > 255) displayContrast = 255;
        display.ssd1306_command(SSD1306_SETCONTRAST);
        display.ssd1306_command(displayContrast);
        displaySettingsMenu();
        break;
      case 1: // Debug
        debugMode = !debugMode;
        displaySettingsMenu();
        break;
      case 2: // Format SD
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("FORMATTING SD...");
        display.println("This is permanent!");
        display.println("Press SELECT again");
        display.println("to confirm.");
        display.display();
        // Wait for second confirm
        while(digitalRead(BTN_SELECT) == HIGH) {
          if(digitalRead(BTN_BACK) == LOW) {
            displaySettingsMenu();
            return;
          }
        }
        // Actually format
        // Note: This is a simple format, just deleting files.
        {
          File root = SD.open(CARD_DIR);
          while(true) {
            File entry = root.openNextFile();
            if(!entry) break;
            String path = String(CARD_DIR) + entry.name();
            SD.remove(path.c_str());
            entry.close();
          }
          root.close();
        }
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Format Complete!");
        display.display();
        delay(1000);
        countCards(); // Recount cards (should be 0)
        displaySettingsMenu();
        break;
      case 3: // About
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("NFC Multitool v1.0");
        display.println("By: Your Name");
        display.println("Built with Cline");
        display.println();
        display.println("Press BACK to exit");
        display.display();
        while(digitalRead(BTN_BACK) == HIGH); // wait for back
        displaySettingsMenu();
        break;
    }
  }
  
  if (btnBackPressed) {
    resetButtons();
    currentMenu = MAIN_MENU;
    menuSelection = 0;
    displayMainMenu();
  }
}

// ——— ADD ABOVE countCards() ———
bool loadCardFromSD(const char* filename, CardData* card) {
  File f = SD.open(filename, FILE_READ);
  if (!f) return false;

  // read UID length + UID
  card->uidLength = f.read();
  f.read(card->uid, card->uidLength);

  // read card type
  card->cardType = f.read();

  // read dataLength (little-endian)
  uint16_t lo = f.read();
  uint16_t hi = f.read();
  card->dataLength = lo | (hi << 8);

  // read data blob
  f.read(card->data, card->dataLength);

  card->isValid = true;
  f.close();
  return true;
}

// ——— ABOVE countCards() ———
bool detectExternalReader() {
  return (nfc.inListPassiveTarget() > 0);
}

void stopCardEmulation() {
  // No action needed for simple detection
}
// ————————————————————

/**
 * Counts the number of .nfc files in the CARD_DIR directory
 * and stores the count in the totalCards variable.
 */
void showLoading(const char* msg, uint16_t duration_ms = 500) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(msg);
  display.println();
  display.print("[");
  for (int i = 0; i < 10; i++) {
    display.print(">");
    display.display();
    delay(duration_ms/10);
  }
  display.println("]");
  display.display();
}

void countCards() {
  totalCards = 0;
  File root = SD.open(CARD_DIR);
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      if (!entry.isDirectory() && String(entry.name()).endsWith(".nfc")) {
        totalCards++;
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
}
