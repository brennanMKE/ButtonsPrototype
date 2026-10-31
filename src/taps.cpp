// Step 3: taps and holds, the costume's real gestures.
//
// Same buttons and gestures as the TronIdentityDisc disc, minus ESP-NOW:
//   white, blue, green   tap: that color; double tap: its pair (yellow, red,
//                        orange); hold 1 s / tap then hold 1 s: the same for
//                        both costumes (here: two quick flashes, then the color)
//   black                tap: outer ring on/off; double tap: flicker; triple
//                        tap: motion; hold 3 s: lights off; tap again: back on
//
// The strip stands in for the disc: the first MAIN_LEDS are the inner C
// (always on, solid) and the rest are the outer ring (on/off and effects).
//
// Two layers: debounce each button into press/release events (as in step 2),
// then a small state machine turns those into gestures: 1-3 taps, or a hold
// after 1-2 taps. Taps wait TAP_GAP_MS after release in case another is
// coming, except the last tap a button understands, which fires on release.
//
//   IDLE --press--> DOWN --release--> GAP --TAP_GAP_MS--> taps(n)
//                    |  ^               |
//                 held  +--press same---+   (n + 1)
//                    v
//                  HELD --> hold(n)

#include <Arduino.h>
#include <FastLED.h>

#include "config.h"

static const uint8_t kPins[BUTTON_COUNT] = BUTTON_PINS;
static const char* kNames[BUTTON_COUNT] = BUTTON_NAMES;

// ---- Colors ----
enum Color : uint8_t { WHITE, YELLOW, BLUE, RED, GREEN, ORANGE, OFF };
static const CRGB kColors[] = {
  CRGB(0xE6FFFF), CRGB(0xFFE64D), CRGB(0x6FC3DF), CRGB(0xFF2A2A), CRGB(0x39FF14), CRGB(0xDF740C)
};
static const char* kColorNames[] = { "white", "yellow", "blue", "red", "green", "orange", "off" };

// The hex values above are screen colors. A screen dims low channel values
// (gamma), but an LED shows them close to face value, so red's 0x2A of green
// and blue turns it pink. Applying the same curve here makes the strip match
// the screen: small values drop to almost nothing, full ones stay full.
static CRGB ledColors[sizeof(kColors) / sizeof(kColors[0])];

static uint8_t gammaByte(uint8_t v) {
  return (uint8_t)(powf(v / 255.0f, LED_GAMMA) * 255.0f + 0.5f);
}

static void buildLedColors() {
  for (uint8_t i = 0; i < sizeof(kColors) / sizeof(kColors[0]); i++) {
    const CRGB& c = kColors[i];
    ledColors[i] = CRGB(gammaByte(c.r), gammaByte(c.g), gammaByte(c.b));
    Serial.printf("  %-6s #%02X%02X%02X -> LED %02X %02X %02X\n", kColorNames[i], c.r, c.g, c.b,
                  ledColors[i].r, ledColors[i].g, ledColors[i].b);
  }
}

// Button order matches BUTTON_PINS: black, white, blue, green.
static const uint8_t BTN_BLACK = 0;
static const uint8_t kPrimary[BUTTON_COUNT]   = { OFF, WHITE,  BLUE, GREEN };
static const uint8_t kSecondary[BUTTON_COUNT] = { OFF, YELLOW, RED,  ORANGE };

// ---- Drawing ----
#define MAIN_LEDS (NUM_LEDS / 2)
#define RING_LEDS (NUM_LEDS - MAIN_LEDS)

enum RingMode : uint8_t { RING_SOLID, RING_FLICKER, RING_MOTION };
static const char* kRingNames[] = { "solid", "flicker", "motion" };

CRGB leds[NUM_LEDS];
static uint8_t  currentColor = BLUE;
static uint8_t  lastColor = BLUE;   // what tapping black brings back after off
static bool     ringOn = true;
static uint8_t  ringMode = RING_SOLID;
static uint32_t flashMs = 0;        // start of the "both costumes" flashes
static bool     flashing = false;

