// Step 2: debounced buttons driving colors.
//
// This is the same button logic as the TronIdentityDisc disc, minus ESP-NOW:
//   - press white, blue or green: the strip changes to that color right away
//   - press black: the strip goes off
//   - hold any button LONG_HOLD_MS: the strip goes off
// Step 3 adds the costume's real gestures (double taps, timed holds).
//
// Every event is also printed, so it works with no LED strip attached.

#include <Arduino.h>
#include <FastLED.h>

#include "config.h"

static const uint8_t kPins[BUTTON_COUNT] = BUTTON_PINS;
static const char* kNames[BUTTON_COUNT] = BUTTON_NAMES;
static const CRGB kColors[BUTTON_COUNT] = {
  CRGB::Black, CRGB(0xE6FFFF), CRGB(0x6FC3DF), CRGB(0x39FF14)  // black (off), white, blue, green
};

CRGB leds[NUM_LEDS];

// Debouncing: the raw pin can flicker for a few ms when a button moves.
// `lastRead` follows the raw pin and `changedAt` notes when it last moved.
// Only when the raw pin has stayed put for DEBOUNCE_MS does `stable` take
// the new value, and that is the moment a press or release "happens".
struct Button {
  bool     stable = HIGH;     // debounced state: HIGH released, LOW pressed
  bool     lastRead = HIGH;   // raw pin on the previous loop
  uint32_t changedAt = 0;     // millis() when the raw pin last changed
  uint32_t pressedAt = 0;     // millis() when the current press began
  bool     holdFired = false; // so a long hold only fires once
};
static Button buttons[BUTTON_COUNT];

static void showColor(int8_t color) {  // -1 = off
  fill_solid(leds, NUM_LEDS, color < 0 ? CRGB::Black : kColors[color]);
  FastLED.show();
}

static void onPress(uint8_t i) {
  if (i == 0) {
    Serial.printf("%-5s pressed  -> lights off\n", kNames[i]);
    showColor(-1);
    return;
  }
  Serial.printf("%-5s pressed  -> color %s\n", kNames[i], kNames[i]);
  showColor(i);
}

static void onRelease(uint8_t i, uint32_t heldMs) {
  Serial.printf("%-5s released after %lu ms\n", kNames[i], (unsigned long)heldMs);
}

static void onHold(uint8_t i) {
  Serial.printf("%-5s held %d s -> lights off\n", kNames[i], LONG_HOLD_MS / 1000);
  showColor(-1);
}

static void pollButtons() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    Button& b = buttons[i];
    bool r = digitalRead(kPins[i]);

    // 1. Raw pin moved: restart the settle timer.
    if (r != b.lastRead) {
      b.lastRead = r;
      b.changedAt = now;
    }

    // 2. Raw pin has been still long enough and differs from what we
    //    believe: accept it. This is a real press or release.
    if (r != b.stable && now - b.changedAt >= DEBOUNCE_MS) {
      b.stable = r;
      if (b.stable == LOW) {
        b.pressedAt = now;
        b.holdFired = false;
        onPress(i);
      } else {
        onRelease(i, now - b.pressedAt);
      }
    }

    // 3. Still held: has it been held long enough?
    if (b.stable == LOW && !b.holdFired && now - b.pressedAt >= LONG_HOLD_MS) {
      b.holdFired = true;
      onHold(i);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);  // let USB CDC enumerate so the first lines aren't lost

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) pinMode(kPins[i], INPUT_PULLUP);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);
  FastLED.setBrightness(LED_BRIGHTNESS);
  showColor(-1);

  Serial.println("\nButtonsAndColors step 2: press a color button for its color, black or hold 3 s for off");
}

void loop() {
  pollButtons();
}
