// Last Update: 10/23/2026
// Based on a friend's SCRAP v2 sumobot code, rewritten for different hardware.

/* SUMOBOT PIN CONFIGURATION

HARDWARE
- Arduino Uno R3 (+ sensor/IO shield)
- 2x BTS7960 (IBT-2) motor driver, one per motor
- 2x 12V 280RPM DC motor
- 3x SHARP IR distance sensor 10-80cm (GP2Y0A21, analog)
- 2x single line sensor (analog output)
- 1x start switch (5 second delay before fighting)
- 3S 11.1V LiPo

Same wiring as sumobot_tester.ino.
"Left" and "right" = as seen from behind the bot, looking forward.

BTS7960 LEFT MOTOR
RPWM      - D5
LPWM      - D6
R_EN, L_EN, VCC - 5V (each with its own wire)
GND       - GND
B+ / B-   - Battery + / -
M+ / M-   - Left motor

BTS7960 RIGHT MOTOR
RPWM      - D9
LPWM      - D10
R_EN, L_EN, VCC - 5V (each with its own wire)
GND       - GND
B+ / B-   - Battery + / -
M+ / M-   - Right motor

SHARP IR (Red = 5V, Black = GND, Yellow = signal)
RIGHT - A0
FRONT - A1
LEFT  - A2

LINE SENSORS (analog)
LEFT  - A4
RIGHT - A5
Calibrated on BLACK during the last second of the countdown,
so the bot must sit still inside the ring before it starts.

START SWITCH (optional, see USE_START_SWITCH)
One leg - D4
Other leg - GND
No switch = the 5 second countdown starts as soon as the bot is powered on.

STATUS LED - D13 (built-in LED)
  OFF        = waiting (stopped)
  BLINKING   = 5 second countdown
  ON         = fighting

POWER
Battery + -> both BTS7960 B+ and Arduino VIN (or DC jack)
Battery - -> both BTS7960 B- and Arduino GND
ALL GROUNDS MUST BE CONNECTED TOGETHER.

FEATURES:
- 5 second delayed start (at power on, or with a switch)
- Switch again (or turn toggle OFF) = emergency stop
- Serial Monitor: x = stop, g = start countdown
- Line escape (back up + turn away from the edge)
- Enemy tracking with 3 SHARP IR sensors
- Searches toward the side the enemy was last seen
- Serial Monitor (9600) sensor test + motor test while waiting

*/


// ================= PIN CONFIGURATION =================

// Left motor driver (BTS7960)
#define LEFT_RPWM 5
#define LEFT_LPWM 6

// Right motor driver (BTS7960)
#define RIGHT_RPWM 9
#define RIGHT_LPWM 10

// SHARP IR sensors
#define SHARP_RIGHT A0
#define SHARP_FRONT A1
#define SHARP_LEFT A2

// Line sensors
#define LINE_LEFT A4
#define LINE_RIGHT A5

// Start switch
#define START_SWITCH 4

// Status LED
#define STATUS_LED 13


// ================= SWITCH CONFIGURATION =================

// 0 = NO SWITCH : countdown starts right after power on / reset
// 1 = SWITCH on D4 (type set below)
#define USE_START_SWITCH 0

// 0 = PUSH BUTTON : press = start countdown, press again = stop
// 1 = TOGGLE      : ON = start countdown, OFF = stop
#define START_SWITCH_IS_TOGGLE 0

// Delayed start
#define START_DELAY 5000


// ================= LINE CONFIGURATION =================

// How much a line sensor reading must change from the black
// baseline to count as the white line (same as the tester).
#define EDGE_MARGIN 150

// Black baseline, measured during the countdown
int lineBaseLeft = 0;
int lineBaseRight = 0;
bool lineCalibrated = false;

long lineSumLeft = 0;
long lineSumRight = 0;
long lineSamples = 0;


// ================= MOTOR DIRECTION =================

