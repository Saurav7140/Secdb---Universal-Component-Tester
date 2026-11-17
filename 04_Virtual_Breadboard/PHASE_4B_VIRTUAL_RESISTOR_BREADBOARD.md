# SECDB — Phase 4B Virtual Breadboard: Resistor Measurement Integration

## Status

Phase 4B is complete at the virtual/LTspice level. The 1 kΩ, 10 kΩ, and 100 kΩ resistor ranges were integrated with a conservative TMUX1104 path, TI's official OPAx391 macro-model, a 100 Ω PA0 isolation resistor, ADC loading, ADC uncertainty, and open/short fault models.

The supported 100 Ω–1 MΩ range remains inside the ±2% modeled electronics target. This is simulation verification, not physical breadboard validation.

## Virtual circuit

```text
                   1k 0.1% --- S1
3.3 V rail ------- 10k 0.1% -- S2    TMUX1104
                   100k 0.1% - S3       D -------- SENSE -------- RX -------- GND
                                 S4 unused             |
                                                       +--> OPA391 follower --> 100Ω --> PA0
```

Only one reference path is selected at a time. The unknown resistor `RX` is connected from `SENSE` to ground.

## Virtual parts added in Phase 4B

| Quantity | Part | Function |
|---:|---|---|
| 1 | TMUX1104DGS | Select one of three reference resistors |
| 1 | VSSOP-10 to DIP adapter | Required for a future solderless breadboard |
| 1 | 1 kΩ, 0.1% resistor | Low range reference |
| 1 | 10 kΩ, 0.1% resistor | Middle range reference |
| 1 | 100 kΩ, 0.1% resistor | High range reference |
| 1 | 100 nF ceramic capacitor | TMUX1104 supply decoupling |
| 1 | two-terminal test socket/virtual terminals | Connection for `RX` |

The OPA391 buffer, its 100 nF capacitor, 100 Ω output resistor, PA0, and 3.3 V/GND rails are carried forward from Phase 4A.

## TMUX1104DGS pin connections

> Phase 5A supersession: the resistor-range TMUX1104 is retained as the shared R/C source mux, but its GPIO allocation and S4 use are replaced by the Phase 5 master architecture. This table remains the Phase 4 simulation record.

The table applies to the DGS 10-pin VSSOP package.

| Pin | Name | Virtual connection |
|---:|---|---|
| 1 | A0 | Black Pill PB12 |
| 2 | S1 | Lower end of 1 kΩ reference; upper end goes to 3.3 V |
| 3 | GND | Ground rail |
| 4 | S3 | Lower end of 100 kΩ reference; upper end goes to 3.3 V |
| 5 | EN | Black Pill PB14 |
| 6 | VDD | 3.3 V rail |
| 7 | S4 | Leave unconnected in Version 1 |
| 8 | D | `SENSE` node |
| 9 | S2 | Lower end of 10 kΩ reference; upper end goes to 3.3 V |
| 10 | A1 | Black Pill PB13 |

Place 100 nF directly between pins 6 and 3.

## Range-control truth table

| PB14/EN | PB13/A1 | PB12/A0 | Selected path |
|---:|---:|---:|---|
| 0 | X | X | All reference paths disconnected |
| 1 | 0 | 0 | S1: 1 kΩ |
| 1 | 0 | 1 | S2: 10 kΩ |
| 1 | 1 | 0 | S3: 100 kΩ |
| 1 | 1 | 1 | S4: unused; firmware must avoid |

During startup, PB14/EN must remain low until the ADC, buffer, and control GPIOs are configured.

## OPA391 and PA0 connection

```text
TMUX pin 8 / SENSE -> OPA391DBV pin 3 (+IN)
OPA391 pin 1 OUT   -> OPA391 pin 4 (-IN)
OPA391 pin 1 OUT   -> 100 Ω -> Black Pill PA0 / ADC1_IN0
OPA391 pin 5       -> 3.3 V
OPA391 pin 2       -> GND
```

The OPA391 prevents PA0 leakage from directly loading high-value resistors.

## Calibrated resistance equation

The complete selected path is used, not only the printed resistor value:

```text
RPATH = measured reference resistor + calibrated switch-path resistance
RX = RPATH × ADC_code / (4095 - ADC_code)
```

The conservative simulated effective paths are:

| Selected resistor | Modeled resistor | Switch resistance | Effective path |
|---:|---:|---:|---:|
| 1 kΩ | 1001 Ω | 9.5 Ω | 1010.5 Ω |
| 10 kΩ | 10010 Ω | 9.5 Ω | 10019.5 Ω |
| 100 kΩ | 100100 Ω | 9.5 Ω | 100109.5 Ω |

These are simulation values. Future physical calibration must replace them with measured constants.

## Autorange sequence

1. Disable the multiplexer by setting `EN=0`.
2. Select S2 using `A1:A0=01`.
3. Set `EN=1`, wait for settling, discard the first ADC conversion, and average 32 readings.
4. Use the 10 kΩ trial code:

```text
trial code below approximately 1000 -> repeat using the 1 kΩ range
trial code above approximately 3100 -> repeat using the 100 kΩ range
otherwise                            -> retain the 10 kΩ range
```

5. Disable `EN` before changing the address.
6. Select the final range, enable it, discard the first reading, and average again.
7. Calculate resistance using the calibrated `RPATH` for that channel.

