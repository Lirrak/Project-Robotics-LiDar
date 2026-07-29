/*
 * ESP32 Motor Controller & Odometry Firmware for Autonomous Differential Robot
 * Sub-controller for Raspberry Pi 3B+
 * 
 * Hardware Pinout:
 * - Left Motor Driver (TB6612FNG): PWM_L = GPIO 25, IN1_L = GPIO 26, IN2_L = GPIO 27
 * - Right Motor Driver (TB6612FNG): PWM_R = GPIO 14, IN1_R = GPIO 12, IN2_R = GPIO 13
 * - STBY Pin: GPIO 33
 * - Left Encoder: Phase A = GPIO 18, Phase B = GPIO 19
 * - Right Encoder: Phase A = GPIO 16, Phase B = GPIO 17
 * - MPU6050 IMU: SDA = GPIO 21, SCL = GPIO 22
 * - UART Serial to Raspberry Pi: TX = GPIO 1 (Default Serial) at 115200 Baud
 */

#include <Wire.h>

// --- Robot Kinematics Parameters ---
const float WHEEL_RADIUS = 0.0325; // 65mm diameter wheel -> R = 0.0325m
const float WHEEL_BASE   = 0.160;  // Distance between 2 wheels L = 160mm = 0.16m
const float COUNTS_PER_REV = 330.0; // Encoder PPR * Gear Ratio (JGA25 11 PPR * 30:1 = 330)

// --- Pin Definitions ---
#define PWM_L  25
#define IN1_L  26
#define IN2_L  27
#define PWM_R  14
#define IN1_R  12
#define IN2_R  13
#define STBY   33

#define ENC_L_A 18
#define ENC_L_B 19
#define ENC_R_A 16
#define ENC_R_B 17

// --- Global Variables ---
volatile long countLeft = 0;
volatile long countRight = 0;

float targetV = 0.0; // Linear velocity target (m/s)
float targetW = 0.0; // Angular velocity target (rad/s)

float currentVelL = 0.0, currentVelR = 0.0;
float targetVelL = 0.0, targetVelR = 0.0;

// PID Parameters
float Kp = 2.5, Ki = 0.5, Kd = 0.05;
float errSumL = 0, lastErrL = 0;
float errSumR = 0, lastErrR = 0;

unsigned long lastTime = 0;
unsigned long lastSerialTime = 0;

// Encoder Interrupt Service Routines
void IRAM_ATTR isrEncoderLeft() {
  if (digitalRead(ENC_L_B) == HIGH) {
    countLeft++;
  } else {
    countLeft--;
  }
}

void IRAM_ATTR isrEncoderRight() {
  if (digitalRead(ENC_R_B) == HIGH) {
    countRight++;
  } else {
    countRight--;
  }
}

void setup() {
  Serial.begin(115200);

  // Motor Driver Pins
  pinMode(PWM_L, OUTPUT); pinMode(IN1_L, OUTPUT); pinMode(IN2_L, OUTPUT);
  pinMode(PWM_R, OUTPUT); pinMode(IN1_R, OUTPUT); pinMode(IN2_R, OUTPUT);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH); // Enable motor driver

  // Encoder Pins
  pinMode(ENC_L_A, INPUT_PULLUP);
  pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP);
  pinMode(ENC_R_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_L_A), isrEncoderLeft, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), isrEncoderRight, RISING);

  lastTime = millis();
}

void setMotorLeft(float speed) { // speed from -255 to 255
  int pwmVal = constrain(abs(speed), 0, 255);
  if (speed > 0) {
    digitalWrite(IN1_L, HIGH);
    digitalWrite(IN2_L, LOW);
  } else if (speed < 0) {
    digitalWrite(IN1_L, LOW);
    digitalWrite(IN2_L, HIGH);
  } else {
    digitalWrite(IN1_L, LOW);
    digitalWrite(IN2_L, LOW);
  }
  analogWrite(PWM_L, pwmVal);
}

void setMotorRight(float speed) { // speed from -255 to 255
  int pwmVal = constrain(abs(speed), 0, 255);
  if (speed > 0) {
    digitalWrite(IN1_R, HIGH);
    digitalWrite(IN2_R, LOW);
  } else if (speed < 0) {
    digitalWrite(IN1_R, LOW);
    digitalWrite(IN2_R, HIGH);
  } else {
    digitalWrite(IN1_R, LOW);
    digitalWrite(IN2_R, LOW);
  }
  analogWrite(PWM_R, pwmVal);
}

void processIncomingSerial() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("CMD,")) {
      // Parse format: "CMD,v,w"
      int idx1 = line.indexOf(',');
      int idx2 = line.indexOf(',', idx1 + 1);
      if (idx1 > 0 && idx2 > 0) {
        targetV = line.substring(idx1 + 1, idx2).toFloat();
        targetW = line.substring(idx2 + 1).toFloat();

        // Convert (v, w) to Left & Right Wheel target velocities
        targetVelL = targetV - (targetW * WHEEL_BASE / 2.0);
        targetVelR = targetV + (targetW * WHEEL_BASE / 2.0);
      }
    }
  }
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;

  processIncomingSerial();

  if (dt >= 0.05) { // 20Hz PID Control Loop
    lastTime = now;

    // Read current encoder tick counts
    noInterrupts();
    long dTicksL = countLeft;
    long dTicksR = countRight;
    countLeft = 0;
    countRight = 0;
    interrupts();

    // Calculate current velocity in m/s
    currentVelL = (dTicksL / COUNTS_PER_REV) * (2.0 * M_PI * WHEEL_RADIUS) / dt;
    currentVelR = (dTicksR / COUNTS_PER_REV) * (2.0 * M_PI * WHEEL_RADIUS) / dt;

    // PID Left Wheel
    float errL = targetVelL - currentVelL;
    errSumL += errL * dt;
    float dErrL = (errL - lastErrL) / dt;
    float pwmOutL = (Kp * errL) + (Ki * errSumL) + (Kd * dErrL);
    lastErrL = errL;

    // PID Right Wheel
    float errR = targetVelR - currentVelR;
    errSumR += errR * dt;
    float dErrR = (errR - lastErrR) / dt;
    float pwmOutR = (Kp * errR) + (Ki * errSumR) + (Kd * dErrR);
    lastErrR = errR;

    // Drive Motors
    setMotorLeft(pwmOutL * 255.0);
    setMotorRight(pwmOutR * 255.0);

    // Send Telemetry back to Raspberry Pi (at 20Hz)
    // Format: "TELE,velL,velR,ticksL,ticksR"
    Serial.print("TELE,");
    Serial.print(currentVelL, 4); Serial.print(",");
    Serial.print(currentVelR, 4); Serial.print(",");
    Serial.print(dTicksL); Serial.print(",");
    Serial.println(dTicksR);
  }
}
