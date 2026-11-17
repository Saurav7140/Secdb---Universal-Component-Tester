# SECDB — Phase 4D Virtual Breadboard: Two-Polarity Diode Integration

## Status

Phase 4D is complete at the virtual/LTspice level. A diode is driven in both directions through two protected STM32 GPIO legs, both test terminals are read sequentially through a TMUX1104 and the shared OPA391/PA0 path, and the healthy, reversed, open, short, and resistive-leakage signatures have been verified.

This is simulation verification, not physical validation. The voltage bands remain first-pass diagnostic rules until real devices are measured.

## Virtual circuit

```text
PA8/DRIVE_A -- 1.001 kΩ -- terminal A -- DUT -- terminal B -- 1.001 kΩ -- PA9/DRIVE_B
                                |                    |
                               S1                   S2
                                \                    /
                                  TMUX1104 #3
                                      D
                                      |
                             OPA391 follower
                                      |
                                    100 Ω
                                      |
                                 PA0 / ADC1_IN0
```

The STM32's approximately 50 Ω output resistance was included in each simulated drive leg. The physical external resistor remains 1 kΩ, preferably 0.1%.

Only one terminal is connected to the ADC buffer at a time. The two OPA391 instances used per simulated device are observational copies of the two sequential TMUX read states; the physical design still uses one shared OPA391.

## Virtual parts added in Phase 4D

| Quantity | Part | Function |
|---:|---|---|
| 1 | TMUX1104DGS | Select terminal A or terminal B for ADC reading |
| 1 | VSSOP-10 to DIP adapter | Required for a future solderless breadboard |
| 2 | 1 kΩ, 0.1% resistors | Limit current from either drive direction |
| 1 | 100 nF ceramic capacitor | TMUX1104 local supply decoupling |
| 1 | 100 kΩ resistor | Pull TMUX `EN` low during reset |
| 1 | two-terminal test socket | Diode/DUT connection |

The OPA391, its decoupling capacitor, 100 Ω ADC isolation resistor, and PA0 are reused from Phase 4A.

## GPIO and test-terminal connections

| STM32 pin | Connection | Measurement use |
|---|---|---|
| PA8 | Through 1.001 kΩ to terminal A | `DRIVE_A`: high, low, or analog/high-impedance |
| PA9 | Through 1.001 kΩ to terminal B | `DRIVE_B`: high, low, or analog/high-impedance |
| PA0 | OPA391 output through 100 Ω | Sequential terminal ADC reading |

PA8 and PA9 were provisional Phase 4 drive assignments. Phase 5A supersedes them with PA6 and PA10 because PA9 is reserved for USB OTG FS VBUS sensing. The electrical simulation results remain applicable; use the Phase 5 master pin map for later schematic and firmware work.

## TMUX1104DGS sense-multiplexer connections

> Phase 5A supersession: the separate diode sense TMUX1104 is replaced by channels S2/S3 of the universal TMUX1108. Use the Phase 5 master architecture for later schematic and firmware work.

The table applies to the DGS 10-pin VSSOP package.

| Pin | Name | Virtual connection |
|---:|---|---|
| 1 | A0 | Black Pill PB3 |
| 2 | S1 | Diode terminal A |
| 3 | GND | Ground rail |
| 4 | S3 | Leave unconnected in Version 1 |
| 5 | EN | Black Pill PB5 and 100 kΩ pull-down to ground |
| 6 | VDD | 3.3 V rail |
| 7 | S4 | Leave unconnected in Version 1 |
| 8 | D | Shared OPA391 `+IN` measurement node |
| 9 | S2 | Diode terminal B |
| 10 | A1 | Black Pill PB4 |

Place 100 nF directly between pins 6 and 3. PB3/PB4 must be configured as ordinary GPIO if only two-wire SWD debugging is retained; optional JTAG/SWO use would require a later pin reassignment.

## Sense-multiplexer truth table

