/*
  Underground Cable Fault Locator using VSWR Method
  This code implements a VSWR-based fault detection system for underground cables
  
  Hardware connections:
  - LCD: RS=12, E=11, D4=5, D5=4, D6=3, D7=2
  - Forward voltage input: A0 (from LM358 op-amp)
  - Reflected voltage input: A1 (from LM358 op-amp)
  - Test button: Pin 7
  - Buzzer: Pin 8
  - LED indicator: Pin 13
  
  Author: Based on Underground Cable Fault Locator documentation
*/

#include <LiquidCrystal.h>

// Initialize LCD with interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pin definitions
const int FORWARD_VOLTAGE_PIN = A0;
const int REFLECTED_VOLTAGE_PIN = A1;
const int TEST_BUTTON_PIN = 7;
const int BUZZER_PIN = 8;
const int LED_PIN = 13;

// System constants
const float VELOCITY_FACTOR = 0.66;  // Typical velocity factor for coaxial cable
const float CABLE_LENGTH = 1000.0;   // Maximum cable length in meters
const float REFERENCE_VOLTAGE = 5.0; // Arduino reference voltage
const int ADC_RESOLUTION = 1024;     // 10-bit ADC

// Measurement variables
float forwardVoltage = 0;
float reflectedVoltage = 0;
float vswr = 0;
float reflectionCoeff = 0;
float faultDistance = 0;
bool faultDetected = false;

// System state
bool testInProgress = false;
unsigned long testStartTime = 0;
const unsigned long TEST_DURATION = 2000; // Test duration in milliseconds

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("Cable Fault");
  lcd.setCursor(0, 1);
  lcd.print("Locator Ready");
  
  // Initialize pins
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Turn off buzzer and LED initially
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  delay(2000);
  displayMainMenu();
}

void loop() {
  // Check if test button is pressed
  if (digitalRead(TEST_BUTTON_PIN) == LOW && !testInProgress) {
    startTest();
  }
  
  // Handle test process
  if (testInProgress) {
    performMeasurement();
    
    // Check if test duration has elapsed
    if (millis() - testStartTime > TEST_DURATION) {
      completeTest();
    }
  }
  
  delay(100);
}

void displayMainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press TEST for");
  lcd.setCursor(0, 1);
  lcd.print("Cable Analysis");
}

void startTest() {
  testInProgress = true;
  testStartTime = millis();
  faultDetected = false;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing Cable...");
  lcd.setCursor(0, 1);
  lcd.print("Please Wait");
  
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("Starting cable fault test...");
}

