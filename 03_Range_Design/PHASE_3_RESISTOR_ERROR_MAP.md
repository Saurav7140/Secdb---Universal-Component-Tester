# SECDB Phase 3 — Complete Resistor Range Error Map

## Final result of this step

The proposed three resistor ranges can cover 100 Ω to 1 MΩ, but the high-resistance range must not drive the STM32 ADC directly if guaranteed accuracy matters. A low-input-bias unity-gain buffer is required between the divider/switch node and the ADC.

Preliminary architecture:

```text
3.3 V
  |
1 kΩ / 10 kΩ / 100 kΩ reference
  |
TMUX1104 selected channel
  |
  +------ measurement node ------ RX ------ GND
  |
OPA391 voltage follower
  |
100 Ω ADC isolation resistor, provisional
  |
STM32 ADC
```

The OPA391 and 100 Ω output resistor are preliminary choices and must be checked in a transient settling simulation and on hardware before the BOM is frozen.

## Autorange selection used in the simulation

| Actual RX | Selected path |
|---:|---:|
| Below 3.162 kΩ | 1 kΩ reference |
| 3.162 kΩ to 31.622 kΩ | 10 kΩ reference |
| Above 31.622 kΩ | 100 kΩ reference |

The boundaries are the geometric means between adjacent reference values. This keeps the ADC reading reasonably far from both rails. Firmware may later test more than one range and choose the result closest to mid-scale.

## Error-map assumptions

- 3.3 V divider supply and ADC reference are the same rail.
- Reference resistors are at the +0.1% tolerance extreme.
- TMUX1104 series resistance is modeled as 9.5 Ω.
- The complete selected path is calibrated in firmware.
- ADC results are quantized to 12 bits.
- The STM32F401 ADC contribution is represented by ±4 LSB, corresponding to the datasheet maximum total unadjusted error at an 18 MHz ADC clock.
- Switch/buffer-node leakage is swept through ±2 nA.
- ADC sampling time is long enough for the input to settle.

The STM32F401 datasheet gives ±4 LSB maximum total unadjusted error at 18 MHz. At 30 MHz it lists ±5 LSB under a source-resistance condition below 10 kΩ, so the slower 18 MHz starting point is preferred for this measurement system.

Official MCU source: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf

## Verified 13-point LTspice result

| RX actual | Range | ADC voltage | Code | Calculated RX | Center error | Modeled error interval |
|---:|---:|---:|---:|---:|---:|---:|
| 100 Ω | 1 kΩ | 0.297 V | 369 | 100.07 Ω | +0.07% | −1.12% to +1.27% |
| 220 Ω | 1 kΩ | 0.590 V | 732 | 219.95 Ω | −0.02% | −0.69% to +0.64% |
| 470 Ω | 1 kΩ | 1.048 V | 1300 | 470.00 Ω | 0.00% | −0.45% to +0.45% |
| 1 kΩ | 1 kΩ | 1.641 V | 2037 | 1000.19 Ω | +0.02% | −0.37% to +0.41% |
| 2.2 kΩ | 1 kΩ | 2.263 V | 2806 | 2.1997 kΩ | −0.01% | −0.46% to +0.44% |
| 4.7 kΩ | 10 kΩ | 1.054 V | 1308 | 4.7024 kΩ | +0.05% | −0.40% to +0.50% |
| 10 kΩ | 10 kΩ | 1.649 V | 2046 | 10.0048 kΩ | +0.05% | −0.44% to +0.44% |
| 22 kΩ | 10 kΩ | 2.268 V | 2814 | 22.0101 kΩ | +0.05% | −0.41% to +0.50% |
| 47 kΩ | 100 kΩ | 1.054 V | 1308 | 46.9836 kΩ | −0.04% | −0.48% to +0.41% |
| 100 kΩ | 100 kΩ | 1.649 V | 2046 | 99.9629 kΩ | −0.04% | −0.43% to +0.45% |
| 220 kΩ | 100 kΩ | 2.268 V | 2814 | 219.913 kΩ | −0.04% | −0.49% to +0.53% |
| 470 kΩ | 100 kΩ | 2.721 V | 3376 | 470.055 kΩ | +0.01% | −0.66% to +0.69% |
| 1 MΩ | 100 kΩ | 2.999 V | 3722 | 998.948 kΩ | −0.10% | −1.27% to +1.39% |

Within these model assumptions, every point remains inside the provisional ±2% target.

## Why the ADC buffer is required

The STM32F401 datasheet specifies I/O input leakage as high as ±1 µA. At the 1 MΩ endpoint, the divider current is only about 3 µA, so this limit cannot be ignored.

Direct unbuffered simulation at 1 MΩ:

| ADC leakage direction | Calculated resistance | Error |
|---|---:|---:|
| +1 µA worst direction | 1.4783 MΩ | +47.8% |
| −1 µA worst direction | 744.16 kΩ | −25.6% |

Preliminary OPA391-buffered model:

| Condition included | Calculated resistance | Error |
|---|---:|---:|
| Positive offset/bias/output-loading direction | 1.00279 MΩ | +0.279% |
| Negative offset/bias/output-loading direction | 997.22 kΩ | −0.278% |

The model uses a conservative ±750 µV input offset near the high common-mode region, ±5 pA input bias for operation up to 85°C, 10 Ω output resistance, and the full ±1 µA MCU leakage applied to the buffer output. The buffer supplies the ADC leakage, so it no longer flows through the high-resistance divider.

The OPA391 operates from 1.7–5.5 V, has rail-to-rail input/output, and is offered in SOT-23-5. TI specifies input bias up to 0.8 pA at 25°C and 5 pA from 0°C to 85°C.

Official op-amp source: https://www.ti.com/lit/ds/symlink/opa391.pdf

## Calibration requirement

Store one effective resistance for each complete selected path:

```text
RPATH_1K_CAL
RPATH_10K_CAL
RPATH_100K_CAL
```

Each value includes the physical reference resistor, multiplexer on-resistance, copper resistance, and repeatable connector resistance. Firmware then uses:

```text
RX = RPATH_CAL × ADC_code / (4095 - ADC_code)
```

Calibration cannot remove random contact resistance, temperature-dependent leakage, contamination, or all switch on-resistance flatness. Those effects must remain in the hardware error budget.

## Decision and acceptance gate

This resistor-range design is accepted for continued Phase 3 work with these conditions:

1. Use 1 kΩ, 10 kΩ, and 100 kΩ, preferably 0.1%, reference resistors.
2. Continue with TMUX1104 as the preliminary range selector.
3. Add an OPA391-class low-bias RRIO voltage follower before the STM32 ADC.
4. Begin ADC operation at 18 MHz or lower with a long sampling time.
5. Calibrate all three complete reference paths.
6. Verify op-amp stability and settling with the final output isolation resistor and ADC load.
7. Confirm the ±2% result on a breadboard; the simulation is not a hardware guarantee.

## Files generated for this completed step

- `Resistor_Full_Range_Error_Map.cir`, `.log`, and `.raw`
- `Resistor_ADC_Buffer_Risk.cir`, `.log`, and `.raw`
- `PHASE_3_RESISTOR_ERROR_MAP.md`