| PB5/EN | PB4/A1 | PB3/A0 | ADC source |
|---:|---:|---:|---|
| 0 | X | X | No diode terminal selected |
| 1 | 0 | 0 | S1: terminal A |
| 1 | 0 | 1 | S2: terminal B |
| 1 | 1 | 0 | S3 unused; firmware must avoid |
| 1 | 1 | 1 | S4 unused; firmware must avoid |

The resistor and capacitor TMUX `EN` signals must remain low throughout a diode test so only the diode sense multiplexer drives the shared buffer input.

## Required two-polarity test

| Test | PA8 / A | PA9 / B | Directed differential |
|---|---|---|---|
| A-to-B | High | Low | `VA - VB` |
| Safe changeover | Analog input, no pull | Analog input, no pull | No result |
| B-to-A | Low | High | `VB - VA` |

The low output must be configured before the high output in each active state. Before reversing polarity, both pins must first return to analog/no-pull mode. The two 1 kΩ resistors limit current even if firmware briefly makes the wrong drive combination.

For a shorted DUT, the conservative simulated current is:

```text
Rleg = 1.001 kΩ + 50 Ω = 1.051 kΩ
Ishort = 3.3 V / (2 × 1.051 kΩ + 10 mΩ)
       = 1.56993 mA
```

This remains below the 2 mA Phase 3 acceptance target.

## ADC calculation

For each polarity, firmware sequentially reads both terminals:

```text
VA = VDDA × code_A / 4095
VB = VDDA × code_B / 4095

V_AB_test = VA - VB      when A is driven high
V_BA_test = VB - VA      when B is driven high
```

ADC codes may also be compared directly for ratiometric blocking detection:

```text
diff_code = abs(code_high_terminal - code_low_terminal)
blocking threshold = 0.90 × calibrated_open_diff_code
```

At 3.3 V, the 0.10 V short threshold is approximately 124 ADC codes. The short decision still requires both polarity differentials to be below the threshold.

## Integrated LTspice model

Main file: `Virtual_Breadboard_Diode_Integration.cir`

The model includes:

- TI OPAx391 PSpice macro-model Rev. B;
- LTspice 1N4148 and 1N5819 models;
- an illustrative red-LED model;
- 1.001 kΩ current-limiting resistors;
- 50 Ω GPIO output resistance per leg;
- 9.5 Ω TMUX sense-path resistance;
- 100 Ω buffer-to-PA0 isolation and 7 pF ADC loading;
- temperature steps of -10 °C, 25 °C, and 60 °C.

The conducting cases completed without convergence errors.

## Verified device results through the ADC path

| Device/model | -10 °C | 25 °C | 60 °C | 25 °C current |
|---|---:|---:|---:|---:|
| 1N4148 | 0.667598 V | 0.600193 V | 0.531974 V | 1.28440 mA |
| 1N5819 | 0.210587 V | 0.141775 V | 0.077136 V | 1.50249 mA |
| Illustrative red LED | 1.990907 V | 2.005210 V | 2.018366 V | 0.615989 mA |

For the 25 °C 1N4148 case, the sequential terminal readings were:

| Terminal | Voltage | Equivalent ADC code |
|---|---:|---:|
| A | 1.950089 V | 2419.88 |
| B | 1.349896 V | 1675.10 |
| Difference | 0.600193 V | 744.78 codes |

The model values are current- and temperature-dependent references, not universal diode specifications. In particular, the 1N5819 forward voltage drops below 0.10 V at 60 °C; this demonstrates why one low reading cannot independently prove a short.

## Two-polarity and fault verification

Fault file: `Virtual_Breadboard_Diode_Faults.cir`

The TI op-amp macro-model became stuck in source stepping when several rail-level blocking cases were nested with temperature and polarity steps. The completed fault deck therefore uses unity-gain behavioral buffer copies with the same 9.5 Ω sense path, 100 Ω ADC isolation, and deliberately opposing ±1 µA ADC loading on the open/short references. This limitation is documented rather than treating an incomplete vendor-model run as evidence.

