# SECDB — Phase 4C Virtual Breadboard: Capacitor Measurement Integration

## Status

Phase 4C is complete at the virtual/LTspice level. The 100 nF–100 µF capacitor range was integrated with a dedicated TMUX1104 charge/discharge controller, the TI OPA391 macro-model, the PA0 ADC path, calibrated threshold timing, and open/short/leakage/above-range cases.

This is simulation verification. Physical validation remains pending, so a capacitor must not be described as hardware-tested from these results.

## Virtual circuit

```text
                         TMUX1104 #2
3.3 V -- 10.01 kΩ -- S1   (charge)
GND   --  1.001 kΩ - S2   (discharge)
                      D -------- SENSE -------- CX -------- GND
                                   |
                                   +--> OPA391 follower --> 100 Ω --> PA0

S3, S4: unused
EN = 0: SENSE isolated from both charge and discharge paths
```

`CX` is connected with its positive terminal at `SENSE` and negative terminal at ground when a polarized capacitor is tested.

## Virtual parts added in Phase 4C

| Quantity | Part | Function |
|---:|---|---|
| 1 | TMUX1104DGS | Select charge, discharge, or isolation state |
| 1 | VSSOP-10 to DIP adapter | Required for a future solderless breadboard |
| 1 | 10 kΩ, 0.1% resistor | Controlled capacitor charging path |
| 1 | 1 kΩ, 0.1% resistor | Controlled capacitor discharge path |
| 1 | 100 nF ceramic capacitor | Local TMUX1104 supply decoupling |
| 1 | polarized-safe two-terminal socket | Connection for the unknown capacitor |

The Phase 4A OPA391, its 100 nF decoupling capacitor, 100 Ω PA0 isolation resistor, and 3.3 V/GND rails are reused.

## TMUX1104DGS pin connections

> Phase 5A supersession: this separate capacitor TMUX is removed. Capacitor charge and discharge become S2 and S4 of the shared R/C source TMUX1104. This table remains the Phase 4 simulation record.

The following table applies to the DGS 10-pin VSSOP package.

| Pin | Name | Virtual connection |
|---:|---|---|
| 1 | A0 | Black Pill PB6 |
| 2 | S1 | 10.01 kΩ charging resistor; other end to 3.3 V |
| 3 | GND | Ground rail |
| 4 | S3 | Leave unconnected in Version 1 |
| 5 | EN | Black Pill PB8 |
| 6 | VDD | 3.3 V rail |
| 7 | S4 | Leave unconnected in Version 1 |
| 8 | D | `SENSE` node and positive capacitor terminal |
| 9 | S2 | 1.001 kΩ discharge resistor; other end to ground |
| 10 | A1 | Black Pill PB7 |

Place 100 nF directly between TMUX pins 6 and 3. PB6, PB7, and PB8 are provisional ordinary GPIO assignments that do not overlap the current TFT, USB, SWD, or Phase 4B switch allocations.

## Control truth table

| PB8/EN | PB7/A1 | PB6/A0 | State |
|---:|---:|---:|---|
| 0 | X | X | Both paths disconnected; safe/isolate state |
| 1 | 0 | 0 | S1 selected: charge through 10.01 kΩ |
| 1 | 0 | 1 | S2 selected: discharge through 1.001 kΩ |
| 1 | 1 | 0 | S3 unused; firmware must avoid |
| 1 | 1 | 1 | S4 unused; firmware must avoid |

Firmware must set `EN=0` before changing A1/A0. `EN` must also remain low during reset until the control GPIOs have known output levels.

## OPA391 and PA0 connection

```text
TMUX pin 8 / SENSE -> OPA391DBV pin 3 (+IN)
OPA391 pin 1 OUT   -> OPA391 pin 4 (-IN)
OPA391 pin 1 OUT   -> 100 Ω -> Black Pill PA0 / ADC1_IN0
OPA391 pin 5       -> 3.3 V
OPA391 pin 2       -> GND
```

The buffer prevents the ADC input from directly disturbing the timing node.

## Threshold and calibrated equation

The initial 12-bit ADC threshold is code 2589:

```text
threshold ratio = 2589 / 4095 = 0.6322344322
threshold voltage = 3.3 × 2589 / 4095 = 2.086373626 V
Kthreshold = -ln(1 - 2589/4095) = 1.000309588
```

The conservative simulated charge path is:

```text
Rcharge_path = 10.010 kΩ + 9.5 Ω = 10.0195 kΩ
```

Therefore the calibrated firmware equation is:

```text
CX = Tthreshold / (Rcharge_path × Kthreshold)
CX = Tthreshold / 10022.601916
```

The physical board must replace `Rcharge_path` with a calibrated value. Using `C = T/10 kΩ` is acceptable only as an approximate first display value, not the final calibrated calculation.

## LTspice integration model

Main file: `Virtual_Breadboard_Capacitor_Integration.cir`

It includes:

- TI OPAx391 PSpice macro-model Rev. B;
- 10.01 kΩ charge and 1.001 kΩ discharge resistors, representing +0.1% limits;
- 9.5 Ω selected TMUX path resistance;
- 50 pF socket/PCB/input parasitic capacitance;
- 100 Ω OPA391-to-PA0 isolation;
- 7 pF ADC input load;
- ADC thresholds at codes 2585, 2589, and 2593 to represent the ±4 LSB allocation;
- 100 nF, 1 µF, 10 µF, and 100 µF test values.

