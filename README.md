# Sumo-bot

Autonomous sumo robot for Arduino Uno.

- [`sumo_bot/sumo_bot.ino`](sumo_bot/sumo_bot.ino): the fighting code.
- [`sumobot_tester/sumobot_tester.ino`](sumobot_tester/sumobot_tester.ino): bench tester for the Sharp sensors, line sensors and motors (Serial Monitor, 9600 baud). Both sketches use the same wiring.

## Hardware

- Arduino Uno R3 (+ sensor/IO shield)
- 2x BTS7960 (IBT-2) motor driver, one per motor
- 2x 12V 280RPM DC motor
- 3x SHARP IR distance sensor 10-80cm (GP2Y0A21, analog)
- 2x single line sensor (analog output)
- Start switch: optional (not fitted yet)
- 3S 11.1V LiPo

## Wiring

| Part | Pin | Arduino |
|---|---|---|
| Left BTS7960 | RPWM | D5 |
| | LPWM | D6 |
| | R_EN, L_EN, VCC | 5V (each its own wire) |
| | GND | GND |
| | B+ / B- | Battery + / - |
| | M+ / M- | Left motor |
| Right BTS7960 | RPWM | D9 |
| | LPWM | D10 |
| | R_EN, L_EN, VCC | 5V (each its own wire) |
| | GND | GND |
| | B+ / B- | Battery + / - |
| | M+ / M- | Right motor |
| SHARP IR (yellow wire) | Right | A0 |
| | Front | A1 |
| | Left | A2 |
| Line sensor (analog) | Left | A4 |
| | Right | A5 |
| Start switch (optional) | one leg / other leg | D4 / GND |
| Status LED | built-in | D13 |

"Left" and "right" are as seen from behind the bot, looking forward.

**Power:** battery + goes to both BTS7960 B+ and the Arduino VIN (or DC jack); battery - goes to both B- and Arduino GND. All grounds must be connected together.

## How it works

**No switch (default, `USE_START_SWITCH 0`):**

1. Place the bot in the ring and power it on (or press the Uno's reset button).
2. The 5 second countdown starts right away (LED blinks). During the last second the line sensors calibrate on the black ring, so keep the bot still on black.
3. Fight (LED on). To stop it, switch off the battery. Pressing reset restarts the countdown, so it does **not** stop the bot.

On USB, type `x` in the Serial Monitor to stop and `g` to start the countdown again. **Lift the wheels when plugging in on the bench**: the motors start 5 seconds after power on.

**With a switch (`USE_START_SWITCH 1`, switch between D4 and GND):**

1. Power on. The LED is off and the robot waits.
2. Place the bot in the ring and press the switch: 5 second countdown, then fight.
3. Press the switch again to stop.

If your switch is an ON/OFF toggle instead of a push button, also set `START_SWITCH_IS_TOGGLE` to `1`: ON starts the countdown, OFF stops.

## Testing

Use `sumobot_tester.ino` for bench tests. The main sketch also has a test while stopped (type `x` first): open the Serial Monitor at 9600 baud:

- Sensor readings are printed every 250ms.
- Type `f`, `b`, `l` or `r` to run the motors forward, backward, left or right for half a second.

## Tuning

All settings are at the top of the sketch:

- `LEFT_MOTOR_REVERSED` / `RIGHT_MOTOR_REVERSED`: set to `1` if a wheel spins the wrong way.
- `EDGE_MARGIN`: how much a line sensor reading must change from black to count as the white line (use the tester to pick a value).
- `FRONT_ATTACK_DISTANCE` / `SIDE_ATTACK_DISTANCE`: how far (cm) the robot reacts to an enemy.
- `ESCAPE_BACK_TIME` / `ESCAPE_TURN_TIME`: how far it backs up and turns at the edge.
- `attackSpeed`, `turnAttackSpeed`, `searchSpeed`, `escapeSpeed`: motor speeds (0-255).

## Power tips

- If the Arduino resets when the motors reverse suddenly, add a large capacitor (~1000µF) across the battery wires.
- Put a 10-100µF capacitor at each SHARP sensor's power pins to steady its readings.
