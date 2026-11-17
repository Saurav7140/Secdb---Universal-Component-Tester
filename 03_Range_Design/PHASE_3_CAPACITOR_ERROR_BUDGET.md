# SECDB Phase 3 — Capacitor Timing and Error Budget

## Final decision of this step

One 10 kΩ charging range is sufficient for the Version 1 target of 100 nF to 100 µF. A second charging range is not justified yet.

The measurement must include:

- a controlled discharge operation before every test;
- a calibrated timing factor;
- ADC threshold uncertainty handling;
- an open/below-range time threshold;
- a five-second maximum charge timeout;
- leakage-aware diagnostic wording.

## Proposed circuit

```text
STM32 charge GPIO
        |
 effective GPIO resistance
        |
 RCHARGE = 10 kΩ, 0.1%
        |
        +------ capacitor measurement node
        |                  |
        |               CX unknown
        |                  |
        +------ ADC buffer/ADC path
                           |
                          GND
```

The final analog-front-end review will decide whether the capacitor node can drive the ADC directly or shares the resistor subsystem's buffer. Unlike the static 1 MΩ resistor test, a real capacitor provides a low dynamic impedance during the conversion, but switch and protection interactions still require testing.

## Model assumptions

- Ideal 3.3 V source with a 1 ns transition.
- 10 kΩ charging resistor at +0.1%: 10.01 kΩ.
- Conservative 50 Ω GPIO output resistance.
- 50 pF of combined socket, PCB, and input parasitic capacitance.
- Nominal ADC threshold code 2589.
- STM32F401 ADC threshold uncertainty represented by ±4 LSB: codes 2585 and 2593.
- Capacitor values: 100 nF, 1 µF, 10 µF, and 100 µF.
- Separate leakage, ESR, open, and short reference cases.

The ±4 LSB value follows the STM32F401 datasheet maximum total unadjusted error at an 18 MHz ADC clock. Source-impedance and sampling-time requirements still have to be respected.

Official source: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf

## Threshold definition

The ideal 63.212% threshold is:

```text
V63 = 3.3 × (1 - exp(-1)) = 2.085998 V
```

Nearest 12-bit ADC code:

```text
Code63 ≈ 2589
```

Modeled threshold voltages:

| ADC condition | Code | Voltage |
|---|---:|---:|
| −4 LSB | 2585 | 2.083150 V |
| Nominal | 2589 | 2.086374 V |
| +4 LSB | 2593 | 2.089597 V |

Firmware equation:

```text
C = T63 / RCHARGE
```

A calibration multiplier should later correct the complete GPIO, resistor, timing, and threshold path.

## Verified ideal-value results with non-ideal test path

| Actual capacitor | Early threshold | Nominal threshold | Late threshold | Firmware result using 10 kΩ | Center error |
|---:|---:|---:|---:|---:|---:|
| 100 nF | 1.004430 ms | 1.007109 ms | 1.009789 ms | 100.711 nF | +0.711% |
| 1 µF | 10.037915 ms | 10.063983 ms | 10.090052 ms | 1.006398 µF | +0.640% |
| 10 µF | 100.362569 ms | 100.634803 ms | 100.907037 ms | 10.063480 µF | +0.635% |
| 100 µF | 1.003642 s | 1.006313 s | 1.008987 s | 100.631296 µF | +0.631% |

The approximately +0.6% common shift is mainly produced by the +0.1% charging resistor and 50 Ω GPIO path. It is repeatable enough to calibrate. The ±4 LSB threshold uncertainty contributes approximately ±0.27%.

## Firmware timing budget

Use a hardware timer with a 1 MHz counter so one timer count equals 1 µs.

| Actual capacitor | Approximate T63 | 1 µs timer quantization |
|---:|---:|---:|
| 100 nF | 1 ms | 0.1% per count |
| 1 µF | 10 ms | 0.01% per count |
| 10 µF | 100 ms | 0.001% per count |
| 100 µF | 1 s | 0.0001% per count |

The ADC should be sampled quickly enough that threshold-detection latency is no more than approximately 10 µs. At the 100 nF endpoint, 10 µs corresponds to 1%. A shorter per-channel ADC sampling setting can be used here than for the high-resistance divider because the capacitor holds the measurement node while the ADC samples it.

