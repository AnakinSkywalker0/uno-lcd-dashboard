# Uno LCD Dashboard

A browser dashboard that controls a 16x2 I2C LCD on an Arduino Uno over Web Serial: text lines, backlight, live clock, a 20x16 pixel-art editor with image import, and a walking-character animation.

**Live page:** https://anakinskywalker0.github.io/uno-lcd-dashboard/ (Chrome or Edge only; Web Serial is not in Firefox or Safari)

## Wiring

| LCD (I2C backpack) | Uno |
|---|---|
| GND | GND |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

The sketch assumes I2C address `0x27`; change it in `firmware/lcd_console/lcd_console.ino` if yours is `0x3F`.

## Setup

1. Install the `LiquidCrystal_I2C` library and the `arduino:avr` core.
2. Upload `firmware/lcd_console` to the Uno:
   ```
   arduino-cli compile --fqbn arduino:avr:uno firmware/lcd_console
   arduino-cli upload -p <PORT> --fqbn arduino:avr:uno firmware/lcd_console
   ```
3. Open the live page (or serve `index.html` on `localhost`), click **Connect Uno**, and pick the Uno's port. Close any other program using that port first.

## Serial protocol (115200 baud, one command per line)

| Command | Effect |
|---|---|
| `L0 <text>` / `L1 <text>` | Set a row (padded or truncated to 16 chars) |
| `CLR` | Clear the screen and stop animations |
| `BL 0` / `BL 1` | Backlight off / on |
| `PIX <128 hex digits>` | Show a 20x16 image: 8 tiles x 8 rows, 5-bit rows, bit 4 = leftmost pixel |
| `WALK 0` / `WALK 1` | Walker animation off / on |

The Uno replies `READY` at boot, then `OK` or `ERR <reason>` per command. The LCD has only 8 custom characters, which is why images are limited to 20x16 pixels.