The ADC thresholds correspond closely to geometric range boundaries around 3.16 kΩ and 31.6 kΩ. Adjacent ranges are both usable near a boundary, so a few ADC codes of noise must not create a fault condition.

## LTspice model

Main file: `Virtual_Breadboard_Resistor_Integration.cir`

The simulation includes:

- TI OPAx391 PSpice macro-model Rev. B;
- 9.5 Ω TMUX1104 worst-case 3.3 V on-resistance allocation;
- ±2 nA switch-path leakage allocation;
- 100 Ω OPA391-to-PA0 resistor;
- 7 pF ADC load;
- ±1 µA MCU leakage at the buffer output;
- ±3.973 mV per reading for ±4 ADC LSB plus conservative buffer-offset allocation;
- 19 resistor and range-boundary values.

## Representative verified results

| Actual RX | Selected range | Nominal ADC code | Nominal estimate | Conservative error interval |
|---:|---:|---:|---:|---:|
| 100 Ω | 1 kΩ | 368.7 | 99.9925 Ω | -1.512% to +1.501% |
| 220 Ω | 1 kΩ | 732.1 | 219.992 Ω | -0.843% to +0.838% |
| 1 kΩ | 1 kΩ | 2036.8 | 999.990 Ω | -0.494% to +0.494% |
| 3 kΩ | 1 kΩ | 3063.2 | 2999.997 Ω | -0.652% to +0.658% |
| 3.162 kΩ boundary | 10 kΩ in sweep | 982.4 | 3162.195 Ω | -0.679% to +0.676% |
| 10 kΩ | 10 kΩ | 2045.5 | 9999.903 Ω | -0.495% to +0.495% |
| 30 kΩ | 10 kΩ | 3069.8 | 29999.975 Ω | -0.657% to +0.663% |
| 31.623 kΩ boundary | 100 kΩ in sweep | 983.0 | 31621.957 Ω | -0.686% to +0.683% |
| 100 kΩ | 100 kΩ | 2046.4 | 99999.028 Ω | -0.506% to +0.506% |
| 470 kΩ | 100 kΩ | 3375.9 | 470001.202 Ω | -0.880% to +0.894% |
| 1 MΩ | 100 kΩ | 3722.4 | 1000013.492 Ω | -1.536% to +1.583% |

All supported points remain inside ±2% in this conservative simulation. The largest errors occur near the low and high ends because the divider output approaches an ADC rail.

## Autorange trial verification

Using the 10 kΩ trial path:

| RX | Trial ADC code | Decision |
|---:|---:|---|
| 3 kΩ | 943.6 | Change to 1 kΩ |
| 3.162 kΩ | 982.4 | Boundary; either adjacent range is usable |
| 3.3 kΩ | 1014.5 | Keep 10 kΩ |
| 30 kΩ | 3069.8 | Keep 10 kΩ |
| 31.623 kΩ | 3109.7 | Boundary; either adjacent range is usable |
| 33 kΩ | 3141.2 | Change to 100 kΩ |

## Fault verification

File: `Virtual_Breadboard_Resistor_Faults.cir`

The TI macro-model did not converge when repeatedly stepped directly into the artificial 10 mΩ rail condition, so rail-level faults were verified separately using the same divider and a unity-gain behavioral buffer. This limitation is documented rather than hidden.

| Fault model | Sense voltage | Modeled ADC voltage | Raw calculated code | Firmware interpretation |
|---|---:|---:|---:|---|
| 10 mΩ short on 1 kΩ range | 32.66 µV | -67.34 µV | -0.084 | Clamp to code 0; short candidate |
| 1 TΩ open on 100 kΩ range | 3.2999997 V | 3.3000997 V | 4095.124 | Clamp to code 4095; open candidate |

The small out-of-rail modeled voltages come from forcing the worst ±1 µA leakage through the idealized 100 Ω output path. A real ADC conversion is bounded to codes 0–4095.

First-pass fault thresholds:

```text
all valid low-range attempts below code 20   -> short or below supported range
all valid high-range attempts above code 4075 -> open, no contact, or above supported range
```

## Firmware calculation pseudocode

```text
trial = read_average(S2_10K)

if trial < 1000:
    range = S1_1K
else if trial > 3100:
    range = S3_100K
else:
    range = S2_10K

code = read_average(range)

if range == S1_1K and code < 20:
    result = SHORT_OR_BELOW_RANGE
else if range == S3_100K and code > 4075:
    result = OPEN_OR_ABOVE_RANGE
else:
    resistance = calibrated_rpath[range] * code / (4095 - code)
```

## Limitations

- A real solderless breadboard can add leakage and contact resistance, especially near 1 MΩ.
- The TMUX1104 is represented by worst-case resistance and leakage rather than its complete transient macro-model.
- The OPA391 macro-model does not replace datasheet worst-case analysis.
- Resistor health still requires a user-entered nominal value and tolerance.
- Physical validation is pending.

Official component references:

- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- OPA391 product/model page: https://www.ti.com/product/OPA391
- STM32F401CC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf

## Phase 4B conclusion

The resistor network is ready for virtual acceptance. Its wiring, control truth table, autorange thresholds, calibrated equation, supported range, and fault rules are defined and numerically verified.

Phase 4C capacitor, Phase 4D diode, and Phase 4E known-pinout BJT integration have now been completed separately. Phase 4 is complete; the next stage is Phase 5 shared analog-front-end consolidation and GPIO/switch-resource audit.