LTspice used Gmin stepping to find the OPA391 operating point near the ground rail, then completed every stepped transient and measurement without convergence failure.

## Verified charge results

| Actual CX | Code-2585 time | Code-2589 time | Code-2593 time | Calibrated estimate | Center error |
|---:|---:|---:|---:|---:|---:|
| 100 nF | 0.999606 ms | 1.002265 ms | 1.004931 ms | 100.000463 nF | +0.000463% |
| 1 µF | 9.996059 ms | 10.022637 ms | 10.049286 ms | 1.00000351 µF | +0.000351% |
| 10 µF | 99.960585 ms | 100.226353 ms | 100.492827 ms | 10.0000333 µF | +0.000333% |
| 100 µF | 0.999606 s | 1.002263 s | 1.004928 s | 100.000318 µF | +0.000318% |

The unusually small center error is the result of using the exact modeled path and threshold factor; it is not a physical accuracy claim. The code-2585 to code-2593 spread is approximately ±0.266%. Firmware polling latency, timer quantization, component tolerance, leakage, and calibration dominate the real error.

## Verified discharge results

The discharge criterion is 1% of 3.3 V, or 33 mV (approximately ADC code 41).

| CX | Time to 33 mV | Final voltage at end of integrated run |
|---:|---:|---:|
| 100 nF | 0.465582 ms | 1.11 nV |
| 1 µF | 4.653731 ms | 1.11 nV |
| 10 µF | 46.535173 ms | 1.11 nV |
| 100 µF | 0.465350 s | 1.11 nV |

A first-pass discharge timeout of 0.6 s therefore covers the supported 100 µF upper endpoint in this model. The firmware must verify `ADC code <= 41` before beginning the charge timer; it must not assume that waiting alone guarantees discharge.

## Fault and leakage verification

Fault file: `Virtual_Breadboard_Capacitor_Faults.cir`

| Case | Verified result | Interpretation |
|---|---:|---|
| Open socket represented by 50 pF parasitic | threshold in 0.501 µs | Open, absent, or far below 100 nF |
| 10 mΩ short | 3.294 µV after 5 s | Short candidate; threshold never reached |
| 1 mF capacitor | 1.2965 V after 5 s | Above range; threshold never reached |
| 100 µF with 1 MΩ leakage | threshold in 1.009554 s; apparent 100.728 µF | Small positive error |
| 100 µF with 100 kΩ leakage | threshold in 1.083145 s; apparent 108.070 µF | Significant positive error |
| 100 µF with 15 kΩ leakage | 1.97798 V after 5 s | Threshold impossible; excessive leakage/timeout |

The 5-second timeout cannot uniquely distinguish short, excessive leakage, reversed connection, poor contact, or capacitance above range. Firmware must report a combined diagnostic instead of claiming a single cause.

## Firmware measurement sequence

1. Set capacitor TMUX `EN=0`.
2. Select S2 with `A1:A0=01`, then set `EN=1` to discharge through 1.001 kΩ.
3. Read PA0 until code is at or below 41. Abort with `DISCHARGE_FAILED_OR_EXTERNAL_VOLTAGE` if this does not occur within the allowed interval.
4. Set `EN=0` before changing the address.
5. Select S1 with `A1:A0=00`.
6. Start a 1 MHz hardware timer and enable the TMUX charge path.
7. Poll PA0 no slower than every 10 µs. Stop the timer at the first reading at or above code 2589.
8. If the crossing occurs before 0.25 ms, report `OPEN_OR_BELOW_100NF`.
9. If no crossing occurs by 5 s, disable the TMUX and report `TIMEOUT_SHORT_LEAKAGE_OR_ABOVE_RANGE`.
10. Otherwise calculate capacitance with the calibrated `Rcharge_path` and threshold factor.
11. Set `EN=0` after the reading, then repeat the discharge operation before allowing removal.

Suggested calculation:

```text
if 0.25 ms <= elapsed <= 5 s:
    capacitance_F = elapsed_s / calibrated_timing_denominator
```

The acceptance band for the declared 100 nF–100 µF range should be applied after calibration, not merely from the broad 0.25 ms/5 s diagnostic limits.

## Safety and limitations

- Only unpowered capacitors may be inserted.
- The user must externally and safely discharge a previously charged capacitor before insertion; the onboard 1 kΩ path is a measurement reset path, not a high-energy discharge tool.
- The test voltage is 3.3 V maximum. Observe electrolytic polarity.
- The circuit does not measure ESR or remaining capacitor lifetime.
- Leakage changes the apparent capacitance and can cause timeout.
- The TMUX is modeled by conservative selected-path resistance, not a complete transient macro-model.
- Breadboard contact resistance, contamination leakage, rail tolerance, noise, and real STM32 timing are not yet validated.

Official component references:

- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- OPA391 product/model page: https://www.ti.com/product/OPA391
- STM32F401CC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf

## Phase 4C conclusion

The capacitor charge/discharge network is ready for virtual acceptance. Its pin-level wiring, safe control states, exact timing equation, supported range, discharge criterion, and non-unique fault rules are now documented and numerically verified.

Phase 4D diode and Phase 4E known-pinout BJT integration have now been completed separately. Phase 4 is complete; the next stage is Phase 5 shared analog-front-end consolidation and GPIO/switch-resource audit.
