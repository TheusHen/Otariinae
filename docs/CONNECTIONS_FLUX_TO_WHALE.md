# Flux ↔ WhaleShark connection plan

This document defines the recommended V1 interconnect between the **Flux** board (RP2040) and the **WhaleShark** board (ESP32).

The goal here is not just “make the boards talk”, but to make the wiring safe, debuggable, and compatible with normal ESP32 boot behavior.

## Design choice

For V1, the cleanest inter-board link is:

- **I2C** for control and telemetry
- **1 interrupt/status line** from Flux to WhaleShark
- **1 enable/maintenance line** from WhaleShark to Flux
- **shared ground**
- **single-source 5V power**, with Flux regulating itself to 3.3V through its onboard regulator

I2C was chosen because Flux already exposes dedicated I2C pins on the schematic, it keeps wiring simple, and it is enough for relay control, state reporting, log coordination, time sync, and matrix commands.

## Recommended ESP32 pins on WhaleShark

These pins were selected to avoid common ESP32 boot/strapping trouble:

| WhaleShark (ESP32) | Role | Why |
|---|---|---|
| GPIO21 | I2C SDA | Standard ESP32 I2C SDA choice |
| GPIO22 | I2C SCL | Standard ESP32 I2C SCL choice |
| GPIO27 | FLUX_INT | Safe digital input, not a boot strap pin |
| GPIO26 | FLUX_ENABLE | Safe digital output, not a boot strap pin |
| 5V rail | Flux power input | Lets Flux use its onboard 3.3V regulator |
| GND | Common reference | Mandatory for logic integrity |

## Confirmed Flux pins from the schematic image

From the Flux schematic image, the following pins are already clearly defined:

| Flux signal | RP2040 GPIO |
|---|---|
| I2C0_SDA | GPIO4 |
| I2C0_SCL | GPIO5 |
| SD_MISO | GPIO16 |
| SD_CS | GPIO17 |
| SD_SCK | GPIO18 |
| SD_MOSI | GPIO19 |
| SD_DET | GPIO20 |

The relay and matrix assignments below are the V1 firmware defaults:

| Flux function | RP2040 GPIO |
|---|---|
| Main aquarium pump relay | GPIO10 |
| Aquaponics peristaltic pump relay | GPIO11 |
| Spare relay 1 | GPIO12 |
| Spare relay 2 | GPIO13 |
| Flux interrupt/status to WhaleShark | GPIO14 |
| Flux enable / maintenance input from WhaleShark | GPIO15 |
| MAX7219 DIN | GPIO6 |
| MAX7219 CLK | GPIO7 |
| MAX7219 CS | GPIO8 |

## Final wiring: board to board

### Required lines

| WhaleShark pin | Connect to Flux pin | Direction | Notes |
|---|---|---|---|
| GPIO21 | GPIO4 (I2C0_SDA) | bidirectional | I2C data |
| GPIO22 | GPIO5 (I2C0_SCL) | output/clock | I2C clock |
| GPIO27 | GPIO14 | Flux → Whale | Optional but recommended; signals fault, new log state, or heartbeat edge |
| GPIO26 | GPIO15 | Whale → Flux | Lets WhaleShark place Flux in maintenance-safe mode |
| 5V | VBUS / 5V input | power | Use only one 5V source at a time |
| GND | GND | reference | Required |

### Optional lines

| WhaleShark pin | Connect to Flux pin | Purpose |
|---|---|---|
| GPIO25 | RUN or spare reset conditioning circuit | Optional hard reset strategy for Flux |
| GPIO33 | Spare ADC monitor | Optional voltage / current monitor return if you add it later |

## Power notes

### Recommended V1 power model

- Feed the whole system from one main regulated **5V rail**.
- WhaleShark uses its own 3.3V domain.
- Flux receives **5V on VBUS / 5V input**, then its onboard regulator generates **3.3V**.
- Both boards must share **GND**.

### Important

Do **not** power Flux from both:

- USB-C **and**
- WhaleShark 5V interconnect

at the same time unless you intentionally add power-path protection.

For development, use either:

- Flux by USB-C alone, or
- system 5V rail through the interconnect

## Why these ESP32 pins were chosen

The ESP32 has several pins that are tied to boot behavior or internal flash, so they are bad candidates for inter-board control if avoidable.

Avoid for this link whenever possible:

- GPIO0
- GPIO2
- GPIO5
- GPIO12
- GPIO15
- GPIO6 to GPIO11
- GPIO1 and GPIO3 if you still want easy serial debugging

That is why the plan uses **GPIO21 / GPIO22 / GPIO26 / GPIO27** on WhaleShark.

## Internal V1 role split

### WhaleShark owns

- Wi-Fi
- NTP / internet time
- high-level automation state
- HTTP API
- secret-protected control endpoints
- consolidated sensor state
- long-term scheduling decisions

### Flux owns

- relay switching
- MAX7219 display rendering
- MicroSD logging
- local failsafe behavior
- keeping outputs in a safe state if WhaleShark disappears

## Recommended harness

For the interconnect cable, use a dedicated 6-pin or 8-pin harness:

### Minimum 6-pin

1. 5V
2. GND
3. SDA
4. SCL
5. FLUX_INT
6. FLUX_ENABLE

### Better 8-pin

1. 5V
2. GND
3. SDA
4. SCL
5. FLUX_INT
6. FLUX_ENABLE
7. spare
8. spare

This leaves room for future fault lines, analog return, or wake/reset.

## MAX7219 note

The MAX7219 is controlled locally by Flux, not by WhaleShark directly. That is intentional.

WhaleShark should only send high-level instructions such as:

- boot animation
- scroll text
- loading state
- warning icon
- timer countdown mode
- maintenance mode

Flux then translates that into actual matrix drawing.

## V1 expected behavior

On boot:

1. Flux powers up.
2. Flux brings relays to safe defaults.
3. Flux initializes the MAX7219 and shows a local boot animation.
4. WhaleShark boots, joins Wi-Fi, gets time, initializes sensors.
5. WhaleShark sends a time sync and display state to Flux.
6. Flux switches from local boot mode to commanded runtime mode.

That gives the project a cleaner startup sequence and avoids depending on Wi-Fi before showing life on the matrix.
