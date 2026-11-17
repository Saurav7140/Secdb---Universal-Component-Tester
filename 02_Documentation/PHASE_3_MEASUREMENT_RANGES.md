# SECDB — Phase 3 Measurement Requirements and Range Definition

## Purpose

This document freezes realistic Version 1 targets before the four measurement circuits are combined. The values below are engineering targets for simulation and breadboard validation, not guaranteed final-product specifications.

## MCU measurement baseline

The planned STM32F401CC provides one 12-bit successive-approximation ADC with programmable per-channel sampling time. Its ADC input range is from `VREF-` to `VREF+`; on this device `VREF+` is internally connected to `VDDA` and `VREF-` to `VSSA`. The device documentation lists an internal sample-and-hold capacitance up to approximately 7 pF and sampling selections up to 480 ADC clock cycles.

Design baseline:

| Parameter | Version 1 decision |
|---|---|
| ADC resolution | 12 bit, codes 0–4095 |
| Nominal analog rail | 3.3 V |
| Nominal LSB | approximately 0.806 mV at 3.3 V |
| Firmware sampling | Long sample time initially; optimize only after breadboard tests |
| Noise reduction | Median/outlier rejection plus averaging of 16–64 readings |
| Rail handling | Avoid treating codes close to 0 or 4095 as precision measurements |
| Calibration | Measure known references; use ratiometric equations where possible |

High source resistance requires longer ADC sampling because the internal sampling capacitor must charge through the source impedance. ST's ADC accuracy guidance explicitly recommends increasing sampling time for high-impedance sources.

Official references:

- STM32F401CC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
- STM32F401 reference manual RM0368: https://www.st.com/resource/en/reference_manual/DM00096844.pdf
- ST application note AN2834, ADC accuracy: https://www.st.com/resource/en/application_note/an2834-how-to-get-the-best-adc-accuracy-in-stm32-microcontrollers-stmicroelectronics.pdf

## 1. Resistor range

### Frozen initial target

| Item | Target |
|---|---|
| Supported measurement range | 100 Ω to 1 MΩ |
| Reference resistors | 1 kΩ, 10 kΩ, and 100 kΩ |
| Preferred reference tolerance | 0.1%, low-temperature-coefficient |
| Target accuracy, 100 Ω–100 kΩ | ±2% after calibration |
| Target accuracy, 100 kΩ–1 MΩ | ±5% after calibration |
| User input | Nominal resistance and tolerance when health classification is required |

Equation:

```text
RX = RREF × ADC_code / (4095 - ADC_code)
```

This ratiometric equation is valid when the divider supply and ADC reference are the same rail.

Expected useful endpoints:

| RX | Selected RREF | VADC | Approx. ADC code |
|---:|---:|---:|---:|
| 100 Ω | 1 kΩ | 0.300 V | 372 |
| 1 kΩ | 1 kΩ | 1.650 V | 2048 |
| 10 kΩ | 10 kΩ | 1.650 V | 2048 |
| 100 kΩ | 100 kΩ | 1.650 V | 2048 |
| 1 MΩ | 100 kΩ | 3.000 V | 3723 |

Proposed future range selection:

1. Take a trial reading with 10 kΩ.
2. If the reading is too low, retry with 1 kΩ.
3. If the reading is too high, retry with 100 kΩ.
4. When more than one range is valid, use the result closest to mid-scale.
5. Apply calibration constants for the actual reference path.

The analog switch resistance becomes part of `RREF`. A switch with tens of ohms of on-resistance would produce unacceptable error in the 1 kΩ range unless carefully calibrated. Exact switch selection is therefore postponed until its on-resistance, leakage, voltage range, and channel count are reviewed.

Provisional status rules:

```text
No valid reading and ADC near full scale on every range -> open/no contact
ADC near zero on the 1 kΩ range                  -> short/very low resistance
Measured value within user-entered tolerance     -> healthy/within tolerance
Measured value outside tolerance                 -> wrong value or degraded candidate
```

## 2. Capacitor range

### Frozen initial target

| Item | Target |
|---|---|
| Supported range | 100 nF to 100 µF |
| Charging resistor | 10 kΩ initially |
| Threshold | 63.212% of ADC full scale, approximately code 2589 |
| Timing range | Approximately 1 ms to 1 s |
| Target accuracy, 100 nF–10 µF | ±5% after calibration |
| Target accuracy, 10–100 µF | ±10% after calibration |
| User input | Nominal capacitance and tolerance for health classification |

Equation:

```text
C = T63 / RCHARGE
```

Requirements:

- Discharge the capacitor and confirm a near-zero ADC reading before charging.
- Add a timeout for a short, excessive leakage, or an unsupported large capacitor.
- Detect an unrealistically fast rise as open/no contact or below-range capacitance.
- Require correct polarity for electrolytic capacitors.
- Do not claim ESR measurement from this circuit.
- Do not accept a charged external capacitor into an unprotected MCU input.