// If a motor spins backward when it should go forward,
// change its value to 1 (instead of swapping wires).
#define LEFT_MOTOR_REVERSED 0
#define RIGHT_MOTOR_REVERSED 0


// ===== ATTACK DISTANCE CONFIGURATION ======

// SHARP 10-80cm sensors. Distances are in cm.
#define FRONT_ATTACK_DISTANCE 60
#define SIDE_ATTACK_DISTANCE 40

// Keep pushing this long after the front sensor loses the enemy.
// SHARP sensors give wrong readings below 10cm (when touching),
// so this keeps the robot pushing during contact.
#define FRONT_LOCK_TIME 300


// ================= SPEED SETTINGS =================

const int fullSpeed = 255;  // FULL SPEED CONSTANT

int attackSpeed = 255;      // Front attack (push)
int turnAttackSpeed = 200;  // Turning toward enemy seen on the side
int searchSpeed = 120;      // Spinning while searching
int escapeSpeed = 255;      // Backing away from the line
int testSpeed = 150;        // Serial motor test


// ================= LINE ESCAPE TIMING (ms) =================

#define ESCAPE_BACK_TIME 250
#define ESCAPE_TURN_TIME 250
#define ESCAPE_TURN_TIME_BOTH 450  // both sensors on the line = turn more


// ================= SENSOR VALUES =================

int frontDistance;
int leftDistance;
int rightDistance;

bool lineLeft;
bool lineRight;


// ================= ROBOT STATE =================

#define STATE_IDLE 0
#define STATE_COUNTDOWN 1
#define STATE_FIGHT 2

byte robotState = STATE_IDLE;

unsigned long countdownStart = 0;
int lastCountdownSecond = -1;


// ================= LINE ESCAPE STATE =================

#define ESCAPE_NONE 0
#define ESCAPE_BACK 1
#define ESCAPE_TURN 2

byte escapePhase = ESCAPE_NONE;
unsigned long escapePhaseEnd = 0;
int escapeTurnTime = ESCAPE_TURN_TIME;
int escapeTurnDir = 1;  // -1 = LEFT, 1 = RIGHT


// ================= ENEMY MEMORY =================

int lastSeenDir = 1;  // -1 = LEFT, 1 = RIGHT (which way to search)
unsigned long lastFrontSeen = 0;


// ================= SWITCH DEBOUNCE =================

#define DEBOUNCE_TIME 30

bool switchOn = false;
bool switchLastRaw = false;
unsigned long switchChangedAt = 0;


// ================= SERIAL TEST =================

unsigned long lastSensorPrint = 0;


// ================= SETUP =================

