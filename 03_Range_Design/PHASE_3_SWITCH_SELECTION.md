# SECDB Phase 3 — Preliminary Analog-Switch Selection

## Decision

Use **TMUX1104** as the preliminary resistor-range multiplexer for continued simulation. Keep **ADG704** as the fallback. Do not use TMUX1204, CD4053, or 74HC4051 for the precision resistor ranges without a separate error justification.

This is a preliminary engineering selection, not yet a purchase or final BOM lock. Availability and price must be checked before PCB release.

## Required function

The resistor subsystem needs one common measurement node connected to one of three reference paths:

```text
                  1 kΩ ---- S1
3.3 V ----------- 10 kΩ --- S2 ---- D ---- ADC_IN/test resistor
                  100 kΩ -- S3
                  spare ---- S4
```

A 4:1 multiplexer provides the three required ranges, one spare input, two address pins, and an enable pin.

## Datasheet comparison at approximately 3.3 V

| Parameter | TMUX1104 | ADG704 | TMUX1204 |
|---|---:|---:|---:|
| Configuration | 4:1 | 4:1 | 4:1 |
| Supply range | 1.08–5.5 V | 1.8–5.5 V | 1.08–5.5 V |
| Typical RON near 3.3 V | 3.7 Ω | approximately 4.5–5 Ω | 9 Ω |
| Maximum RON near 3.3 V | 8.8 Ω at 25°C; 9.5 Ω through 85°C | 8 Ω through 85°C at 3 V ±10% | 15 Ω through 85°C |
| Off leakage at 25°C | up to approximately 0.1 nA, depending on terminal | up to approximately 0.1 nA | tens to hundreds of nA |
| Break-before-make | Yes | Yes | Yes |
| Practical package | 10-pin VSSOP available | 10-pin µSOIC | Package-dependent |

Manufacturer sources:

- TI TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- Analog Devices ADG704 datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/adg704.pdf
- TI TMUX1204 datasheet: https://www.ti.com/lit/ds/symlink/tmux1204.pdf

## Why TMUX1104 is preferred

- It is explicitly specified at 3.3 V.
- Its leakage is extremely low, which matters when the divider current is only about 3 µA at the 1 MΩ endpoint.
- It provides break-before-make operation.
- It accepts 3.3 V STM32 logic directly.
- The 10-pin VSSOP package is more suitable for student PCB assembly than its smaller USON option.
- Its on-resistance error can be calibrated as part of the effective reference path.

The ADG704 has a slightly lower guaranteed 3 V maximum RON and is technically suitable. TMUX1104 is preferred provisionally because it is a newer precision-switch design with detailed 3.3 V and extended-temperature specifications. The final choice may change if local availability strongly favors ADG704.

## Non-ideal LTspice check

The preliminary model includes:

- 1 kΩ and 100 kΩ reference resistors at +0.1% tolerance;
- 9.5 Ω switch on-resistance;
- a conservative 2 nA aggregate leakage term on the 1 MΩ test;
- calculations both with and without calibrated effective path resistance.

This model intentionally uses resistors and current sources rather than claiming to be a transistor-level manufacturer model.

Verified results:

| Case | Firmware assumption | Reported value | Error |
|---|---|---:|---:|
| Actual 1 kΩ, +0.1% RREF and 9.5 Ω RON | Nominal 1 kΩ only | 989.61 Ω | −1.04% |
| Same 1 kΩ case | Calibrated 1010.5 Ω path | 1000.00 Ω | approximately 0% |
| Actual 1 MΩ, high range with 2 nA leakage | Nominal 100 kΩ only | 999.573 kΩ | −0.043% |
| Same 1 MΩ case | Calibrated 100.1095 kΩ path | 1.000667 MΩ | +0.067% |

This meets the provisional ±2% low/mid-range and ±5% high-range goals in simulation. Hardware calibration and temperature testing remain mandatory.

## Schematic requirements

For the later PCB:

```text
TMUX1104 VDD -> 3.3 V
TMUX1104 GND -> analog ground
0.1 µF ceramic directly between VDD and GND
A0, A1, EN -> STM32 GPIO
D -> resistor-measurement common node
S1 -> 1 kΩ reference path
S2 -> 10 kΩ reference path
S3 -> 100 kΩ reference path
S4 -> leave reserved or use only after design review
```

Firmware should disable the multiplexer before changing address lines, wait for analog settling, enable the selected channel, discard the first ADC conversion, and then average subsequent samples.

## Not yet authorized as final hardware

Before freezing this component:

1. Check stock and price from a trusted distributor.
2. Confirm the exact orderable VSSOP part number and footprint.
3. Verify the truth table and enable polarity in the schematic review.
4. Run breadboard or breakout-board tests if the package permits.
5. Calibrate each complete path using known precision resistors.