The initial product will stop at 100 µF. A 1000 µF capacitor would require approximately 10 seconds with 10 kΩ and should remain future range-expansion work.

## 3. Diode range

### Frozen initial target

| Item | Target |
|---|---|
| Test supply | 3.3 V |
| Current-limiting resistors | 1 kΩ in each terminal leg |
| Maximum modeled short-circuit test current | approximately 1.56 mA |
| Supported devices | Low-voltage silicon, Schottky, and basic LEDs |
| Forward-voltage target resolution | ±20 mV after calibration |
| Required tests | Both polarities |

The first-pass blocking threshold is ratiometric:

```text
VBLOCK = 0.90 × calibrated open-circuit reading
```

Provisional bands for initial firmware:

| Forward reading | Reverse reading | Interpretation |
|---|---|---|
| 0.45–0.90 V | Above `VBLOCK` | Silicon-like unidirectional junction |
| 0.05–0.50 V | Above `VBLOCK` | Schottky-like unidirectional junction |
| 1.2–2.9 V | Above `VBLOCK` | LED-like; color/model dependent |
| Above `VBLOCK` | Above `VBLOCK` | Open, absent, or poor contact |
| Below 0.10 V | Below 0.10 V | Shorted |

The overlap between the type bands is intentional; ambiguous readings must be displayed as type uncertain. A below-0.10 V forward reading is not alone a short because a Schottky diode can have a very low forward drop; both directions must read below 0.10 V. These thresholds are provisional and must be tuned using calibrated hardware samples. LEDs whose forward voltage is close to 3.3 V may not be testable reliably from this supply.

## 4. BJT range

### Frozen initial target

| Item | Target |
|---|---|
| Supported types | Small-signal NPN and PNP |
| Pin handling | User inserts known E-B-C pins correctly |
| Collector resistor | 1 kΩ |
| Base resistor | 1 MΩ calibrated complete path |
| Approximate hFE reporting range | 20–500 |
| hFE accuracy classification | Indicative, target ±10%; not a precision curve-tracer result |
| Automatic pin detection | Excluded from Version 1 |
| Base-node ADC connection | Ultra-low-input-bias buffer mandatory |

For NPN:

```text
IB = (VCC - VBASE) / RB
IC = (VCC - VCOLLECTOR) / RC
hFE = IC / IB
```

For PNP:

```text
IB = VBASE / RB
IC = VCOLLECTOR / RC
hFE = IC / IB
```

Validity requirements:

- Accept `hFE` only when the transistor is in its active region.
- For NPN require `VCE > 0.30 V` and `VCB > 0.20 V`; use mirrored `VEC` and `VBC` rules for PNP.
- Require a plausible `VBE` or `VEB` of 0.45–0.85 V before reporting gain.
- Treat near-zero collector-emitter voltage carefully: it can mean a physical short or tester-induced saturation.
- Confirm a suspected C-E short by removing base drive and checking whether collector current remains.
- If the exact transistor type is unknown, report measured `VBE/VEB`, `hFE`, and functional response instead of asserting degradation from a universal gain threshold.

The 1 MΩ base path carries only about 2.6 µA. Simulation shows that the STM32's worst-case ±1 µA direct input leakage could shift an hFE≈302 result to approximately 189–415. The base node must therefore use the selected OPA391-class ultra-low-bias buffer. With path calibration and a conservative ±3.973 mV error on each measured node, the simulated electronics error is approximately ±7.2% at hFE≈20 and below ±2% above hFE≈100.

## System-level safety limits

- All ADC pins must remain between `VSSA` and `VDDA` during normal operation.
- Add current limiting and input protection before allowing unknown or precharged components to reach MCU pins.
- Clearly mark polarity for electrolytic capacitors, diodes, and BJTs.
- Version 1 tests only unpowered, disconnected components.
- Do not connect components taken directly from a powered circuit.
- Provide a discharge procedure and verify capacitor voltage before measurement.

## Breadboard acceptance criteria

The project may advance to shared analog switching only after the following are demonstrated with a Black Pill prototype:

1. Known resistors from 100 Ω to 1 MΩ can be measured using the three proposed reference values.
2. Calibrated resistor error meets the stated range targets.
3. A silicon diode and Schottky diode show forward conduction and reverse blocking in both insertion directions.
4. 100 nF, 1 µF, 10 µF, and 100 µF timing measurements are repeatable.
5. Open and short capacitor cases are distinguished safely.
6. A known 2N3904/BC547-class NPN and 2N3906-class PNP operate in the active region during gain measurement.
7. Repeated readings are stable enough for the intended diagnostic rules.
8. No ADC input exceeds its supply rails.

## Phase 3 completion

The resistor, capacitor, diode, and BJT nonideal error budgets are complete. The next engineering phase is Phase 4 breadboard validation using the STM32F401 development board, the selected buffer/switch candidates, and real component samples. Shared-front-end consolidation remains after breadboard evidence.
