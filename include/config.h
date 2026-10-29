#pragma once

// ---- Buttons ----
// Same pins and order as the TronIdentityDisc disc: white, blue, red, green.
// Each button connects its GPIO to GND when pressed. The internal pull-up
// holds the pin HIGH otherwise, so:  HIGH = released, LOW = pressed.
#define BUTTON_COUNT   4
#define BUTTON_PINS    { 0, 1, 3, 4 }
#define BUTTON_NAMES   { "white", "blue", "red", "green" }

#define DEBOUNCE_MS    30      // a pin must hold still this long to count
#define LONG_HOLD_MS   3000    // hold any button this long: lights off

// ---- LEDs (step 2 only) ----
// WS2812B strip data on GPIO 10 through a 330 ohm resistor, like the disc.
// On USB power keep the count small and the brightness low.
#define LED_PIN        10
#define NUM_LEDS       8
#define LED_BRIGHTNESS 64
#define LED_MAX_MA     400     // stay well inside what a USB port can supply