// Erratic flicker: the whole ring holds a level for a random time, mostly
// full, sometimes dipping, with the odd LED dropping out.
static uint8_t ringFlickerLevel(uint32_t now) {
  static uint8_t  level = 255;
  static uint32_t at = 0, holdMs = 0;
  if (now - at >= holdMs) {
    at = now;
    if (random8() < 80) { level = random8(20, 170); holdMs = random8(20, 90); }
    else                { level = 255;              holdMs = random16(60, 450); }
  }
  return level;
}

static void drawRing(uint32_t now) {
  CRGB* ring = leds + MAIN_LEDS;
  if (!ringOn || currentColor == OFF) {
    fill_solid(ring, RING_LEDS, CRGB::Black);
    return;
  }
  uint8_t flicker = ringMode == RING_FLICKER ? ringFlickerLevel(now) : 255;
  // Motion: a bright head travels round once per RING_MOTION_MS with a
  // fading tail, over a dim base.
  uint16_t head = (uint32_t)(now % RING_MOTION_MS) * RING_LEDS / RING_MOTION_MS;
  const uint16_t tail = RING_LEDS / 3;
  for (uint16_t i = 0; i < RING_LEDS; i++) {
    uint8_t level = 255;
    if (ringMode == RING_MOTION) {
      uint16_t behind = (head + RING_LEDS - i) % RING_LEDS;
      level = behind < tail ? 255 - (uint32_t)behind * (255 - RING_MOTION_FLOOR) / tail : RING_MOTION_FLOOR;
    } else if (ringMode == RING_FLICKER) {
      level = random8() < 6 ? flicker / 4 : flicker;
    }
    CRGB c = ledColors[currentColor];
    c.nscale8_video(level);
    ring[i] = c;
  }
}

static void render() {
  uint32_t now = millis();
  // Two quick flashes of the whole strip: on, off, on, off.
  if (flashing && now - flashMs < 4 * FLASH_MS) {
    bool on = ((now - flashMs) / FLASH_MS) % 2 == 0;
    fill_solid(leds, NUM_LEDS, on ? ledColors[currentColor] : CRGB::Black);
    FastLED.show();
    return;
  }
  flashing = false;
  fill_solid(leds, MAIN_LEDS, currentColor == OFF ? CRGB::Black : ledColors[currentColor]);
  drawRing(now);
  FastLED.show();
}

static void setColor(uint8_t color) {
  if (color != OFF) lastColor = color;
  currentColor = color;
}

static void setRing(bool on, uint8_t mode) {
  ringOn = on;
  ringMode = mode;
  Serial.printf("  ring %s (%s)\n", on ? "on" : "off", kRingNames[mode]);
}

// ---- Gestures ----
static uint8_t maxTaps(uint8_t b) { return b == BTN_BLACK ? 3 : 2; }

// 0 = no hold gesture for this button and tap count.
static uint32_t holdMs(uint8_t b, uint8_t taps) {
  if (b == BTN_BLACK) return taps == 1 ? LONG_HOLD_MS : 0;
  return SHORT_HOLD_MS;
}

static void onTaps(uint8_t b, uint8_t taps) {
  if (b != BTN_BLACK) {
    uint8_t color = taps == 1 ? kPrimary[b] : kSecondary[b];
    Serial.printf("%-5s x%u -> own costume %s\n", kNames[b], taps, kColorNames[color]);
    setColor(color);
    return;
  }
  if (currentColor == OFF) {   // black brings a dark costume back on
    Serial.printf("%-5s x%u -> back on, %s\n", kNames[b], taps, kColorNames[lastColor]);
    setColor(lastColor);
    return;
  }
  // Double and triple tap pick an effect; repeating it goes back to solid.
  Serial.printf("%-5s x%u\n", kNames[b], taps);
  switch (taps) {
    case 1: setRing(!ringOn, ringMode); break;
    case 2: setRing(true, ringMode == RING_FLICKER ? RING_SOLID : RING_FLICKER); break;
    case 3: setRing(true, ringMode == RING_MOTION ? RING_SOLID : RING_MOTION); break;
  }
}