| DUT | A-to-B differential | B-to-A differential | Interpretation |
|---|---:|---:|---|
| Healthy 1N4148, anode at A | 0.596310 V | 3.299995 V | Conducting then blocking; anode at A |
| 1 GΩ open | 3.299793 V | 3.300193 V | Open/absent/poor contact |
| 10 mΩ short | -0.184 mV | +0.216 mV | Short; tiny sign change is modeled output-loading offset |
| 100 kΩ resistive leakage | 3.232062 V | 3.232062 V | Still blocking/open-like in this coarse test |
| 10 kΩ resistive leakage | 2.726822 V | 2.726822 V | Fails blocking; leaky/bidirectional/unknown |

The raw behavioral open-node model slightly exceeds the ideal rails in one loading direction, producing calculated codes from about -0.12 to 4095.12. Real ADC readings must be clamped to the valid integer range 0–4095 before further processing.

## Frozen first-pass classification

First perform an open-socket calibration using both drive polarities and store `OPEN_DIFF`. Then define:

```text
VBLOCK = 0.90 × OPEN_DIFF
VSHORT = 0.10 V
```

| A-to-B result | B-to-A result | Report |
|---|---|---|
| `> VBLOCK` | `> VBLOCK` | `OPEN_OR_NO_CONTACT` |
| `< VSHORT` | `< VSHORT` | `SHORT` |
| conducting | `> VBLOCK` | `DIODE_OR_JUNCTION_ANODE_A` |
| `> VBLOCK` | conducting | `DIODE_OR_JUNCTION_ANODE_B` |
| between short and blocking | between short and blocking | `LEAKY_BIDIRECTIONAL_OR_UNKNOWN` |

For a confirmed unidirectional junction, retain the tentative labels:

| Forward voltage | Display label |
|---:|---|
| 0.05–0.50 V | Schottky-like |
| 0.45–0.90 V | Silicon-like |
| 1.20–2.90 V | LED-like |
| Other conducting value | Diode/junction — type uncertain |

The 0.45–0.50 V overlap and any reading within 20 mV of a boundary must be labelled uncertain. High-forward-voltage white/blue LEDs close to the 3.3 V rail remain unsupported because they can resemble an open circuit.

## Firmware sequence

1. Disable every measurement TMUX and configure PA8/PA9 as analog inputs with no pulls.
2. Confirm that the inserted component is unpowered and not charged.
3. Configure PA9 low, then PA8 high for the A-to-B test.
4. Select terminal A on the sense TMUX, discard the first ADC conversion, and average the readings.
5. Disable the sense TMUX, select terminal B, then repeat the ADC reading.
6. Disable the sense TMUX and return both drive pins to analog/no-pull mode.
7. Allow a break-before-make interval.
8. Configure PA8 low, then PA9 high for the B-to-A test.
9. Read terminal A and B sequentially using the same process.
10. Disable the sense TMUX and return both drive pins to analog/no-pull mode.
11. Calculate both directed differentials and apply the two-reading classification table.
12. Store or display the raw terminal readings and forward voltage with the diagnostic label.

## Safety and limitations

- Only unpowered, discharged two-terminal components may be inserted.
- The terminals are limited to the board's 0–3.3 V domain; do not connect an external powered circuit.
- Two-polarity testing diagnoses junction behavior but does not prove rated reverse voltage, maximum current, power rating, or long-term health.
- The circuit is not a precision reverse-leakage meter.
- The LED comparison model is illustrative only.
- Physical contact resistance, GPIO high/low levels, VDDA accuracy, noise, and real-device distributions remain unverified.

Official component references:

- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- OPA391 product/model page: https://www.ti.com/product/OPA391
- STM32F401CC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf

## Phase 4D conclusion

The diode subsystem is ready for virtual acceptance. Both drive polarities, sequential terminal sensing, current limiting, differential calculations, type guard bands, and honest fault wording are now defined and numerically verified.

Phase 4E known-pinout NPN/PNP BJT integration has now been completed separately. Phase 4 is complete; the next stage is Phase 5 shared analog-front-end consolidation and GPIO/switch-resource audit.
