#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

// LCD Configuration
#define SDA_PIN 21  // I2C SDA
#define SCL_PIN 22  // I2C SCL
#define lcdColumns 16
#define lcdRows 2
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Keypad Configuration
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', 'D', '#'}
};

byte rowPins[ROWS] = {18, 19, 23, 5};
byte colPins[COLS] = {13, 12, 14, 27};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Password Configuration
String password = "336";
String inputPassword = "";
const int maxPasswordLength = 4;
int failedAttempts = 0; // Track failed attempts

// RFID Configuration
#define SCK_PIN_RFID 2
#define MOSI_PIN_RFID 15
#define MISO_PIN_RFID 32
#define RST_PIN_RFID 33
#define SS_PIN_RFID 4

MFRC522 rfid(SS_PIN_RFID, RST_PIN_RFID);

// Define valid UIDs
byte validUIDs[][4] = {
    {0x29, 0xAE, 0x24, 0xD9},
    {0x96, 0x54, 0xDB, 0x2B}
};
const int numberOfUIDs = sizeof(validUIDs) / sizeof(validUIDs[0]);

void setup() {
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(lcdColumns, lcdRows);
  lcd.backlight();
  lcd.clear();
  lcd.print("Enter Password:");
  
  SPI.begin(SCK_PIN_RFID, MISO_PIN_RFID, MOSI_PIN_RFID, SS_PIN_RFID);
  rfid.PCD_Init();
  lcd.clear();
  lcd.print("Scan RFID tag");
}

void loop() {
  digitalWrite(25, HIGH);
  digitalWrite(26, HIGH);
  
  handleKeypad();
  
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (isValidUID(rfid.uid.uidByte, rfid.uid.size)) {
      grantAccess("RFID Access Granted");
    } else {
      failedAttempts++;
      if (failedAttempts >= 3) {
        lockOutUser();
      } else {
        accessDenied("Invalid RFID");
      }
    }
    rfid.PICC_HaltA();
  }
}

void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    if (key == 'D') { // Submit password
      if (inputPassword == password) {
        grantAccess("Access Granted");
      } else {
        failedAttempts++;
        if (failedAttempts >= 3) {
          lockOutUser();
        } else {
          accessDenied("Invalid Password");
        }
      }
    } else if (key == 'A') { // Clear input
      resetPasswordInput();
    } else if (key == 'B') { // Delete last character
      if (inputPassword.length() > 0) {
        inputPassword.remove(inputPassword.length() - 1);
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print(inputPassword);
      }
    } else { // Append key to input
      if (inputPassword.length() < maxPasswordLength) {
        inputPassword += key;
        lcd.setCursor(0, 1);
        lcd.print(inputPassword);
      }
    }
  }
}

bool isValidUID(byte *uid, byte size) {
  for (int i = 0; i < numberOfUIDs; i++) {
    bool match = true;
    for (byte j = 0; j < size; j++) {
      if (uid[j] != validUIDs[i][j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

void grantAccess(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
  digitalWrite(25, LOW);
  digitalWrite(26, LOW);
  delay(5000);
  resetPasswordInput();
}

void accessDenied(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
  delay(2000);
  lcd.clear();
  lcd.print("Enter Password:");
}

void lockOutUser() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Too many attempts!");
  delay(5000);  // Lock out for 5 seconds
  failedAttempts = 0;
  resetPasswordInput();
}

void resetPasswordInput() {
  inputPassword = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");
}