static void onHold(uint8_t b, uint8_t taps) {
  if (b == BTN_BLACK) {
    Serial.printf("%-5s held %d s -> lights off\n", kNames[b], LONG_HOLD_MS / 1000);
    setColor(OFF);
    return;
  }
  uint8_t color = taps == 1 ? kPrimary[b] : kSecondary[b];
  Serial.printf("%-5s x%u + hold -> BOTH costumes %s\n", kNames[b], taps, kColorNames[color]);
  setColor(color);
  flashing = true;
  flashMs = millis();
}

enum SeqState : uint8_t { SEQ_IDLE, SEQ_DOWN, SEQ_GAP, SEQ_HELD };
static SeqState seqState = SEQ_IDLE;
static uint8_t  seqButton = 0;
static uint8_t  seqTaps = 0;
static uint32_t seqAt = 0;   // press time in DOWN, release time in GAP

static void onPress(uint8_t b, uint32_t now) {
  if (seqState == SEQ_GAP && b == seqButton) {
    seqTaps++;
  } else if (seqState == SEQ_IDLE || seqState == SEQ_GAP) {
    if (seqState == SEQ_GAP) onTaps(seqButton, seqTaps);  // another button ends the last sequence
    seqButton = b;
    seqTaps = 1;
  } else {
    return;  // a second button pressed mid-press: ignored
  }
  seqState = SEQ_DOWN;
  seqAt = now;
}

static void onRelease(uint8_t b, uint32_t now) {
  if (b != seqButton) return;
  if (seqState == SEQ_HELD) {
    seqState = SEQ_IDLE;
  } else if (seqState == SEQ_DOWN) {
    if (seqTaps >= maxTaps(b)) {
      seqState = SEQ_IDLE;
      onTaps(b, seqTaps);
    } else {
      seqState = SEQ_GAP;
      seqAt = now;
    }
  }
}

static void runSequence(uint32_t now) {
  if (seqState == SEQ_DOWN) {
    uint32_t h = holdMs(seqButton, seqTaps);
    if (h && now - seqAt >= h) {
      seqState = SEQ_HELD;
      onHold(seqButton, seqTaps);
    }
  } else if (seqState == SEQ_GAP && now - seqAt >= TAP_GAP_MS) {
    seqState = SEQ_IDLE;
    onTaps(seqButton, seqTaps);
  }
}

// ---- Debouncing (as in step 2) ----
struct Button {
  bool     stable = HIGH;
  bool     lastRead = HIGH;
  uint32_t changedAt = 0;
  bool     ignore = false;   // held since boot: no events until released
};
static Button buttons[BUTTON_COUNT];

static void pollButtons() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    Button& b = buttons[i];
    bool r = digitalRead(kPins[i]);
    if (r != b.lastRead) {
      b.lastRead = r;
      b.changedAt = now;
    }
    if (r != b.stable && now - b.changedAt >= DEBOUNCE_MS) {
      b.stable = r;
      if (b.ignore) {
        if (b.stable == HIGH) b.ignore = false;
      } else if (b.stable == LOW) {
        onPress(i, now);
      } else {
        onRelease(i, now);
      }
    }
  }
  runSequence(now);
}

void setup() {
  Serial.begin(115200);
  delay(500);  // let USB CDC enumerate so the first lines aren't lost

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    pinMode(kPins[i], INPUT_PULLUP);
    // Still held from power-on: ignore it until released, so it is neither
    // a tap nor a hold. On the disc this is the press that woke it.
    if (digitalRead(kPins[i]) == LOW) {
      buttons[i].stable = buttons[i].lastRead = LOW;
      buttons[i].ignore = true;
    }
  }

  Serial.printf("\nButtonsAndColors step 3: taps and holds (gap %d ms, hold %d ms)\n",
                TAP_GAP_MS, SHORT_HOLD_MS);
  Serial.printf("Colors, gamma %.1f:\n", LED_GAMMA);
  buildLedColors();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);
  FastLED.setBrightness(LED_BRIGHTNESS);
  render();

  Serial.printf("Starting %s, ring on\n", kColorNames[currentColor]);
}

void loop() {
  pollButtons();

  static uint32_t tFrame = 0;
  uint32_t now = millis();
  if (now - tFrame >= FRAME_MS) {  // ring animation
    tFrame = now;
    render();
  }
}