Initial firmware target:

```text
Timer tick:             1 µs
ADC threshold:          code 2589
Maximum ADC poll period: 10 µs
Average before charging: verify near-zero start
Maximum charge timeout: 5 s
```

## Leakage results

| 100 µF leakage model | Threshold time | Apparent capacitance | Effect |
|---:|---:|---:|---|
| No added leakage | 1.006313 s | 100.63 µF | Baseline path error |
| 1 MΩ parallel leakage | 1.013664 s | 101.37 µF | Small positive error |
| 100 kΩ parallel leakage | 1.087892 s | 108.79 µF | Significant positive error |
| 15 kΩ parallel leakage | Never reached threshold; 1.9748 V after 5 s | Invalid | Timeout/excessive leakage |

Leakage can make the RC calculation overestimate capacitance, and severe leakage prevents the 63.2% threshold from being reached. The board must therefore avoid claiming that capacitance alone proves capacitor health.

Recommended result wording:

```text
Measured capacitance: 94 µF
Charging response: normal
Leakage screen: passed/failed at this low-voltage test
Status: within entered tolerance / outside entered tolerance
```

Do not claim an accurate ESR value or remaining lifetime.

## ESR result

A deliberately exaggerated 100 Ω series ESR on a 100 µF capacitor produced a threshold time of 1.006266 s, almost the same as the no-ESR result.

Conclusion: this 10 kΩ RC method is intentionally insensitive to realistic ESR values. ESR measurement requires a different, much lower impedance or AC test and remains outside Version 1 unless all core functions are complete.

## Open and short references

| Fault | Simulation result | Provisional interpretation |
|---|---:|---|
| Open/no capacitor | 3.300 V after 10 µs | Immediate rise; open, absent, or below range |
| 10 mΩ short | approximately 3.28 µV after 5 s | Short circuit |

Provisional firmware thresholds:

```text
T63 below 0.25 ms
    -> open, no contact, or capacitance below 100 nF

T63 from approximately 0.5 ms to 2 s
    -> calculate capacitance and classify against supported range

Threshold not reached by 5 s
    -> short, excessive leakage, reversed/incorrect connection, or above range
```

These values must be recalibrated on the breadboard.

## Discharge requirement

Discharging through the same 10 kΩ resistor takes about five seconds for a 100 µF capacitor to fall below approximately 1% of its initial voltage. That is functional but slow.

The later shared front end should include a separately controlled discharge path, provisionally around 1 kΩ. For 100 µF this gives a five-time-constant discharge interval of approximately 0.5 s and an initial current of only 3.3 mA.

Before charging, firmware must confirm that the capacitor voltage is near zero. The board must instruct users to discharge externally charged capacitors safely before insertion.

## Error-budget conclusion

For the 100 nF lower endpoint, a conservative uncalibrated budget is approximately:

| Contribution | Approximate magnitude |
|---|---:|
| 0.1% charging resistor | 0.1% |
| 50 Ω GPIO path | 0.5% |
| ±4 LSB threshold | ±0.27% |
| 1 µs timer quantization | 0.1% |
| ADC polling latency target | up to 1% |
| 50 pF parasitic capacitance | 0.05% |

The modeled and allocated error remains compatible with a ±5% measurement target after calibration. Large electrolytic capacitors retain the wider ±10% project target because leakage and the capacitor's own tolerance dominate.

## Acceptance gate

The capacitor range is accepted for continued Phase 3 work with these conditions:

1. Keep a single 10 kΩ, 0.1% charging resistor for 100 nF–100 µF.
2. Use ADC threshold code 2589 initially.
3. Use a 1 MHz timer and keep ADC threshold polling at 10 µs or faster.
4. Add a controlled discharge path, provisionally 1 kΩ.
5. Apply a five-second timeout.
6. Treat high leakage as a charging-response failure, not an exact ESR measurement.
7. Calibrate timing using known reference capacitors.
8. Confirm all thresholds on breadboard hardware before freezing diagnostics.

## Files generated for this completed step

- `Capacitor_Nonideal_Error_Budget.cir`
- `Capacitor_Nonideal_Error_Budget.log`
- `Capacitor_Nonideal_Error_Budget.raw`
- `PHASE_3_CAPACITOR_ERROR_BUDGET.md`
