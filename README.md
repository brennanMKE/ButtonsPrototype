# Buttons Prototype

A breadboard practice project for wiring and reading push buttons on an ESP32-C3. It uses the same four buttons and pins as the [TronIdentityDisc](../TronIdentityDisc) disc, so once this works on a breadboard, the same layout goes onto the protoboard.

| Button | GPIO | Job |
|---|---|---|
| 1 | 5 | Black: outer ring, off, wake |
| 2 | 6 | White (double tap: yellow) |
| 3 | 7 | Green (double tap: orange) |
| 4 | 10 | Blue (double tap: red) |

LED strip data is on GPIO 4, next to 3.3V, 5V and GND on the ESP32-C3 Super Mini.

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
| 2, 8, 9 | Strapping pins: their level at power-up decides how the chip boots. 8 also drives the Super Mini's onboard LED, and 9 is the BOOT button. |
| 4 | Used for the LED strip in this project |
| 18, 19 | USB, the cable you're flashing over |
| 21 | Serial TX: the bootloader drives it at power-up, so a pressed button would fight it |

Only GPIO 0-5 can wake the C3 from deep sleep. Black is on GPIO 5 for that reason: it will double as the wake button in the future low-power mode.

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
 │  GPIO 5 ├──────────────────── [black button] ────┘      │      │      │
 │  GPIO 6 ├──────────────────── [white button] ───────────┘      │      │
 │  GPIO 7 ├──────────────────── [green button] ──────────────────┘      │
 │ GPIO 10 ├──────────────────── [blue  button] ─────────────────────────┘
 │         │
 │  GPIO 4 ├──[330 Ω]──── strip DIN     (step 2, optional)
 │      5V ├───────────── strip 5V
 │     GND ├───────────── strip GND
 └─────────┘
```

1. Put the ESP32-C3 across the breadboard's center gap so each row of pins has its own holes.
2. Run a jumper from a **GND** pin on the board to the breadboard's blue (–) rail.
3. Put each button across the center gap too. That keeps its legs in separate rows.
4. For each button, connect one leg to its GPIO and the **diagonally opposite** leg to the blue (–) rail.
5. Put the buttons in order (black, white, green, blue) so it matches the disc. On the Super Mini they're all on one side (5, 6, 7, 8, 9, 10, 20, 21); just skip over 8 and 9.

Pin labels differ between boards (some print `0`, others `GPIO0` or `IO0`), so check your board's pinout.

### Lining the buttons up (ESP32-C3 Super Mini)

On the Super Mini the four buttons come off one side, in order, with one gap where GPIO 8 (onboard LED) and 9 (BOOT) are skipped. The other side has the power pins and LED data; GPIO 0 and 1 there are free for later.

```
              USB-C
          ┌───┴───┴───┐
   strip ─┤ 5V     5  ├──── [black]  ─┐
  rails  ─┤ GND    6  ├──── [white]  ─┤
         ─┤ 3V3    7  ├──── [blue ]  ─┤
 LED DIN ─┤ 4      8  │  (onboard LED, leave free)
          │ 3      9  │  (BOOT, leave free)
          │ 2     10  ├──── [green]  ─┤
          │ 1     20  │               │
          │ 0     21  │               │
          └───────────┘               │
                                 GND rail
```

Each button: one leg to its GPIO, the diagonally opposite leg to the GND rail. Put the four buttons side by side in order, and the jumpers from 5/6/7 and from 10 are all short. This is the same layout to copy onto the protoboard, with room left over for additions such as an LED power switch.

This pin order is the common Super Mini layout. Check it against your board's silkscreen before wiring.

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
blue  GPIO 10 -> LOW  (pressed)
blue  GPIO 10 -> HIGH (released)
blue  GPIO 10 -> LOW  (pressed)
      ^ 3 changes in 20 ms: that's bounce
blue  GPIO 10 -> HIGH (released)
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
blue  pressed  -> color blue
blue  released after 182 ms
green pressed  -> color green
green held 3 s -> lights off
green released after 3410 ms
```

Notice that each press prints once now. The bounce is gone. The code in [src/buttons.cpp](src/buttons.cpp) walks through how debouncing and hold detection work, one step at a time.

Things to try:

- Change `DEBOUNCE_MS` to `0` in `include/config.h` and watch presses double up.
- Change `LONG_HOLD_MS` to see what hold time feels right.
- Press two buttons at once.

## Step 3: Taps and Holds

The experiment for the costume controls ([TronIdentityDisc docs/controls.md](../TronIdentityDisc/docs/controls.md)). Same wiring as step 2. The strip stands in for the disc: its first half is the inner C, and its second half is the outer ring.

```bash
pio run -e taps -t upload
pio device monitor
```

| Button | Gesture | Costume meaning | What this demo does |
|---|---|---|---|
| White, blue, green | Tap | Own costume: primary color | That color |
| | Double tap | Own costume: secondary (yellow, red, orange) | That color |
| | Hold 1 s | Both costumes: primary | Two quick flashes, then the color |
| | Tap, then hold 1 s | Both costumes: secondary | Two quick flashes, then the color |
| Black | Tap | Outer ring on/off | Second half on/off |
| | Double tap | Ring flicker (again: steady) | Second half flickers |
| | Triple tap | Ring motion (again: steady) | A dot circles the second half |
| | Hold 3 s | Own costume off | Off; tap black to come back |

```
blue  x1 -> own costume blue
blue  x2 -> own costume red
green x1 + hold -> BOTH costumes green
black x2
  ring on (flicker)
black held 3 s -> lights off
```

Things to notice:

- **A tap waits.** After you let go, it waits `TAP_GAP_MS` (350 ms) to see whether another tap is coming before it counts. That pause is the cost of telling single from double.
- **The last tap a button understands fires on release**, without waiting: the 2nd tap on a color button, the 3rd on black.
- **A hold fires while you're still holding**: 1 s on a color button, 3 s on black.
- **A different button during the wait** counts the first sequence as taps and starts fresh.

Tune `TAP_GAP_MS` and `SHORT_HOLD_MS` in `include/config.h`. Try 250 and 500 for the gap: shorter feels snappier, but double and triple taps get harder to hit. Whatever feels right here goes into the costume firmware.

[src/taps.cpp](src/taps.cpp) has the state machine drawn at the top. It works in two layers: debounced press/release events (as in step 2), then a small state machine that turns them into gestures.

## Moving to the Protoboard

The protoboard is the same circuit made permanent:

- Keep the same order: black GPIO 5, white GPIO 6, green GPIO 7, blue GPIO 10. LED data on GPIO 4.
- Run one shared GND wire along the buttons, the same job as the breadboard rail.
- Keep using diagonal legs.
- Before soldering the board on, check each button with the multimeter: GPIO pad to GND should beep only while pressed.
- After soldering, flash **step 1** again. It's the quickest way to prove every button still works.

## Project Layout

```
include/config.h      Pins, names, debounce, hold and tap-gap times, LED settings
src/wiring_test.cpp   Step 1: raw pin changes and bounce
src/buttons.cpp       Step 2: debounced press / release / hold with colors
src/taps.cpp          Step 3: the costume gestures (taps, holds, outer ring)
```
