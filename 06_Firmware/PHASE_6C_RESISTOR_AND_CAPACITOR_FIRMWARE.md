# Phase 6C — Resistor Autoranging and Capacitor Timing Firmware

## Completion status

Phase 6C is complete at source level. The firmware now contains bounded resistor autoranging and capacitor timing modules built on the Phase 6A safe-state driver and Phase 6B calibrated ADC layer.

The code has not been ARM-compiled or exercised on a physical target because no STM32CubeIDE toolchain or board is available in the current environment. All constants derived from LTspice are labelled as initial values and must be replaced or confirmed during physical calibration.

## Added source files

```text
Phase_6_Firmware/Core/
├── Inc/
│   ├── secdb_resistor.h
│   └── secdb_capacitor.h
└── Src/
    ├── secdb_resistor.c
    ├── secdb_capacitor.c
    └── phase6c_integration_example.c
```

`secdb_adc.h/.c` now also provides the inverse calibrated-code transformation needed to convert ideal capacitor thresholds into fast raw ADC thresholds. `secdb_timebase.c` was corrected to accumulate elapsed cycles across DWT counter wrap instead of allowing its microsecond output to jump backward.

## Resistor implementation

### Default path constants

The initial complete paths include the +0.1% modeled range resistor, 9.5 Ω U_SRC resistance, and 330 Ω socket guard:

| Range | Initial effective path |
|---|---:|
| 1 kΩ | 1340.5 Ω |
| 10 kΩ | 10349.5 Ω |
| 100 kΩ | 100439.5 Ω |

They are stored in milliohms so the measurement uses deterministic integer arithmetic. Phase 7 must replace them with physical per-board calibration values. Plausibility limits reject zero, swapped, or grossly incorrect calibration entries.

### Measurement sequence

1. Require a valid Phase 6B ADC calibration.
2. Select the 10 kΩ trial path through the safe AFE API.
3. Acquire a corrected, first-sample-discarded, trimmed 32-reading average.
4. Select 1 kΩ below code 1000, 100 kΩ above code 3100, or retain 10 kΩ.
5. Perform a fresh averaged conversion on the selected final range.
6. Reject codes below 20 as short/below measurable and above 4075 as open/above measurable.
7. Calculate resistance with the selected complete path:

```text
RX = RPATH × corrected_code / (4095 - corrected_code)
```

8. Mark valid calculated values below 100 Ω or above 1 MΩ as outside the supported range rather than silently claiming in-range accuracy.
9. Disconnect the AFE on every return path.

The result preserves the selected range, trial code, final code, resistance, ADC status, and AFE status for later display and logging.

## Capacitor implementation

### Default timing constant

The Phase 5C protected charge path is:

```text
RCHARGE_PATH = 10.010 kΩ + 9.5 Ω + 330 Ω = 10.3495 kΩ
Kthreshold   = -ln(1 - 2589/4095) = 1.000309588
RCHARGE_PATH × Kthreshold = 10.352704 kΩ
```

The stored timing denominator is therefore `10352704 milliohms`. Capacitance is calculated without floating point:

```text
capacitance_nF = elapsed_us × 1,000,000 / denominator_milliohm
```

### Threshold translation

The ideal discharge and charge thresholds are corrected codes 41 and 2589. Before timing, Phase 6C translates both into raw ADC codes using the inverse of the Phase 6B two-point calibration. For example, a common +10-code ADC offset produces raw thresholds 51 and 2599. This avoids a signed 64-bit division inside every fast polling iteration.

### Measurement sequence

1. Require valid ADC calibration and a plausible timing denominator.
2. Select the protected discharge path and discard the first ADC conversion.
3. Poll until the calibrated discharge threshold is reached or 5 seconds elapse.
4. Enter isolated `CAP_READY`, wait through the Phase 6A settling period, discard once, and confirm the node remains discharged.
5. Enable the 10 kΩ charge source using the guarded `SECDB_AFE_StartCapCharge()` call and capture the start timestamp at that event.
6. Poll raw ADC codes until the translated 63.212% threshold is reached or the 5-second timeout expires. No post-start 200 µs delay is inserted.
7. Classify a crossing below 250 µs or a calculated value below 100 nF as open/below range.
8. Classify a calculated value above 100 µF as above the supported range.
9. Report a non-crossing only as `TIMEOUT_SHORT_LEAKAGE_OR_ABOVE_RANGE`; the circuit cannot uniquely separate those causes.
10. Reapply the discharge path after every completed charge attempt and confirm code ≤41 before returning.
11. Disconnect the AFE after final discharge.

If final discharge cannot be confirmed, the return status is `FINAL_DISCHARGE_FAILED` while `measurement_status` preserves the original measurement outcome. The UI must warn the user not to remove the capacitor until it is externally discharged safely.

## Timing correction

At 84 MHz the DWT hardware cycle counter wraps in approximately 51 seconds. The earlier direct `CYCCNT / cycles_per_us` implementation could therefore jump backward. Phase 6C changes the timebase to accumulate unsigned cycle deltas and fractional-cycle remainder inside a brief critical section.

The accumulated microsecond counter itself may wrap, but all Phase 6C timeout checks use unsigned subtraction and are only five seconds long. The time function is called continuously during a measurement, so no DWT wrap can be missed.

## Result taxonomy

The algorithms deliberately separate measurement from health diagnosis:

- resistor: valid, short/below measurable, open/above measurable, below supported range, or above supported range;
- capacitor: valid, open/below 100 nF, above 100 µF, combined charge timeout, initial discharge failure/external voltage, or final discharge failure;
- ADC and AFE failures remain technical faults rather than component-health claims.

User-entered nominal values and tolerance-based Healthy/Degraded decisions remain Phase 6E work.

## Verification completed

The source and independent numerical vectors verify:

- the 10 kΩ trial thresholds choose the intended range at representative boundaries;
- all three selected-path equations reproduce 100 Ω through 1 MΩ within ADC-code quantization;
- 50 Ω and 1.5 MΩ remain measurable examples but are labelled outside the supported range;
- rail codes are rejected before the divider denominator can become zero;
- the Phase 5C charge times reproduce 100 nF, 1 µF, 10 µF, and 100 µF;
- below-range, above-range, timeout, initial-discharge, final-discharge, ADC-error, and calibrated-offset cases are represented;
- every ordinary exit disconnects the AFE;
- a successfully completed capacitor attempt confirms final discharge;
- 26 combined resistor/capacitor test vectors are recorded in `PHASE_6C_MEASUREMENT_TEST_VECTORS.csv`.

## Phase 6C acceptance gate

Phase 6C passes at source level. Phase 6D may implement diode and known-pinout BJT algorithms, but must continue to use the safe AFE modes, corrected ADC readings, two-polarity diode rules, and BJT active-region gates already frozen by the earlier phases.

## Source references

- `PHASE_4B_VIRTUAL_RESISTOR_BREADBOARD.md`
- `PHASE_4C_VIRTUAL_CAPACITOR_BREADBOARD.md`
- `PHASE_5C_PROTECTION_POWER_AND_BOM_FREEZE.md`
- `PHASE_6B_ADC_ACQUISITION_AND_CALIBRATION.md`
