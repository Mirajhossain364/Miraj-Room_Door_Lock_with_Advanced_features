#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <IRremote.h>
#include <Servo.h>

// Pin definitions
const int irPin = 4;
const int wledPin = 3;
const int ledPin = 6;
const int servoPin = 5;
const int rfidSSPin = 10;
const int rfidRstPin = 9;
const int keypadRows[4] = {A0, A1, A2, 8};
const int keypadCols[4] = {A3, A4, A5, 7};
const int irReceiverPin = 2;

// Password setup
const int passwordLength = 8;
char enteredPassword[passwordLength];
char masterPassword[passwordLength] = "D10659A";

// Servo setup
Servo lockServo;

// Keypad setup
Keypad keypad = Keypad(makeKeymap({
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
}), keypadRows, keypadCols, 4, 4);

// RFID setup
MFRC522 mfrc522(rfidSSPin, rfidRstPin);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// IR setup
IRrecv irReceiver(irReceiverPin);
decode_results irResults;

// Timers
unsigned long unlockDelay = 5000; // Delay to keep the door unlocked for 5 seconds
unsigned long lastUnlockTime = 0;
unsigned long debounceDelay = 300; // Delay to prevent multiple entries of the same key/button
unsigned long lastDebounceTime = 0;  // Variable to store the last time the IR was pressed or keypad was used
bool unlockingInProgress = false;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  irReceiver.enableIRIn();

  lockServo.attach(servoPin);
  pinMode(wledPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Serial.println("System initialized. Awaiting input...");
}

void loop() {
  checkProximity();
  checkIRRemote();
  checkKeypad();
  checkRFID();

  // Non-blocking unlock logic
  if (unlockingInProgress && (millis() - lastUnlockTime >= unlockDelay)) {
    lock(); // Lock the door after the delay
    unlockingInProgress = false;  // Reset the unlocking flag
  }
}

void checkProximity() {
  if (digitalRead(irPin) == LOW) {  // Proximity detected
    Serial.println("Access granted via proximity sensor.");
    unlock();
  }
}

void checkIRRemote() {
  if (irReceiver.decode(&irResults)) {
    Serial.println("IR Command: " + String(irResults.value, HEX));

    // Check for authorized IR commands (replace with your IR codes)
    long authorizedIRCodes[] = {0x807FC837, 0xC3525B4D, 0x8823BAAD, 0x6FB95898};
    for (int i = 0; i < sizeof(authorizedIRCodes) / sizeof(authorizedIRCodes[0]); i++) {
      if (irResults.value == authorizedIRCodes[i]) {
        Serial.println("Access granted via IR remote.");
        unlock();
        break;
      }
    }

    irReceiver.resume(); // Ready for next IR input

    // Implement debounce delay for IR signal
    lastDebounceTime = millis();
  }
}

void checkKeypad() {
  char key = keypad.getKey();
  if (key) {
    Serial.println("Key Pressed: " + String(key));
    enteredPassword[strlen(enteredPassword)] = key;
    digitalWrite(wledPin, HIGH);  // Turn on LED while entering password

    if (strlen(enteredPassword) == passwordLength - 1) {
      validatePassword();
    }

    // Implement debounce delay for keypad input
    lastDebounceTime = millis();
  }
}

void validatePassword() {
  if (strcmp(enteredPassword, masterPassword) == 0) {
    Serial.println("Correct password. Access granted.");
    unlock();
  } else {
    Serial.println("Incorrect password.");
  }
  clearEnteredPassword();
}

void clearEnteredPassword() {
  memset(enteredPassword, 0, sizeof(enteredPassword));
  digitalWrite(wledPin, LOW);  // Turn off LED
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardUID += String(mfrc522.uid.uidByte[i], HEX) + " ";
  }
  
  Serial.println("Scanned RFID Card UID: " + cardUID);

  if (cardUID.equals("D3 29 2E C0") || cardUID.equals("4A AA 27 B0")) {  // Replace with valid card UID
    Serial.println("Authorized access via RFID.");
    unlock();
  } else {
    Serial.println("Unauthorized RFID access.");
  }
}

void unlock() {
  if (!unlockingInProgress) {
    lockServo.write(120);  // Unlock the door (rotate servo)
    digitalWrite(ledPin, HIGH);  // Turn on LED to indicate unlock
    lastUnlockTime = millis();  // Start the unlock timer
    unlockingInProgress = true;  // Set flag indicating unlocking is in progress
  }
}

void lock() {
  lockServo.write(30);  // Lock the door (rotate servo back)
  digitalWrite(ledPin, LOW);  // Turn off LED
}
