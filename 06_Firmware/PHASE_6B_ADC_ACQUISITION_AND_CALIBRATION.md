# Phase 6B — ADC Acquisition and Calibration Gate

## Completion status

Phase 6B is complete at source level. The STM32 HAL firmware now has bounded ADC acquisition, mandatory first-conversion discard, trimmed averaging, noise detection, two-point calibration, corrected-code conversion, timeouts, and a calibration interlock that prevents DUT modes from operating before self-test passes.

The code is implementation-ready for a CubeMX-generated STM32F401CCU6 project. ARM compilation and on-target characterization remain mandatory because no STM32 toolchain or physical board is available in the present environment.

The required startup order is `HAL_Init()`, clock configuration, CubeMX GPIO initialization, `MX_ADC1_Init()`, `SECDB_Timebase_Init()`, `SECDB_AFE_Init()`, `SECDB_ADC_Bind(&hadc1)`, and finally `SECDB_ADC_RunCalibration()`. Binding the ADC before the Phase 6A timebase and AFE initialization is not supported.

## Added files

```text
Phase_6_Firmware/Core/
├── Inc/secdb_adc.h
└── Src/
    ├── secdb_adc.c
    └── phase6b_integration_example.c
```

The existing `secdb_afe.h/.c` files were extended with a calibration-owned measurement permit. Reset, initialization, or failed calibration clears this permit and places the front end in its disconnected safe state.

## ADC acquisition policy

The initial configuration is:

| Setting | Initial value |
|---|---:|
| ADC | ADC1 channel 0 on PA0 |
| Resolution | 12 bit, codes 0–4095 |
| Trigger | Software, single conversion |
| Initial ADC clock | APB2/4; 21 MHz when SYSCLK/APB2 are 84 MHz |
| Channel sampling time | 56 ADC cycles |
| Ordinary averaged reading | 32 captured conversions |
| Outlier handling | Remove one minimum and one maximum |
| Returned sample count | 30 |
| First conversion | Always discarded after ordinary mode selection |
| Per-conversion timeout | 5 ms |
| Maximum permitted raw spread | 16 codes peak-to-peak |

The sample count API accepts 4–64 captured conversions. A conversion timeout, HAL error, or excessive spread immediately disables both muxes and tri-states all DUT drives.

`SECDB_ADC_Bind()` rejects a handle that is not ADC1 or is not configured for 12-bit, scan-disabled, continuous-disabled, right-aligned, single-channel conversion. It then explicitly configures ADC1_IN0 with a 56-cycle sampling time. At a 21 MHz ADC clock, one 56-cycle sample plus the 12-cycle successive-approximation interval takes approximately 3.24 µs before software overhead, which remains compatible with the future approximately 10 µs capacitor-threshold polling objective.

The raw single-conversion API is reserved for time-sensitive work such as the Phase 6C capacitor threshold loop. It does not silently average or insert delays.

## Calibration algorithm

The three equal 10 kΩ ladder resistors generate ratiometric targets:

```text
CAL_1V1 ideal code = 4095 / 3 = 1365
CAL_2V2 ideal code = 2 × 4095 / 3 = 2730
ideal span         = 1365 codes
```

Startup calibration performs this sequence:

1. lock all DUT measurement modes;
2. select `CAL_1V1`, wait 200 µs in the AFE driver, discard one conversion, and acquire a trimmed 32-sample reading;
3. repeat for `CAL_2V2`;
4. disconnect the front end;
5. validate both code windows, ordering, span, and peak-to-peak noise;
6. unlock component modes only on a complete pass;
7. finish in the `CAL_1V1` safe connected-idle state.

The accepted first-PCB windows are deliberately gross-fault limits rather than accuracy claims:

```text
CAL_1V1: 1365 ± 64 codes
CAL_2V2: 2730 ± 64 codes
span:    1365 ± 64 codes
spread:  no more than 16 codes peak-to-peak
```

These windows tolerate initial hardware variation while detecting swapped mux channels, broken ladder connections, rail faults, severe noise, and ADC/buffer faults. They must be tightened from measured board data during Phase 7 validation.

## Two-point code correction

For a valid calibration pair `LOWraw` and `HIGHraw`, a new reading is corrected using integer arithmetic:

```text
corrected = 1365 + (raw - LOWraw) × 1365 / (HIGHraw - LOWraw)
```

The implementation uses signed 64-bit intermediate arithmetic, rounded division, and a final clamp to 0–4095. This corrects static ADC-chain offset and gain between the two ladder taps. It does not replace per-range source-resistance calibration or correct contact resistance, leakage, temperature drift, and nonlinearity.

## Failure behavior

| Failure | Firmware response |
|---|---|
| ADC handle missing or not ADC1 | Reject initialization |
| PA0 sense path disconnected | Reject read |
| ADC start/stop error | Disable both muxes and tri-state drives |
| ADC poll timeout | Disable both muxes and tri-state drives |
| Spread over 16 codes | Mark unstable and enter safe state |
| Calibration tap outside window | Keep DUT modes locked |
| Tap order/span invalid | Keep DUT modes locked |
| Corrected read requested before calibration | Enter safe state and reject |
| Direct DUT mode requested before calibration | AFE rejects it with `CALIBRATION_REQUIRED` |

## Verification completed

The source-level audit confirms:

- ADC results are clamped to 12-bit range;
- ordinary acquisition performs exactly one discarded conversion;
- the retained mean excludes one minimum and one maximum;
- sample counts outside 4–64 and zero timeouts are rejected;
- every conversion error path calls the safe-state transition;
- calibration modes remain available while the DUT interlock is locked;
- all resistor, capacitor, diode, and BJT modes require a successful calibration;
- calibration success leaves only the known 1.1 V reference connected;
- the correction calculation uses a nonzero validated span and signed 64-bit intermediates;
- twelve nominal/fault test cases are recorded in `PHASE_6B_ADC_CALIBRATION_TEST_VECTORS.csv`.

The code has not been target-compiled. That limitation is explicit and prevents a source review from being misreported as hardware validation.

## Phase 6B acceptance gate

Phase 6B passes at source level. Phase 6C may now implement resistor and capacitor algorithms using only these interfaces:

- select measurement states through `SECDB_AFE_ApplyMode()`;
- use corrected averaged codes for static resistor readings;
- use raw bounded ADC polling for capacitor threshold timing;
- require `SECDB_ADC_IsCalibrationValid()` before starting either test;
- enter the safe state on completion or every rejection path.

## Official MCU references

- STM32F401xB/xC datasheet, ADC characteristics and timing: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
- STM32F401 reference manual RM0368, ADC operating modes and sampling-time selections: https://www.st.com/resource/en/reference_manual/dm00096844-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
