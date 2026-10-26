// SUMO BOT TESTER (testing only)
// Open Serial Monitor at 9600 baud, then type a number and press Enter:
//   1 = Sharp IR sensors (distance in cm, updates every 1 second)
//   2 = Line sensors     (calibrates on BLACK, then shows black/WHITE)
//   3 = Motor test       (WHEELS OFF THE GROUND! runs each motor, then both)
//   0 = Stop everything and show the menu
//
// ================= PIN LIST =================
// SHARP IR (analog, 10-80cm)
//   Right sensor  - A0
//   Front sensor  - A1
//   Left sensor   - A2
// LINE SENSORS (analog)
//   Left line     - A4
//   Right line    - A5
// MOTORS (BTS7960 drivers on the sensor shield)
//   Left  motor board: RPWM - D5,  LPWM - D6
//   Right motor board: RPWM - D9,  LPWM - D10
//   R_EN, L_EN, VCC of both boards -> any V pin (5V), each with its own wire
//   GND of both boards -> any G pin
// "Left" and "right" = as seen from behind the bot, looking forward.
// ============================================

// ---- Pins ----
const int SH_R = A0;
const int SH_C = A1;
const int SH_L = A2;

const int LINE_L = A4;
const int LINE_R = A5;

const int M1_R = 5, M1_L = 6;     // left motor
const int M2_R = 9, M2_L = 10;    // right motor

// ---- Settings ----
const int EDGE_MARGIN = 150;      // line sensor: change from black that counts as white
const int TEST_SPEED  = 120;      // motor test speed (0-255)

// ---- State ----
char mode = '0';
unsigned long lastPrint = 0;
int baseL = 0, baseR = 0;

int motorStep = 0;
unsigned long motorStepStart = 0;
bool motorCountdown = false;

// ================= MOTOR HELPERS =================

void drive(int rPin, int lPin, int s) {
  if (s >= 0) { analogWrite(rPin, s); analogWrite(lPin, 0); }
  else        { analogWrite(rPin, 0); analogWrite(lPin, -s); }
}
void move(int left, int right) {
  drive(M1_R, M1_L, left);
  drive(M2_R, M2_L, right);
}
void stopMove() { move(0, 0); }

// ================= SENSOR HELPERS =================

// Distance in cm for a GP2Y0A21 style sensor. Returns -1 if too far.
float readCm(int pin) {
  long sum = 0;
  for (int i = 0; i < 5; i++) sum += analogRead(pin);
  float volts = (sum / 5.0) * 5.0 / 1023.0;
  if (volts < 0.4) return -1;
  return 27.86 * pow(volts, -1.15);
}

void printCm(const __FlashStringHelper* name, float cm) {
  Serial.print(name);
  if (cm < 0) {
    Serial.print(F(">80    "));
  } else {
    Serial.print((int)cm);
    Serial.print(F("cm   "));
  }
}

// ================= MENU =================

void showMenu() {
  Serial.println();
  Serial.println(F("===== SUMO BOT TESTER ====="));
  Serial.println(F("1 = Sharp IR sensors (cm)"));
  Serial.println(F("2 = Line sensors"));
  Serial.println(F("3 = Motor test (wheels off the ground!)"));
  Serial.println(F("0 = Stop / menu"));
  Serial.println(F("Type a number and press Enter."));
}

// ================= SETUP =================

void setup() {
  Serial.begin(9600);

  pinMode(M1_R, OUTPUT); pinMode(M1_L, OUTPUT);
  pinMode(M2_R, OUTPUT); pinMode(M2_L, OUTPUT);
  stopMove();

  showMenu();
}

// ================= START A TEST =================

void startLineTest() {
  Serial.println(F("LINE TEST: keep BOTH sensors over BLACK. Calibrating in 3 seconds..."));
  delay(3000);

  long sumL = 0, sumR = 0;
  for (int i = 0; i < 30; i++) {
    sumL += analogRead(LINE_L);
    sumR += analogRead(LINE_R);
    delay(20);
  }
  baseL = sumL / 30;
  baseR = sumR / 30;

  Serial.print(F("Black baseline  L: ")); Serial.print(baseL);
  Serial.print(F("  R: ")); Serial.println(baseR);
  Serial.println(F("Slide each sensor over the white line. (0 = stop)"));
}

void startMotorTest() {
  Serial.println(F("MOTOR TEST: lift the wheels off the ground!"));
  Serial.println(F("Starting in 3 seconds... (send 0 to cancel)"));
  motorCountdown = true;
  motorStep = 0;
  motorStepStart = millis();
}

// ================= TESTS =================

void runSharpTest() {
  if (millis() - lastPrint < 1000) return;
  lastPrint = millis();

  printCm(F("L:"), readCm(SH_L));
  printCm(F("F:"), readCm(SH_C));
  printCm(F("R:"), readCm(SH_R));
  Serial.println();
}

void runLineTest() {
  if (millis() - lastPrint < 300) return;
  lastPrint = millis();

  int l = analogRead(LINE_L);
  int r = analogRead(LINE_R);
  bool whiteL = abs(l - baseL) > EDGE_MARGIN;
  bool whiteR = abs(r - baseR) > EDGE_MARGIN;

  Serial.print(F("L: ")); Serial.print(l);
  Serial.print(whiteL ? F(" WHITE") : F(" black"));
  Serial.print(F("    R: ")); Serial.print(r);
  Serial.println(whiteR ? F(" WHITE") : F(" black"));
}

void runMotorTest() {
  unsigned long now = millis();

  // 3 second countdown before anything moves
  if (motorCountdown) {
    if (now - motorStepStart < 3000) return;
    motorCountdown = false;
    motorStepStart = now;
    motorStep = 0;
  }

  // Each step runs for 2 seconds, then 1 second stopped
  const unsigned long RUN_MS = 2000, PAUSE_MS = 1000;
  unsigned long t = now - motorStepStart;

  if (t < RUN_MS) {
    switch (motorStep) {
      case 0: if (t < 50) Serial.println(F("LEFT motor FORWARD"));   move(TEST_SPEED, 0);              break;
      case 1: if (t < 50) Serial.println(F("LEFT motor REVERSE"));   move(-TEST_SPEED, 0);             break;
      case 2: if (t < 50) Serial.println(F("RIGHT motor FORWARD"));  move(0, TEST_SPEED);              break;
      case 3: if (t < 50) Serial.println(F("RIGHT motor REVERSE"));  move(0, -TEST_SPEED);             break;
      case 4: if (t < 50) Serial.println(F("BOTH FORWARD"));         move(TEST_SPEED, TEST_SPEED);     break;
      case 5: if (t < 50) Serial.println(F("BOTH REVERSE"));         move(-TEST_SPEED, -TEST_SPEED);   break;
    }
  } else if (t < RUN_MS + PAUSE_MS) {
    stopMove();
  } else {
    motorStep++;
    motorStepStart = now;
    if (motorStep > 5) {
      stopMove();
      Serial.println(F("Motor test finished."));
      mode = '0';
      showMenu();
    }
  }
}

// ================= MAIN LOOP =================

void loop() {
  // Read a command from the Serial Monitor
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '0' || c == '1' || c == '2' || c == '3') {
      stopMove();
      motorCountdown = false;
      mode = c;
      lastPrint = 0;

      if (c == '0') showMenu();
      else if (c == '1') Serial.println(F("SHARP TEST (0 = stop)"));
      else if (c == '2') startLineTest();
      else if (c == '3') startMotorTest();
    }
  }

  // Run the selected test
  if (mode == '1') runSharpTest();
  else if (mode == '2') runLineTest();
  else if (mode == '3') runMotorTest();
}