void performMeasurement() {
  // Read analog values from voltage divider circuits
  int forwardReading = analogRead(FORWARD_VOLTAGE_PIN);
  int reflectedReading = analogRead(REFLECTED_VOLTAGE_PIN);
  
  // Convert ADC readings to voltages
  forwardVoltage = (forwardReading * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
  reflectedVoltage = (reflectedReading * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
  
  // Calculate reflection coefficient
  if (forwardVoltage > 0.1) { // Avoid division by very small numbers
    reflectionCoeff = reflectedVoltage / forwardVoltage;
  } else {
    reflectionCoeff = 0;
  }
  
  // Calculate VSWR
  if (reflectionCoeff < 1.0) {
    vswr = (1 + reflectionCoeff) / (1 - reflectionCoeff);
  } else {
    vswr = 99.9; // Set maximum VSWR for complete reflection
  }
  
  // Determine if fault exists (VSWR > 2.0 indicates significant mismatch)
  if (vswr > 2.0 && reflectedVoltage > 0.2) {
    faultDetected = true;
    
    // Calculate approximate fault distance
    // This is a simplified calculation - real implementation would need
    // time domain reflectometry or more sophisticated analysis
    float timeDelay = calculateTimeDelay();
    faultDistance = (timeDelay * 300000000 * VELOCITY_FACTOR) / 2; // Distance in meters
    
    // Limit distance to cable length
    if (faultDistance > CABLE_LENGTH) {
      faultDistance = CABLE_LENGTH;
    }
  }
  
  // Output debug information to serial
  Serial.print("Forward V: ");
  Serial.print(forwardVoltage, 3);
  Serial.print("V, Reflected V: ");
  Serial.print(reflectedVoltage, 3);
  Serial.print("V, VSWR: ");
  Serial.print(vswr, 2);
  Serial.print(", Distance: ");
  Serial.print(faultDistance, 1);
  Serial.println("m");
}

float calculateTimeDelay() {
  // Simplified time delay calculation based on reflection coefficient
  // In a real system, this would involve pulse transmission and timing
  float normalizedReflection = reflectionCoeff;
  if (normalizedReflection > 1.0) normalizedReflection = 1.0;
  
  // Approximate time delay based on reflection strength
  // Higher reflection typically indicates closer faults
  float timeDelay = (1.0 - normalizedReflection) * (CABLE_LENGTH / (300000000 * VELOCITY_FACTOR));
  return timeDelay;
}

void completeTest() {
  testInProgress = false;
  digitalWrite(LED_PIN, LOW);
  
  displayResults();
  
  if (faultDetected) {
    soundAlarm();
  }
  
  // Return to main menu after 5 seconds
  delay(5000);
  displayMainMenu();
}

void displayResults() {
  lcd.clear();
  
  if (faultDetected) {
    lcd.setCursor(0, 0);
    lcd.print("FAULT DETECTED!");
    lcd.setCursor(0, 1);
    
    // Display distance with appropriate units
    if (faultDistance < 1.0) {
      lcd.print((int)(faultDistance * 100));
      lcd.print("cm from start");
    } else if (faultDistance < 1000.0) {
      lcd.print((int)faultDistance);
      lcd.print("m from start");
    } else {
      lcd.print(">1km from start");
    }
    
    Serial.println("FAULT DETECTED!");
    Serial.print("Estimated distance: ");
    Serial.print(faultDistance, 1);
    Serial.println(" meters");
    Serial.print("VSWR: ");
    Serial.println(vswr, 2);
    
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Cable OK");
    lcd.setCursor(0, 1);
    lcd.print("VSWR: ");
    lcd.print(vswr, 1);
    
    Serial.println("Cable appears to be OK");
    Serial.print("VSWR: ");
    Serial.println(vswr, 2);
  }
}

void soundAlarm() {
  // Sound buzzer pattern for fault detection
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// Additional utility functions
void calibrateSystem() {
  // Function to calibrate the system with known good cable
  Serial.println("Calibration mode - connect known good cable");
  
  float calibrationReadings[10];
  for (int i = 0; i < 10; i++) {
    int reading = analogRead(FORWARD_VOLTAGE_PIN);
    calibrationReadings[i] = (reading * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
    delay(100);
  }
  
  // Calculate average calibration voltage
  float avgCalibration = 0;
  for (int i = 0; i < 10; i++) {
    avgCalibration += calibrationReadings[i];
  }
  avgCalibration /= 10;
  
  Serial.print("Calibration voltage: ");
  Serial.print(avgCalibration, 3);
  Serial.println("V");
}

void displayDetailedInfo() {
  // Function to display detailed measurement information
  Serial.println("=== Detailed Measurement Info ===");
  Serial.print("Forward Voltage: ");
  Serial.print(forwardVoltage, 4);
  Serial.println(" V");
  
  Serial.print("Reflected Voltage: ");
  Serial.print(reflectedVoltage, 4);
  Serial.println(" V");
  
  Serial.print("Reflection Coefficient: ");
  Serial.println(reflectionCoeff, 4);
  
  Serial.print("VSWR: ");
  Serial.println(vswr, 2);
  
  Serial.print("Return Loss: ");
  if (reflectionCoeff > 0) {
    float returnLoss = -20 * log10(reflectionCoeff);
    Serial.print(returnLoss, 1);
    Serial.println(" dB");
  } else {
    Serial.println("Infinite dB");
  }
  
  if (faultDetected) {
    Serial.print("Estimated Fault Distance: ");
    Serial.print(faultDistance, 1);
    Serial.println(" meters");
  }
  Serial.println("================================");
}