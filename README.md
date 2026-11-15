# LCD Dashboard (Arduino Uno / ESP32)

A browser dashboard that controls a 16x2 I2C LCD on an Arduino Uno or an ESP32 over Web Serial: text lines, backlight, live clock, a 20x16 pixel-art editor with image import, and a walking-character animation.

**Live page:** https://anakinskywalker0.github.io/uno-lcd-dashboard/ (Chrome or Edge only; Web Serial is not in Firefox or Safari)

## Wiring

| LCD (I2C backpack) | Arduino Uno | ESP32 DevKit |
|---|---|---|
| GND | GND | GND |
| VCC | 5V | VIN (5V) |
| SDA | A4 | GPIO21 (P21) |
| SCL | A5 | GPIO22 (P22) |

ESP32 pins are 3.3V-only and the backpack's I2C pull-ups usually go to 5V. This works on most boards in practice; use a level shifter on SDA and SCL to be safe.

The sketch assumes I2C address `0x27`; change it in `firmware/lcd_console/lcd_console.ino` if yours is `0x3F`.

## Setup

1. Install the `LiquidCrystal_I2C` library and the core for your board (`arduino:avr` or `esp32:esp32`).
2. Upload `firmware/lcd_console` (same sketch for both boards; use `--fqbn esp32:esp32:esp32` for the ESP32):
   ```
   arduino-cli compile --fqbn arduino:avr:uno firmware/lcd_console
   arduino-cli upload -p <PORT> --fqbn arduino:avr:uno firmware/lcd_console
   ```
3. Open the live page (or serve `index.html` on `localhost`), click **Connect board**, and pick the board's port. Close any other program using that port first.

## Serial protocol (115200 baud, one command per line)

| Command | Effect |
|---|---|
| `L0 <text>` / `L1 <text>` | Set a row (padded or truncated to 16 chars) |
| `CLR` | Clear the screen and stop animations |
| `BL 0` / `BL 1` | Backlight off / on |
| `PIX <128 hex digits>` | Show a 20x16 image: 8 tiles x 8 rows, 5-bit rows, bit 4 = leftmost pixel |
| `WALK 0` / `WALK 1` | Walker animation off / on |

The board replies `READY` at boot, then `OK` or `ERR <reason>` per command. The LCD has only 8 custom characters, which is why images are limited to 20x16 pixels.
