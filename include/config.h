#pragma once

// ---- Buttons ----
// Same pins and order as the TronIdentityDisc disc. Board order: black 5, white 6, green 7, blue 10.
// Each button connects its GPIO to GND when pressed. The internal pull-up
// holds the pin HIGH otherwise, so:  HIGH = released, LOW = pressed.
#define BUTTON_COUNT   4
#define BUTTON_PINS    { 5, 6, 10, 7 }   // black on 5 can wake from deep sleep; skips 8 (LED), 9 (BOOT)
#define BUTTON_NAMES   { "black", "white", "blue", "green" }

#define DEBOUNCE_MS    30      // a pin must hold still this long to count
#define LONG_HOLD_MS   3000    // black held this long: lights off (step 2: any button)
#define SHORT_HOLD_MS  1000    // step 3: color button held this long = "both costumes"
#define TAP_GAP_MS     350     // step 3: a further tap must start within this of the last release

// ---- LEDs (step 2 only) ----
// WS2812B strip data on GPIO 4 through a 330 ohm resistor, like the disc.
// On USB power keep the count small and the brightness low.
#define LED_PIN        4
#define NUM_LEDS       8
#define LED_BRIGHTNESS 64
#define LED_MAX_MA     800     // matches the disc; more than a USB 2.0 port's 500 mA

// ---- Step 3 only ----
// The strip's first half is the disc's inner C, its second half the outer ring.
#define FRAME_MS            20    // redraw rate for the ring effects (50 fps)
#define RING_MOTION_MS      1000  // motion effect: one turn of the bright head
#define RING_MOTION_FLOOR   50    // motion effect: base level behind the tail (0-255)
#define FLASH_MS            80    // "both costumes": each of the two flashes, on and off
#define LED_GAMMA           2.6f  // screen hex -> LED; 2.2-2.8 is typical, higher = more saturated
