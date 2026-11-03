#include <Servo.h>

// ==========================================
// PIN CONFIGURATION
// ==========================================
// L298N Motor Driver Pins
const int LEFT_FWD  = 2;
const int LEFT_REV  = 3;
const int RIGHT_FWD = 4;
const int RIGHT_REV = 5;

// HC-SR04 Ultrasonic Sensor Pins
const int TRIG_PIN  = 6;
const int ECHO_PIN  = 7;

// Servo Pins
const int SERVO_LEFT_PIN   = 8;
const int SERVO_RIGHT_PIN  = 9;
const int SERVO_MIDDLE_PIN = 10;

// ==========================================
// CONFIGURATION CONSTANTS
// ==========================================
const int DETECTION_DISTANCE_CM = 15;  // Trigger grab when object <= 15cm
const int SERVO_STEP_DELAY_MS   = 12;  // Speed of arm movement (smooth sweep)

// Servo Angles
const int ARM_CLOSED_ANGLE = 0;
const int ARM_OPEN_ANGLE   = 90;
const int LIFT_UP_ANGLE    = 0;
const int LIFT_DOWN_ANGLE  = 100;

// Servo Instances
Servo leftArm;
Servo rightArm;
Servo middleArm;

// ==========================================
// MOTOR CONTROL FUNCTIONS
// ==========================================
void moveForward() {
  digitalWrite(LEFT_FWD, HIGH);
  digitalWrite(RIGHT_FWD, HIGH);
  digitalWrite(LEFT_REV, LOW);
  digitalWrite(RIGHT_REV, LOW);
}

void stopMotors() {
  digitalWrite(LEFT_FWD, LOW);
  digitalWrite(RIGHT_FWD, LOW);
  digitalWrite(LEFT_REV, LOW);
  digitalWrite(RIGHT_REV, LOW);
}

// ==========================================
// SENSOR FUNCTION (WITH TIMEOUT & FILTERING)
// ==========================================
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 30000us timeout (~5m max range) prevents hangup if beam is lost
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999; // Return out-of-range if no echo

  return duration * 0.034 / 2;
}

long getFilteredDistance() {
  long d1 = readDistanceCM();
  delay(10);
  long d2 = readDistanceCM();
  delay(10);
  long d3 = readDistanceCM();

  return (d1 + d2 + d3) / 3;
}

// ==========================================
// SERVO SMOOTH MOTION HELPERS
// ==========================================
// Smoothly moves gripper arms together
void moveGrippers(int startAngle, int targetAngle) {
  if (startAngle < targetAngle) {
    for (int pos = startAngle; pos <= targetAngle; pos++) {
      leftArm.write(pos);
      rightArm.write(pos);
      delay(SERVO_STEP_DELAY_MS);
    }
  } else {
    for (int pos = startAngle; pos >= targetAngle; pos--) {
      leftArm.write(pos);
      rightArm.write(pos);
      delay(SERVO_STEP_DELAY_MS);
    }
  }
}

// Smoothly moves the middle lifting arm
void moveLifter(int startAngle, int targetAngle) {
  if (startAngle < targetAngle) {
    for (int pos = startAngle; pos <= targetAngle; pos++) {
      middleArm.write(pos);
      delay(SERVO_STEP_DELAY_MS);
    }
  } else {
    for (int pos = startAngle; pos >= targetAngle; pos--) {
      middleArm.write(pos);
      delay(SERVO_STEP_DELAY_MS);
    }
  }
}

// ==========================================
// WASTE COLLECTION SEQUENCE
// ==========================================
void executeCollectionRoutine() {
  stopMotors();
  delay(500);

  // 1. Open gripper arms
  moveGrippers(ARM_CLOSED_ANGLE, ARM_OPEN_ANGLE);
  delay(300);

  // 2. Lower the main lift arm
  moveLifter(LIFT_UP_ANGLE, LIFT_DOWN_ANGLE);
  delay(400);

  // 3. Close gripper arms to grab garbage
  moveGrippers(ARM_OPEN_ANGLE, ARM_CLOSED_ANGLE);
  delay(400);

  // 4. Raise the main lift arm toward bin
  moveLifter(LIFT_DOWN_ANGLE, LIFT_UP_ANGLE);
  delay(500);

  // 5. Release waste into the collection bin
  moveGrippers(ARM_CLOSED_ANGLE, ARM_OPEN_ANGLE);
  delay(600);

  // 6. Reset gripper back to closed home position
  moveGrippers(ARM_OPEN_ANGLE, ARM_CLOSED_ANGLE);
  delay(300);
}

// ==========================================
// SETUP & MAIN LOOP
// ==========================================
void setup() {
  pinMode(LEFT_FWD, OUTPUT);
  pinMode(LEFT_REV, OUTPUT);
  pinMode(RIGHT_FWD, OUTPUT);
  pinMode(RIGHT_REV, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  leftArm.attach(SERVO_LEFT_PIN);
  rightArm.attach(SERVO_RIGHT_PIN);
  middleArm.attach(SERVO_MIDDLE_PIN);

  // Initialize home positions
  leftArm.write(ARM_CLOSED_ANGLE);
  rightArm.write(ARM_CLOSED_ANGLE);
  middleArm.write(LIFT_UP_ANGLE);

  Serial.begin(9600);
  stopMotors();
  delay(1000);
}

void loop() {
  long distance = getFilteredDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= DETECTION_DISTANCE_CM) {
    executeCollectionRoutine();
  } else {
    moveForward();
  }

  delay(60);
}
