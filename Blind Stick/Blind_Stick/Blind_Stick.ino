/*
 * Ultrasonic Blind Stick with Audio Feedback
 * Uses HC-SR04 ultrasonic sensor and DFPlayer Mini for audio playback
 * Provides distance-based audio alerts through earphones
 */

#include <SoftwareSerial.h>

// Pin definitions
#define TRIG_PIN 9
#define ECHO_PIN 10
#define DFPLAYER_RX 2
#define DFPLAYER_TX 3
#define BUSY_PIN 4

// Create software serial for DFPlayer Mini
SoftwareSerial dfSerial(DFPLAYER_RX, DFPLAYER_TX);

// Distance thresholds (in cm)
#define VERY_CLOSE 10
#define CLOSE 20
#define MEDIUM 30
#define FAR 50

// Audio file numbers (stored on SD card)
#define AUDIO_VERY_CLOSE 1  // "Very close obstacle"
#define AUDIO_CLOSE 2       // "Close obstacle"
#define AUDIO_MEDIUM 3      // "Medium distance"
#define AUDIO_FAR 4         // "Far obstacle"
#define AUDIO_CLEAR 5       // "Path clear"

// Timing variables
unsigned long lastMeasurement = 0;
unsigned long lastAudioPlay = 0;
const unsigned long MEASUREMENT_INTERVAL = 200; // 200ms between measurements
const unsigned long AUDIO_INTERVAL = 1000;     // 1 second between audio alerts

int lastDistance = -1;
int currentAudioFile = -1;

void setup() {
  Serial.begin(9600);
  dfSerial.begin(9600);
  
  // Initialize pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUSY_PIN, INPUT);
  
  // Initialize ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  
  Serial.println("Ultrasonic Blind Stick Initializing...");
  delay(1000);
  
  // Initialize DFPlayer Mini
  initializeDFPlayer();
  
  Serial.println("System Ready!");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Measure distance at regular intervals
  if (currentTime - lastMeasurement >= MEASUREMENT_INTERVAL) {
    int distance = measureDistance();
    
    if (distance > 0 && distance <= 200) { // Valid reading range
      processDistance(distance, currentTime);
    }
    
    lastMeasurement = currentTime;
  }
}

int measureDistance() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10 microsecond pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  if (duration == 0) {
    return -1; // No echo received
  }
  
  // Calculate distance in cm
  int distance = duration * 0.034 / 2;
  
  return distance;
}

void processDistance(int distance, unsigned long currentTime) {
  int audioFile = getAudioFile(distance);
  
  // Only play audio if:
  // 1. Enough time has passed since last audio
  // 2. Distance category has changed
  // 3. DFPlayer is not busy playing
  if ((currentTime - lastAudioPlay >= AUDIO_INTERVAL) && 
      (audioFile != currentAudioFile) && 
      !isDFPlayerBusy()) {
    
    playAudioFile(audioFile);
    currentAudioFile = audioFile;
    lastAudioPlay = currentTime;
    
    // Debug output
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm - Playing audio file: ");
    Serial.println(audioFile);
  }
  
  lastDistance = distance;
}

int getAudioFile(int distance) {
  if (distance <= VERY_CLOSE) {
    return AUDIO_VERY_CLOSE;
  } else if (distance <= CLOSE) {
    return AUDIO_CLOSE;
  } else if (distance <= MEDIUM) {
    return AUDIO_MEDIUM;
  } else if (distance <= FAR) {
    return AUDIO_FAR;
  } else {
    return AUDIO_CLEAR;
  }
}

void initializeDFPlayer() {
  Serial.println("Initializing DFPlayer Mini...");
  
  // Send initialization commands
  sendDFCommand(0x3F, 0x00, 0x00); // Initialize
  delay(500);
  
  sendDFCommand(0x06, 0x00, 0x1E); // Set volume to 30 (max is 30)
  delay(100);
  
  sendDFCommand(0x09, 0x00, 0x01); // Select SD card as source
  delay(100);
  
  Serial.println("DFPlayer Mini initialized");
}

void playAudioFile(int fileNumber) {
  sendDFCommand(0x03, 0x00, fileNumber);
}

bool isDFPlayerBusy() {
  return digitalRead(BUSY_PIN) == LOW;
}

void sendDFCommand(byte command, byte param1, byte param2) {
  byte checksum = 0xFF - (0xFF + 0x06 + command + 0x00 + param1 + param2) + 0x01;
  
  byte cmdArray[10] = {
    0x7E,    // Start byte
    0xFF,    // Version
    0x06,    // Length
    command, // Command
    0x00,    // Feedback (no feedback)
    param1,  // Parameter 1
    param2,  // Parameter 2
    checksum >> 8,   // Checksum high
    checksum & 0xFF, // Checksum low
    0xEF     // End byte
  };
  
  for (int i = 0; i < 10; i++) {
    dfSerial.write(cmdArray[i]);
  }
  
  delay(50); // Small delay after sending command
}

/*
 * HARDWARE CONNECTIONS:
 * 
 * Arduino Uno/Nano:
 * - TRIG_PIN (Pin 9) -> HC-SR04 Trig
 * - ECHO_PIN (Pin 10) -> HC-SR04 Echo
 * - Pin 2 -> DFPlayer Mini RX
 * - Pin 3 -> DFPlayer Mini TX
 * - Pin 4 -> DFPlayer Mini BUSY
 * - 5V -> HC-SR04 VCC, DFPlayer Mini VCC
 * - GND -> HC-SR04 GND, DFPlayer Mini GND
 * - DFPlayer Mini SPK+ and SPK- -> Earphones (or use headphone jack)
 * 
 * SD CARD AUDIO FILES (name them as numbers):
 * 001.mp3 -> "Very close obstacle detected" (≤10cm)
 * 002.mp3 -> "Close obstacle detected" (≤20cm)
 * 003.mp3 -> "Medium distance obstacle" (≤30cm)
 * 004.mp3 -> "Far obstacle detected" (≤50cm)
 * 005.mp3 -> "Path is clear" (>50cm)
 * 
 * FEATURES:
 * - Continuous distance monitoring every 200ms
 * - Audio feedback based on distance ranges
 * - Prevents audio overlap with busy pin detection
 * - Adjustable volume and timing intervals
 * - Serial monitor for debugging
 */