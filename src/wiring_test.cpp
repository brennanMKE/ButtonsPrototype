// Step 1: wiring test.
//
// Reads each button pin as fast as possible and prints every change, with no
// debouncing. Use it to:
//   - confirm each button is wired to the pin you think it is
//   - see contact bounce: one press often shows up as several quick changes
//
// Nothing here is how real firmware should read a button. That is step 2.

#include <Arduino.h>

#include "config.h"

static const uint8_t kPins[BUTTON_COUNT] = BUTTON_PINS;
static const char* kNames[BUTTON_COUNT] = BUTTON_NAMES;

static bool     last[BUTTON_COUNT];
static uint32_t burstStartUs[BUTTON_COUNT];
static uint16_t burstChanges[BUTTON_COUNT];

// A "burst" is every change within this window of the first one. Physical
// presses are much further apart than this, so extra changes are bounce.
static const uint32_t BURST_US = 20000;

void setup() {
  Serial.begin(115200);
  delay(500);  // let USB CDC enumerate so the first lines aren't lost

  Serial.println("\nButtonsAndColors step 1: wiring test");
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    pinMode(kPins[i], INPUT_PULLUP);
    last[i] = digitalRead(kPins[i]);
    Serial.printf("  %-5s GPIO %u starts %s\n", kNames[i], kPins[i],
                  last[i] == LOW ? "LOW (pressed?! check wiring)" : "HIGH (released)");
  }
  Serial.println("Press each button in turn.\n");
}

void loop() {
  uint32_t now = micros();
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    bool r = digitalRead(kPins[i]);

    if (r != last[i]) {
      last[i] = r;
      if (burstChanges[i] == 0) burstStartUs[i] = now;
      burstChanges[i]++;
      Serial.printf("%-5s GPIO %u -> %s\n", kNames[i], kPins[i], r == LOW ? "LOW  (pressed)" : "HIGH (released)");
    }

    // Burst over: report how many changes a single press or release made.
    if (burstChanges[i] > 0 && now - burstStartUs[i] >= BURST_US) {
      if (burstChanges[i] > 1) {
        Serial.printf("      ^ %u changes in %lu ms: that's bounce\n",
                      burstChanges[i], (unsigned long)(BURST_US / 1000));
      }
      burstChanges[i] = 0;
    }
  }
}
