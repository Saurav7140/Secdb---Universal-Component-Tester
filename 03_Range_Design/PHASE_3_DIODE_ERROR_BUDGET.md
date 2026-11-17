# SECDB — Phase 3C Diode Error Budget and Decision Rules

## Status

Phase 3C is complete at the LTspice design level. A two-polarity, differential-voltage test was simulated with current limiting, worst-case series-path resistance, supply variation, ADC uncertainty, temperature stepping, and representative fault loads.

The resulting rules are suitable for the first breadboard firmware. They are diagnostic bands, not precision diode identification limits. Physical samples must still be used to tune the bands in Phase 4.

## Proposed test connection

```text
GPIO/driver A --- 1 kΩ protection --- terminal A --- DUT --- terminal B --- 1 kΩ protection --- GPIO/driver B
                                         |                         |
                                       ADC_A                     ADC_B
```

Each effective drive leg in the conservative simulation contains:

```text
1 kΩ resistor at +0.1% + 50 Ω GPIO resistance + 9.5 Ω switch resistance
= 1060.5 Ω per leg
```

The two 1 kΩ resistors are intentional. They limit current regardless of the inserted diode orientation and protect both driver directions. With a shorted DUT:

```text
ISHORT = 3.3 V / (2 × 1060.5 Ω + 0.01 Ω)
       = 1.5559 mA
```

The ADC reads both terminal voltages and firmware calculates their difference. Therefore, GPIO output-high and output-low voltage errors mainly change test current; they do not directly become a forward-voltage measurement offset.

## Required polarity sequence

| Test | Driver A | Driver B | Firmware result |
|---|---|---|---|
| A-to-B | High | Low | `VAB = VA - VB` |
| Changeover | High impedance | High impedance | No result; allow switching to settle |
| B-to-A | Low | High | `VBA = VB - VA` |

Both measurements are positive magnitudes. Break-before-make changeover is required when analog switches are used. Exact shared-front-end switch routing remains a Phase 5 decision; the measurement logic itself is now fixed.

## LTspice conditions

File: `Diode_Nonideal_Error_Budget.cir`

- nominal supply: 3.3 V;
- conservative leg resistance: 1060.5 Ω per terminal;
- temperature steps: -10 °C, 25 °C, and 60 °C;
- devices: LTspice 1N4148 and 1N5819 library models plus an illustrative red-LED model;
- supply/path corners: 3.135 V with 1060.5 Ω legs and 3.465 V with 1022.7 Ω legs;
- fault loads: 1 GΩ open, 10 mΩ short, 100 kΩ leakage, and 10 kΩ leakage.

## Verified device results

| Device/model | -10 °C | 25 °C | 60 °C | 25 °C current |
|---|---:|---:|---:|---:|
| 1N4148 | 0.667234 V | 0.599783 V | 0.531519 V | 1.27309 mA |
| 1N5819 | 0.210307 V | 0.141464 V | 0.076834 V | 1.48917 mA |
| Illustrative red LED | 1.990442 V | 2.004696 V | 2.017803 V | 0.610704 mA |

These values belong to the selected SPICE models and current. They are not universal component limits. In particular, the generic LED model is useful only for circuit separation and must not be used as a real LED temperature specification.

The 1N4148 result changes by approximately 136 mV across the simulated -10 °C to 60 °C range. Temperature and part variation therefore dominate over ADC quantization when choosing broad type bands.

## Supply and path-resistance sensitivity

For the 1N4148 model at 25 °C:

| Corner | Forward voltage |
|---|---:|
| 3.135 V, 1060.5 Ω per leg | 0.596951 V |
| 3.3 V, 1060.5 Ω per leg | 0.599783 V |
| 3.465 V, 1022.7 Ω per leg | 0.604090 V |

The complete simulated corner spread is 7.14 mV. The supply and series path alter the diode current and therefore its forward voltage, but the resulting change is still much smaller than the temperature shift in this model.

## ADC error allocation

At 3.3 V, one 12-bit code is approximately:

```text
LSB = 3.3 V / 4095 = 0.8059 mV
```

