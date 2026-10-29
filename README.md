# ButtonsAndColors

A breadboard practice project for wiring and reading push buttons on an ESP32-C3. It uses the same four buttons and pins as the [TronIdentityDisc](../TronIdentityDisc) disc, so once this works on a breadboard, the same layout goes onto the protoboard.

| Button | GPIO | Color |
|---|---|---|
| 1 | 0 | White |
| 2 | 1 | Blue |
| 3 | 3 | Red |
| 4 | 4 | Green |

Hold any button for 3 seconds to turn the lights off. Press any button to turn them back on in that color.

## Button Refresher

### What a push button is

A momentary push button (a "tactile switch") is two pieces of metal that touch only while you press. Released, the circuit is open. Pressed, it is closed. It knows nothing about voltage. It just connects two points.

### The 4 legs

The common 6x6 mm tactile button has 4 legs but is only one switch. The legs come in two pairs, and each pair is always connected inside the button. Pressing connects pair A to pair B.

```
   A ─────── A        legs joined by a line are always connected
   │ ┌─────┐ │
     │  ●  │          pressing the button connects A to B
   │ └─────┘ │
   B ─────── B
```

Which way round the pairs go is easy to get wrong. **Use two diagonally opposite legs.** Diagonal legs are always one from each pair, so it works whichever way the button is turned. If in doubt, a multimeter on continuity (beep) mode settles it: legs that beep without pressing are a pair.

### Why one side goes to GND: pull-ups

A GPIO set as an input reads HIGH (3.3 V) or LOW (0 V). A button only connects or disconnects, so something has to decide what the pin reads when the button is released. Otherwise the pin "floats" and reads random noise.

The ESP32-C3 has a built-in **pull-up resistor** (about 45 kΩ) that you turn on with `pinMode(pin, INPUT_PULLUP)`. It gently holds the pin at 3.3 V.

```
 3.3V
  │
 [pull-up, inside the chip]
  │
  ├──────── GPIO pin
  │
 [button]
  │
 GND
```

- **Released**: nothing pulls down, so the pin reads **HIGH**.
- **Pressed**: the button connects the pin straight to GND, so it reads **LOW**.

This is called **active low**: LOW means pressed. It feels backwards at first, but it's the usual way, and it means the wiring is just *GPIO → button → GND*, with no resistors to add.

Never wire a button between a GPIO and **5V**. The C3's pins are 3.3 V only.

### Bounce

The metal contacts don't close cleanly. For a few milliseconds they bounce, so one press can look like *pressed, released, pressed, released, pressed* to a chip that reads the pin millions of times a second. Step 1 below shows this live.

**Debouncing** fixes it: only believe a change once the pin has stayed the same for a short time (30 ms here). Step 2 does this.

### Press, release, hold

Once debounced, a button gives three useful events:

- **Press**: went from released to pressed. This project changes color here, so it feels instant.
- **Release**: went from pressed to released.
- **Hold**: still pressed after some time (3 s here). This project turns the lights off.

### Pins to avoid on the ESP32-C3

| GPIO | Why |
|---|---|
| 2, 8, 9 | Strapping pins: their level at power-up decides how the chip boots. A button held at the wrong moment can stop it starting. (9 is the BOOT button.) |
| 10 | Used for the LED strip in this project |
| 18, 19 | USB, the cable you're flashing over |

GPIO 0-5 are the only pins that can wake the C3 from deep sleep, which is why the buttons are on 0, 1, 3 and 4.

## Parts

- ESP32-C3 board and USB-C cable
- Breadboard and jumper wires
- 4 tactile push buttons (6x6 mm)
- Optional for step 2: a short WS2812B LED strip (8 LEDs is plenty) and a 330 Ω resistor
- Optional: a multimeter

## Breadboard Wiring

```
  ESP32-C3                       Breadboard
 ┌─────────┐
 │     GND ├──────────────────── blue (–) rail ─────┬──────┬──────┬──────┐
 │         │                                        │      │      │      │
 │  GPIO 0 ├──────────────────── [white button] ────┘      │      │      │
 │  GPIO 1 ├──────────────────── [blue  button] ───────────┘      │      │
 │  GPIO 3 ├──────────────────── [red   button] ──────────────────┘      │
 │  GPIO 4 ├──────────────────── [green button] ─────────────────────────┘
 │         │
 │ GPIO 10 ├──[330 Ω]──── strip DIN     (step 2, optional)
 │      5V ├───────────── strip 5V
 │     GND ├───────────── strip GND
 └─────────┘
```

1. Put the ESP32-C3 across the breadboard's center gap so each row of pins has its own holes.
2. Run a jumper from a **GND** pin on the board to the breadboard's blue (–) rail.
3. Put each button across the center gap too. That keeps its legs in separate rows.
4. For each button, connect one leg to its GPIO and the **diagonally opposite** leg to the blue (–) rail.
5. Put the buttons in order (white, blue, red, green) so it matches the disc.

Pin labels differ between boards (some print `0`, others `GPIO0` or `IO0`), so check your board's pinout.

The strip on USB power: keep it short and dim. The code caps it at 400 mA and brightness 64.

## Step 1: Wiring Test

Prints every raw change on the four pins, with no debouncing.

```bash
pio run -e wiring-test -t upload
pio device monitor
```

At startup every pin should say `HIGH (released)`. If one says `LOW`, that button is shorted or wired to the wrong legs (same pair, so it's always "pressed"): turn it 90° or use the diagonal legs.

Press and release each button. You should see something like:

```
red   GPIO 3 -> LOW  (pressed)
red   GPIO 3 -> HIGH (released)
red   GPIO 3 -> LOW  (pressed)
      ^ 3 changes in 20 ms: that's bounce
red   GPIO 3 -> HIGH (released)
```

| Symptom | Likely cause |
|---|---|
| Nothing prints for a button | Leg not in the row you think, GND rail not connected, or wrong GPIO |
| Wrong name prints | Wire on the wrong GPIO: swap it, or swap the order in `include/config.h` |
| Pin stuck LOW | Button legs from the same pair, or wire straight to GND |
| Random changes with nothing pressed | Loose jumper |

Bounce is normal. Some buttons bounce a lot and some barely at all.

## Step 2: Buttons and Colors

Debounced presses, releases and holds, driving the LED strip. Same logic as the disc firmware, without the radio.

```bash
pio run -e buttons -t upload
pio device monitor
```

```
red   pressed  -> color red
red   released after 182 ms
green pressed  -> color green
green held 3 s -> lights off
green released after 3410 ms
```

Notice that each press prints once now. The bounce is gone. The code in [src/buttons.cpp](src/buttons.cpp) walks through how debouncing and hold detection work, one step at a time.

Things to try:

- Change `DEBOUNCE_MS` to `0` in `include/config.h` and watch presses double up.
- Change `LONG_HOLD_MS` to see what hold time feels right.
- Press two buttons at once.

## Moving to the Protoboard

The protoboard is the same circuit made permanent:

- Keep the same order: white GPIO 0, blue GPIO 1, red GPIO 3, green GPIO 4.
- Run one shared GND wire along the buttons, the same job as the breadboard rail.
- Keep using diagonal legs.
- Before soldering the board on, check each button with the multimeter: GPIO pad to GND should beep only while pressed.
- After soldering, flash **step 1** again. It's the quickest way to prove every button still works.

## Project Layout

```
include/config.h      Pins, names, debounce and hold times, LED settings
src/wiring_test.cpp   Step 1: raw pin changes and bounce
src/buttons.cpp       Step 2: debounced press / release / hold with colors
```
