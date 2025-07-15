/***************************************************
 *
 * Arduino RFID Parking Access Control
 * ***************************************************/

/* Include necessary libraries */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <RFID.h>
#include <Servo.h>
#include <EEPROM.h>

Servo barrierServo;                         // Create a servo object
LiquidCrystal_I2C lcd(0x27, 16, 2);         // Create an LCD object
RFID nfc(10, 5);                            // Create an RFID object for the card reader
int accessGranted = 0;                      // Card authorization variable
boolean passageCompleted = 0;               // Passage status variable
long duration;                              // Variable to store how long the Echo pin is active in microseconds
long distance = 0;                          // Variable for distance calculation
int triggerPin = 8;                         // Pin connected to the sensor's Trigger pin
int echoPin = 7;                            // Pin connected to the sensor's Echo pin
int ldrValue = 0;                           // Variable for LDR sensor value
int cardCount = 0;                          // Variable for the count of registered cards
int eepromAddress = 0;                      // Variable for EEPROM address

void setup() {
  SPI.begin();                              // Initialize SPI communication
  nfc.init();                               // Initialize the card reader
  lcd.init();                               // Initialize the LCD
  lcd.backlight();                          // Turn on the LCD backlight
  lcd.print("SCAN YOUR CARD");
  barrierServo.attach(9);                   // Specify the pin the servo is connected to
  barrierServo.write(180);                  // Set the servo to the initial position (block passage)
  pinMode(triggerPin, OUTPUT);              // Set the Trigger pin as an OUTPUT to apply voltage
  pinMode(echoPin, INPUT);                  // Set the Echo pin as an INPUT to read voltage
  digitalWrite(triggerPin, LOW);            // Deactivate the sensor initially
  Serial.begin(9600);                       // Start serial communication for debugging
  cardCount = EEPROM.read(0);               // Read the number of registered cards from EEPROM
  eepromAddress = (cardCount * 5) + 1;      // Calculate the next available EEPROM address
}

void loop() {
  ldrValue = analogRead(A0);                // Read the value from the LDR sensor
  if (ldrValue >= 400) {                    // Check if the ambient light is sufficient (system is active)
    lcd.clear();
    lcd.print("SCAN YOUR CARD");
    if (nfc.isCard()) {                     // Does the card reader detect an NFC card/device?
      if (nfc.readCardSerial()) {           // Was the card's UID successfully read?

        // Check if the scanned card is the Admin Card
        if (nfc.serNum[0] == 85 &&
            nfc.serNum[1] == 89 &&
            nfc.serNum[2] == 109 &&
            nfc.serNum[3] == 139 &&
            nfc.serNum[4] == 234) {
          lcd.clear();
          lcd.print("CARD REG. MODE");
          delay(1000);
          lcd.clear();
          lcd.print("SCAN NEW CARD");
          while (true) {
            if (nfc.isCard()) {             // Wait for a new card to be scanned
              if (nfc.readCardSerial()) {
                // If the Admin Card is scanned again in registration mode, wipe the EEPROM
                if (nfc.serNum[0] == 85 &&
                    nfc.serNum[1] == 89 &&
                    nfc.serNum[2] == 109 &&
                    nfc.serNum[3] == 139 &&
                    nfc.serNum[4] == 234) {
                  for (int i = 0; i < EEPROM.length(); i++) { // EEPROM Wipe
                    EEPROM.update(i, 0);
                  }
                  cardCount = 0;
                  EEPROM.write(0, cardCount);
                  lcd.clear();
                  lcd.print("ALL CARDS WIPED!");
                  delay(1500);
                } else {
                  // Register the new card
                  cardCount = EEPROM.read(0);
                  eepromAddress = (cardCount * 5) + 1;
                  EEPROM.write(eepromAddress, nfc.serNum[0]);     // Write the UID to EEPROM byte by byte
                  EEPROM.write(eepromAddress + 1, nfc.serNum[1]);
                  EEPROM.write(eepromAddress + 2, nfc.serNum[2]);
                  EEPROM.write(eepromAddress + 3, nfc.serNum[3]);
                  EEPROM.write(eepromAddress + 4, nfc.serNum[4]);
                  lcd.clear();
                  lcd.print("Card registered.");
                  cardCount = EEPROM.read(0) + 1;
                  EEPROM.write(0, cardCount); // Update the card count
                }
              }
              break; // Exit the registration loop
            }
          }
        } else {
          // If not the admin card, check if it's a registered user card
          for (int i = 0; i < cardCount; i++) {
            eepromAddress = (i * 5) + 1;
            if (nfc.serNum[0] == EEPROM.read(eepromAddress) &&
                nfc.serNum[1] == EEPROM.read(eepromAddress + 1) &&
                nfc.serNum[2] == EEPROM.read(eepromAddress + 2) &&
                nfc.serNum[3] == EEPROM.read(eepromAddress + 3) &&
                nfc.serNum[4] == EEPROM.read(eepromAddress + 4)) { // Is the UID same as a registered one?
              accessGranted = 1;
            }
          }
          if (accessGranted == 1) { // Has the UID been approved?
            barrierServo.write(90);   // Set servo to open position (allow passage)
            lcd.clear();              // Clear the LCD
            lcd.print("WELCOME!");
            
            // Wait for the vehicle to approach the barrier
            do {
              delay(100);
              digitalWrite(triggerPin, HIGH);
              delayMicroseconds(10);
              digitalWrite(triggerPin, LOW);
              
              // Record the time it takes for the echo to return using pulseIn function.
              duration = pulseIn(echoPin, HIGH);
              
              distance = duration / 29.1 / 2; /* Convert the measured time to distance */
              if (distance > 200) distance = 200; // Cap the distance
            } while (distance > 10);
            
            // Wait for the vehicle to pass the barrier
            do {
              delay(100);
              digitalWrite(triggerPin, HIGH);
              delayMicroseconds(10);
              digitalWrite(triggerPin, LOW);
              
              // Record the time it takes for the echo to return using pulseIn function.
              duration = pulseIn(echoPin, HIGH);
              
              distance = duration / 29.1 / 2; /* Convert the measured time to distance */
              if (distance > 200) distance = 200; // Cap the distance
            } while (distance <= 10);
            
            passageCompleted = 1;
            if (passageCompleted) {
              delay(2000);
              barrierServo.write(180); // Close the barrier
              lcd.clear();
              lcd.print("PASSAGE DONE!");
              accessGranted = 0;      // Reset variables
              passageCompleted = 0;
            }
          } else {
            lcd.clear();
            lcd.print("UNAUTHORIZED CARD!");
            delay(2000);
            lcd.clear();
            lcd.print("SCAN YOUR CARD");
          }
        }
      }
    }
    nfc.halt(); // Terminate RFID communication to save power
  } else {
    lcd.clear();
    lcd.print("SYSTEM CLOSED!"); // System is inactive in the dark
  }
  delay(1000);
}
