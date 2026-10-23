# Sumo-bot

Autonomous sumo robot for Arduino Uno. The sketch is in [`sumo_bot/sumo_bot.ino`](sumo_bot/sumo_bot.ino).

## Hardware

- Arduino Uno R3 (+ sensor/IO shield)
- 2x BTS7960 (IBT-2) motor driver, one per motor
- 2x 12V 280RPM DC motor
- 3x SHARP IR distance sensor 10-80cm (GP2Y0A21, analog)
- 2x single line sensor (TCRT5000 type, digital DO output)
- 1x start switch (5 second delayed start)
- 3S 11.1V LiPo

## Wiring

| Part | Pin | Arduino |
|---|---|---|
| Left BTS7960 | RPWM | D5 |
| | LPWM | D6 |
| | R_EN + L_EN (joined) | D7 |
| | VCC / GND | 5V / GND |
| | B+ / B- | Battery + / - |
| | M+ / M- | Left motor |
| Right BTS7960 | RPWM | D9 |
| | LPWM | D10 |
| | R_EN + L_EN (joined) | D8 |
| | VCC / GND | 5V / GND |
| | B+ / B- | Battery + / - |
| | M+ / M- | Right motor |
| SHARP IR (yellow wire) | Left | A0 |
| | Front | A1 |
| | Right | A2 |
| Line sensor (DO) | Left | D2 |
| | Right | D3 |
| Start switch | one leg / other leg | D4 / GND |
| Status LED | built-in | D13 |

**Power:** battery + goes to both BTS7960 B+ and the Arduino VIN (or DC jack); battery - goes to both B- and Arduino GND. All grounds must be connected together.

## How it works

1. Power on. The LED is off and the robot waits.
2. Press the switch: 5 second countdown (LED blinks).
3. Fight (LED on). Press the switch again to stop.

If your switch is an ON/OFF toggle instead of a push button, set `START_SWITCH_IS_TOGGLE` to `1`: ON starts the countdown, OFF stops.

## Testing

While the robot is waiting, open the Serial Monitor at 9600 baud:

- Sensor readings are printed every 250ms.
- Type `f`, `b`, `l` or `r` to run the motors forward, backward, left or right for half a second.

## Tuning

All settings are at the top of the sketch:

- `LEFT_MOTOR_REVERSED` / `RIGHT_MOTOR_REVERSED`: set to `1` if a wheel spins the wrong way.
- `LINE_DETECTED_STATE`: `LOW` for a white border on most TCRT5000 modules; change to `HIGH` if the line reads backwards.
- `FRONT_ATTACK_DISTANCE` / `SIDE_ATTACK_DISTANCE`: how far (cm) the robot reacts to an enemy.
- `ESCAPE_BACK_TIME` / `ESCAPE_TURN_TIME`: how far it backs up and turns at the edge.
- `attackSpeed`, `turnAttackSpeed`, `searchSpeed`, `escapeSpeed`: motor speeds (0-255).

## Power tips

- If the Arduino resets when the motors reverse suddenly, add a large capacitor (~1000µF) across the battery wires.
- Put a 10-100µF capacitor at each SHARP sensor's power pins to steady its readings.