Using the previously adopted STM32 budget of ±4 LSB per conversion gives ±3.224 mV on each terminal. Two independently measured terminals give a conservative differential limit of ±6.447 mV. Allowing up to ±0.75 mV buffer offset on each channel gives:

```text
worst electronic differential error ≈ ±(6.447 + 1.500) mV
                                    ≈ ±7.95 mV
```

The initial firmware target remains ±20 mV after calibration and averaging. This leaves margin for breadboard noise, rail estimation, switching repeatability, and model-to-hardware differences.

## Verified fault separation

| DUT model | Measured terminal difference | Result against proposed rule |
|---|---:|---|
| 1N4148, reverse at 25 °C | 3.299996 V | Blocking |
| 1 GΩ open | 3.299993 V | Open-like |
| 10 mΩ short | 0.0000155 V | Short |
| 100 kΩ leakage model | 3.231461 V | Blocking/open-like in this coarse test |
| 10 kΩ leakage model | 2.722548 V | Fails blocking test |

This circuit is not a precision reverse-leakage meter. A 100 kΩ equivalent leakage path still looks like a blocking junction, whereas 10 kΩ is detected as abnormal.

## Frozen first-pass firmware rules

Before testing a part, take or calibrate the open-circuit differential reading `VOPEN`. Define:

```text
VBLOCK = 0.90 × VOPEN
VSHORT = 0.10 V
```

At a nominal 3.3 V rail, `VBLOCK` is approximately 2.97 V. The ratiometric definition is preferred over a fixed 3.0 V threshold because it follows the actual test rail.

Apply these rules to the two polarity measurements:

| A-to-B result | B-to-A result | Diagnosis |
|---|---|---|
| Above `VBLOCK` | Above `VBLOCK` | Open, absent part, or poor contact |
| Below `VSHORT` | Below `VSHORT` | Short circuit |
| Conducting | Above `VBLOCK` | Unidirectional junction, anode at A |
| Above `VBLOCK` | Conducting | Unidirectional junction, anode at B |
| Between short and blocking in both directions | Same | Leaky, bidirectional, wrong component, or unknown |

For a confirmed unidirectional junction, use only a tentative type label:

| Measured forward voltage | Display label |
|---:|---|
| 0.05–0.50 V | Schottky-like |
| 0.45–0.90 V | Silicon-like |
| 1.20–2.90 V | LED-like |
| Other conducting value | Diode/junction — type uncertain |

The 0.45–0.50 V overlap is deliberately ambiguous. Firmware should display the measured voltage and use “type uncertain” within overlapping bands or within 20 mV of a boundary. A forward reading below 0.10 V is not by itself proof of a short; the short decision requires both polarity readings below 0.10 V. This preserves detection of very-low-forward-voltage Schottky devices.

LEDs near the 3.3 V test rail, especially some white and blue devices, cannot be separated reliably from an open circuit. Version 1 should report such cases as unsupported/uncertain instead of claiming failure.

## Breadboard acceptance tests for Phase 4

1. Measure at least three samples each of small-signal silicon and Schottky diodes in both insertion directions.
2. Test red, green, and one high-forward-voltage LED and record unsupported cases honestly.
3. Repeat measurements at room temperature and after moderate safe warming/cooling; do not heat parts beyond their ratings.
4. Verify open and short fixtures at least 20 times each without misclassification.
5. Confirm that terminal current stays below 2 mA for a short with the proposed two-resistor connection.
6. Record raw `VA`, `VB`, `VAB`, `VBA`, and the calibrated open reference rather than storing only the label.
7. Add a 20 mV boundary guard band and verify that noisy readings become “uncertain,” not a false healthy/fault result.

## Phase 3C conclusion

Both-polarity testing, two 1 kΩ current-limiting resistors, differential terminal sensing, a ratiometric 90% blocking threshold, and a two-reading short rule are now fixed for the prototype. The broad diode-type labels remain tentative until measured physical-device distributions are collected in Phase 4.

The next Phase 3 task is the BJT gain-range and error budget, including base-path accuracy, active-region validation, ADC loading, temperature sensitivity, and open/short signatures.
