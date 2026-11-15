// LCD console: a 16x2 I2C LCD (Uno, 0x27) controlled by text commands over serial (115200).
// Driven by dashboard/uno-lcd.html via Web Serial. One command per line; every line gets a reply.
//   L0 <text> | L1 <text>   set a row (padded/truncated to 16 chars)
//   CLR                     clear the screen and stop any animation
//   BL 0|1                  backlight off/on
//   PIX <128 hex digits>    20x16 pixel image: 8 tiles x 8 rows, 5-bit rows (bit 4 = leftmost)
//   WALK 0|1                walking-character animation off/on
// Replies: "READY" at boot, then "OK" or "ERR <reason>".
#include <Wire.h>
#include <avr/pgmspace.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte TILE_W = 5, TILE_H = 8, SPRITE_W = 10, SPRITE_H = 16, WIN_TILES = 3;
const byte PIX_COL = 6;                    // 4-column image block centred on 16 columns
const int MAX_X = 16 * TILE_W - SPRITE_W;  // rightmost walker pixel position
const byte STEP_PX = 2;
const unsigned long FRAME_MS = 140;

// Walker sprite, '#' = lit pixel, facing right. Rows 0-5 head, 6-10 torso, 11-15 legs.
const char head[6][11] PROGMEM = {
  "...####...", "..######..", "..##.#.#..", "..######..", "...####...", "....##....",
};
const char torso[2][5][11] PROGMEM = {
  {".########.", "#.######..", "#.######.#", "..######.#", "..######.."},
  {".########.", "..######.#", "#.######.#", "#.######..", "..######.."},
};
const char legs[4][5][11] PROGMEM = {
  {"..##..##..", "..##..###.", ".##....##.", ".##....##.", "###....###"},
  {"..##..##..", "..##..##..", "..##..##..", "..##..##..", ".###..###."},
  {"..##..##..", ".###..##..", ".##....##.", ".##....##.", "###....###"},
  {"..##..##..", "..##..##..", "..##..##..", "..##..##..", ".###..###."},
};

char buf[140];
byte len = 0;
bool overflow = false;  // current line too long: discard it up to the next newline
bool walking = false;
int posX = 0, dir = 1, prevCol = -1;
byte frame = 0;
unsigned long lastFrame = 0;

void showRow(byte row, const char *text) {
  char out[17];
  snprintf(out, sizeof out, "%-16.16s", text);
  lcd.setCursor(0, row);
  lcd.print(out);
}

byte hexVal(char c) { return c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10; }

// Parses 128 hex digits into 8 glyphs and shows them as a 4x2 block.
bool showPixels(const char *hex) {
  if (strlen(hex) != 128) return false;
  byte tiles[8][TILE_H];
  for (byte i = 0; i < 64; i++) {
    if (!isxdigit(hex[2 * i]) || !isxdigit(hex[2 * i + 1])) return false;
    tiles[i / TILE_H][i % TILE_H] = (hexVal(hex[2 * i]) << 4 | hexVal(hex[2 * i + 1])) & 0x1F;
  }
  for (byte i = 0; i < 8; i++) lcd.createChar(i, tiles[i]);
  for (byte r = 0; r < 2; r++) {
    lcd.setCursor(PIX_COL, r);
    for (byte t = 0; t < 4; t++) lcd.write((byte)(r * 4 + t));
  }
  return true;
}

bool spritePixel(byte x, byte y) {
  const char *row;
  if (y < 6) row = head[y];
  else if (y < 11) row = torso[(frame / 2) & 1][y - 6];
  else row = legs[frame & 3][y - 11];
  return pgm_read_byte(row + x) == '#';
}

void drawWalker() {
  byte offset = posX % TILE_W;
  int col = posX / TILE_W;
  byte tiles[WIN_TILES * 2][TILE_H] = {};
  for (byte y = 0; y < SPRITE_H; y++)
    for (byte x = 0; x < SPRITE_W; x++) {
      if (!spritePixel(dir > 0 ? x : SPRITE_W - 1 - x, y)) continue;
      byte wx = offset + x;
      tiles[(y / TILE_H) * WIN_TILES + wx / TILE_W][y % TILE_H] |= 1 << (TILE_W - 1 - wx % TILE_W);
    }
  for (byte i = 0; i < WIN_TILES * 2; i++) lcd.createChar(i, tiles[i]);
  if (prevCol >= 0)
    for (byte t = 0; t < WIN_TILES; t++) {
      int c = prevCol + t;
      if (c < col || c >= col + WIN_TILES)
        for (byte r = 0; r < 2; r++) { lcd.setCursor(c, r); lcd.print(' '); }
    }
  for (byte r = 0; r < 2; r++)
    for (byte t = 0; t < WIN_TILES && col + t < 16; t++) {
      lcd.setCursor(col + t, r);
      lcd.write((byte)(r * WIN_TILES + t));
    }
  prevCol = col;
}

void stepWalker() {
  drawWalker();
  frame = (frame + 1) & 3;
  posX += dir * STEP_PX;
  if (posX >= MAX_X) { posX = MAX_X; dir = -1; }
  else if (posX <= 0) { posX = 0; dir = 1; }
}

void startWalker(bool on) {
  walking = on;
  lcd.clear();
  posX = 0; dir = 1; frame = 0; prevCol = -1;
}

void handleLine(const char *s) {
  if (!strncmp(s, "L0 ", 3) || !strncmp(s, "L1 ", 3)) {
    if (walking) { Serial.println(F("ERR walking")); return; }
    showRow(s[1] - '0', s + 3);
  } else if (!strcmp(s, "CLR")) {
    startWalker(false);
  } else if (!strcmp(s, "BL 0") || !strcmp(s, "BL 1")) {
    if (s[3] == '1') lcd.backlight(); else lcd.noBacklight();
  } else if (!strncmp(s, "PIX ", 4)) {
    walking = false;
    lcd.clear();
    if (!showPixels(s + 4)) { Serial.println(F("ERR pix")); return; }
  } else if (!strcmp(s, "WALK 0") || !strcmp(s, "WALK 1")) {
    startWalker(s[5] == '1');
  } else {
    Serial.println(F("ERR unknown"));
    return;
  }
  Serial.println(F("OK"));
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  showRow(0, "LCD console");
  showRow(1, "open dashboard");
  Serial.println(F("READY"));
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { buf[len] = 0; handleLine(buf); len = 0; }
    else if (c != '\r') { if (len < sizeof buf - 1) buf[len++] = c; else len = sizeof buf; }
  }
  if (len == sizeof buf) { len = 0; Serial.println(F("ERR too long")); }  // overflowed line discarded at next \n boundary
  if (walking && millis() - lastFrame >= FRAME_MS) { lastFrame = millis(); stepWalker(); }
}