void setup() {

  Serial.begin(9600);


  // ================= MOTOR PINS =================

  pinMode(LEFT_RPWM, OUTPUT);
  pinMode(LEFT_LPWM, OUTPUT);

  pinMode(RIGHT_RPWM, OUTPUT);
  pinMode(RIGHT_LPWM, OUTPUT);

  stopMove();


  // ================= SWITCH / LED =================

  pinMode(START_SWITCH, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  // Read the switch once so a toggle that is already ON at power up
  // does NOT start the robot. It must be turned OFF then ON again.
  switchLastRaw = (digitalRead(START_SWITCH) == LOW);
  switchOn = switchLastRaw;


  Serial.println(F("SUMOBOT READY"));
  Serial.println(F("x = stop, g = start countdown"));
  Serial.println(F("Motor test (when stopped): f = forward, b = backward, l = left, r = right"));

#if !USE_START_SWITCH
  // No switch: start the 5 second countdown right away
  startCountdown();
#endif
}


// ================= MAIN LOOP =================

void loop() {

#if USE_START_SWITCH
  handleSwitch();
#endif


  // ================= SERIAL STOP =================

  if (robotState != STATE_IDLE && Serial.available()) {

    char command = Serial.read();

    if (command == 'x' || command == 'X') {
      stopRobot();
    }
  }


  // ================= WAITING =================

  if (robotState == STATE_IDLE) {

    idleMode();

    return;
  }


  // ================= COUNTDOWN =================

  if (robotState == STATE_COUNTDOWN) {

    countdownMode();

    return;
  }


  // ================= FIGHT =================

  autonomousMode();
}


// =================================================
// ================= START SWITCH ==================
// =================================================

// Returns 1 when the switch turns ON, -1 when it turns OFF, 0 otherwise
int readSwitchEdge() {

  bool raw = (digitalRead(START_SWITCH) == LOW);

  if (raw != switchLastRaw) {

    switchLastRaw = raw;
    switchChangedAt = millis();
  }

  if (millis() - switchChangedAt > DEBOUNCE_TIME && raw != switchOn) {

    switchOn = raw;

    return switchOn ? 1 : -1;
  }

  return 0;
}


void handleSwitch() {

  int edge = readSwitchEdge();

#if START_SWITCH_IS_TOGGLE

  // TOGGLE: ON = start, OFF = stop
  if (edge == 1 && robotState == STATE_IDLE) {
    startCountdown();
  }

  else if (edge == -1 && robotState != STATE_IDLE) {
    stopRobot();
  }

#else

  // PUSH BUTTON: every press switches between start and stop
  if (edge == 1) {

    if (robotState == STATE_IDLE) {
      startCountdown();
    }

    else {
      stopRobot();
    }
  }

#endif
}


void startCountdown() {

  stopMove();

  robotState = STATE_COUNTDOWN;
  countdownStart = millis();
  lastCountdownSecond = -1;

  lineSumLeft = 0;
  lineSumRight = 0;
  lineSamples = 0;

  Serial.println(F("DELAY MODE: COUNTDOWN"));
}


void stopRobot() {

  stopMove();

  robotState = STATE_IDLE;
  digitalWrite(STATUS_LED, LOW);

  Serial.println(F("SYSTEM STOPPED"));
}


// =================================================
// ================= COUNTDOWN =====================
// =================================================

void countdownMode() {

  // Motors must remain stopped
  stopMove();

  unsigned long elapsed = millis() - countdownStart;


  // ================= LINE CALIBRATION =================

  // Last second of the countdown: the bot sits still on BLACK
  if (elapsed >= START_DELAY - 1000 && elapsed < START_DELAY) {

    lineSumLeft += analogRead(LINE_LEFT);
    lineSumRight += analogRead(LINE_RIGHT);
    lineSamples++;
  }


  // ================= START FIGHT =================

  if (elapsed >= START_DELAY) {

    if (lineSamples > 0) {

      lineBaseLeft = lineSumLeft / lineSamples;
      lineBaseRight = lineSumRight / lineSamples;
      lineCalibrated = true;
    }

    Serial.print(F("Black baseline  L: "));
    Serial.print(lineBaseLeft);
    Serial.print(F("  R: "));
    Serial.println(lineBaseRight);

    robotState = STATE_FIGHT;

    escapePhase = ESCAPE_NONE;
    lastSeenDir = 1;
    lastFrontSeen = 0;

    digitalWrite(STATUS_LED, HIGH);

    Serial.println(F("FIGHT!"));

    return;
  }


  // ================= 5, 4, 3, 2, 1 =================

  int secondsLeft = (START_DELAY - elapsed + 999) / 1000;

  if (secondsLeft != lastCountdownSecond) {

    lastCountdownSecond = secondsLeft;
    Serial.println(secondsLeft);
  }

  // Blink LED: ON for the first half of every second
  digitalWrite(STATUS_LED, (elapsed % 1000) < 500 ? HIGH : LOW);
}


// =================================================
// ================= AUTONOMOUS MODE ===============
// =================================================

void autonomousMode() {

  unsigned long now = millis();


  // ================= LINE DETECTION =================

  readLineSensors();

  // Start a new escape unless we are already backing up
  if ((lineLeft || lineRight) && escapePhase != ESCAPE_BACK) {

    startEscape();
  }


  // ================= LINE ESCAPE: BACK UP =================

  if (escapePhase == ESCAPE_BACK) {

    if (now < escapePhaseEnd) {

      moveBackward(escapeSpeed);

      return;
    }

    escapePhase = ESCAPE_TURN;
    escapePhaseEnd = now + escapeTurnTime;
  }


  // ================= LINE ESCAPE: TURN AWAY =================

  if (escapePhase == ESCAPE_TURN) {

    if (now < escapePhaseEnd) {

      if (escapeTurnDir < 0) {
        turnLeft(escapeSpeed);
      } else {
        turnRight(escapeSpeed);
      }

      return;
    }

    escapePhase = ESCAPE_NONE;
  }


  // ================= SHARP IR =================

  readDistanceSensors();


  // ================= FRONT =================

  if (frontDistance > 0 && frontDistance <= FRONT_ATTACK_DISTANCE) {

    lastFrontSeen = now;

    pushForward(attackSpeed);  // FULL SPEED

    return;
  }

  // Lost the enemy in front only a moment ago (probably touching it)
  if (lastFrontSeen != 0 && now - lastFrontSeen < FRONT_LOCK_TIME) {

    pushForward(attackSpeed);

    return;
  }


  // ================= LEFT =================

  if (leftDistance > 0 && leftDistance <= SIDE_ATTACK_DISTANCE) {

    lastSeenDir = -1;

    turnLeft(turnAttackSpeed);

    return;
  }


  // ================= RIGHT =================

  if (rightDistance > 0 && rightDistance <= SIDE_ATTACK_DISTANCE) {

    lastSeenDir = 1;

    turnRight(turnAttackSpeed);

    return;
  }


  // ================= SEARCH =================

  search();
}


// =================================================
// ================= LINE ESCAPE ===================
// =================================================

void startEscape() {

  // LEFT sensor on line -> turn RIGHT (away from the edge)
  if (lineLeft && !lineRight) {

    escapeTurnDir = 1;
    escapeTurnTime = ESCAPE_TURN_TIME;
  }

  // RIGHT sensor on line -> turn LEFT
  else if (lineRight && !lineLeft) {

    escapeTurnDir = -1;
    escapeTurnTime = ESCAPE_TURN_TIME;
  }

  // BOTH -> turn around, toward where the enemy was last seen
  else {

    escapeTurnDir = lastSeenDir;
    escapeTurnTime = ESCAPE_TURN_TIME_BOTH;
  }

  escapePhase = ESCAPE_BACK;
  escapePhaseEnd = millis() + ESCAPE_BACK_TIME;
}


// =================================================
// ================= SEARCH =========================
// =================================================

void search() {

  // Spin toward the side the enemy was last seen
  if (lastSeenDir < 0) {
    turnLeft(searchSpeed);
  } else {
    turnRight(searchSpeed);
  }
}


// =================================================
// ================= SENSORS =======================
// =================================================

void readLineSensors() {

  // No black baseline yet = can't tell where the line is
  if (!lineCalibrated) {

    lineLeft = false;
    lineRight = false;

    return;
  }

  lineLeft = abs(analogRead(LINE_LEFT) - lineBaseLeft) > EDGE_MARGIN;
  lineRight = abs(analogRead(LINE_RIGHT) - lineBaseRight) > EDGE_MARGIN;
}


void readDistanceSensors() {

  frontDistance = readSharp(SHARP_FRONT);
  leftDistance = readSharp(SHARP_LEFT);
  rightDistance = readSharp(SHARP_RIGHT);
}


// Returns distance in cm (10-80), or 0 if nothing is in range
int readSharp(byte pin) {

  // Average a few readings, motors make the signal noisy
  int raw = 0;

  for (byte i = 0; i < 4; i++) {
    raw += analogRead(pin);
  }

  raw /= 4;


  float volts = raw * (5.0 / 1023.0);

  // Below 0.4V = farther than 80cm = nothing there
  if (volts < 0.4) {
    return 0;
  }


  // GP2Y0A21 voltage -> distance curve (same as the tester)
  int cm = 27.86 * pow(volts, -1.15);

  if (cm > 80) {
    return 0;
  }

  if (cm < 10) {
    cm = 10;
  }

  return cm;
}


// =================================================
// ================= IDLE / SERIAL TEST ============
// =================================================

void idleMode() {

  stopMove();


  // ================= MOTOR TEST =================

  if (Serial.available()) {

    char command = Serial.read();

    switch (command) {

      case 'g':
      case 'G':
        startCountdown();
        return;

      case 'f':
      case 'F':
        Serial.println(F("TEST: FORWARD"));
        pushForward(testSpeed);
        delay(500);
        break;

      case 'b':
      case 'B':
        Serial.println(F("TEST: BACKWARD"));
        moveBackward(testSpeed);
        delay(500);
        break;

      case 'l':
      case 'L':
        Serial.println(F("TEST: LEFT"));
        turnLeft(testSpeed);
        delay(500);
        break;

      case 'r':
      case 'R':
        Serial.println(F("TEST: RIGHT"));
        turnRight(testSpeed);
        delay(500);
        break;
    }

    stopMove();
  }


  // ================= SENSOR TEST =================

  if (millis() - lastSensorPrint < 250) {
    return;
  }

  lastSensorPrint = millis();

  readDistanceSensors();
  readLineSensors();

  Serial.print(F("L: "));
  printDistance(leftDistance);

  Serial.print(F("  F: "));
  printDistance(frontDistance);

  Serial.print(F("  R: "));
  printDistance(rightDistance);

  // Raw values; WHITE/black only shows after the first countdown
  Serial.print(F("  | LINE L: "));
  Serial.print(analogRead(LINE_LEFT));
  if (lineCalibrated) {
    Serial.print(lineLeft ? F(" WHITE") : F(" black"));
  }

  Serial.print(F("  LINE R: "));
  Serial.print(analogRead(LINE_RIGHT));
  if (lineCalibrated) {
    Serial.print(lineRight ? F(" WHITE") : F(" black"));
  }

  Serial.print(F("  | SWITCH: "));
  Serial.println(switchOn ? F("ON") : F("OFF"));
}


void printDistance(int cm) {

  if (cm == 0) {
    Serial.print(F("---"));
  }

  else {
    Serial.print(cm);
    Serial.print(F("cm"));
  }
}


// =================================================
// ================= MOTOR FUNCTIONS ===============
// =================================================

// speed: -255 (full backward) to 255 (full forward)
void driveMotor(byte rpwmPin, byte lpwmPin, int speed) {

  speed = constrain(speed, -255, 255);

  // Never drive RPWM and LPWM at the same time
  if (speed >= 0) {

    analogWrite(lpwmPin, 0);
    analogWrite(rpwmPin, speed);
  }

  else {

    analogWrite(rpwmPin, 0);
    analogWrite(lpwmPin, -speed);
  }
}


void setMotors(int leftSpeed, int rightSpeed) {

  if (LEFT_MOTOR_REVERSED) {
    leftSpeed = -leftSpeed;
  }

  if (RIGHT_MOTOR_REVERSED) {
    rightSpeed = -rightSpeed;
  }

  driveMotor(LEFT_RPWM, LEFT_LPWM, leftSpeed);
  driveMotor(RIGHT_RPWM, RIGHT_LPWM, rightSpeed);
}


// STOP
void stopMove() {

  setMotors(0, 0);
}


// FORWARD
void pushForward(int speed) {

  setMotors(speed, speed);
}


// BACKWARD
void moveBackward(int speed) {

  setMotors(-speed, -speed);
}


// LEFT (spin in place)
void turnLeft(int speed) {

  setMotors(-speed, speed);
}


// RIGHT (spin in place)
void turnRight(int speed) {

  setMotors(speed, -speed);
}
