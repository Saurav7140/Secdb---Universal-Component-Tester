# Phase 6E — Rule-Based Diagnostic Engine

## Completion status

Phase 6E is complete at source level. The new diagnostic layer converts the four measurement result structures into one normalized result model for the future display and logger. It keeps measured facts, user-reference comparisons, ambiguous electrical signatures, safety warnings, and tester failures separate.

No target build or hardware validation is claimed. The workspace still has no STM32 ARM compiler, generated CubeIDE project, or physical board.

## Delivered files

- `Phase_6_Firmware/Core/Inc/secdb_diagnostic.h`
- `Phase_6_Firmware/Core/Src/secdb_diagnostic.c`
- `Phase_6_Firmware/Core/Src/phase6e_integration_example.c`
- `outputs/01a0951b-45eb-77d2-8455-d2e3dcc3f765/PHASE_6E_DIAGNOSTIC_TEST_VECTORS.xlsx`

The Phase 6C capacitor implementation was also hardened. An ADC failure during charging now enters the final-discharge routine instead of returning while the charge source may still be connected.

## Normalized result model

Every diagnostic result contains:

- component type;
- stable diagnostic code;
- severity;
- confidence category;
- recommended next action;
- measured value and unit when valid;
- whether a user reference was applied;
- accepted reference limits;
- signed deviation in basis points for resistor and capacitor comparisons.

The diagnostic module performs no GPIO or ADC operations. It is a pure decision layer above `secdb_resistor`, `secdb_capacitor`, `secdb_diode`, and `secdb_bjt`, so the measurement algorithms remain the sole authority for electrical thresholds.

## Severity and confidence

Severity controls presentation priority:

| Severity | Intended meaning |
|---|---|
| Information | Valid fact with no pass/fail reference |
| Pass | Measurement passed the applicable rule |
| Caution | Result needs comparison or further checking |
| Fault | Strong fault evidence or a non-working signature |
| Safety | Stop and discharge or remove external power |
| System error | Calibration, ADC, AFE, or configuration problem |

Confidence prevents the UI from overstating conclusions:

| Confidence | Intended meaning |
|---|---|
| Observed | Directly supported by the electrical signature |
| Conditional | Valid only under an entered reference or broad type rule |
| Candidate | Evidence suggests the result but does not prove root cause |
| Ambiguous | Multiple electrical causes remain possible |
| Unavailable | No component diagnosis is valid because the tester failed |

For example, the base-off BJT rule produces `C-E short candidate`, not `confirmed C-E short`. A resistor near full scale remains `open or above range`, and a capacitor timeout remains `short, leakage, or above range`.

## Nominal-value comparison

Resistor and capacitor health comparison is optional. Without a reference, a valid result is `MEASURED_NO_REFERENCE`; the value remains available but the firmware does not claim healthy or degraded.

The user reference contains a nominal value and tolerance in basis points, where 100 basis points equals 1.00%. References are accepted only when:

- the nominal value is inside the supported measurement range;
- tolerance is from 0% through 50%;
- the reference is explicitly enabled.

The comparison uses exact 64-bit integer cross-multiplication:

```text
|measured - nominal| × 10000 <= nominal × tolerance_basis_points
```

Equality is included. Therefore, 9.500 kΩ and 10.500 kΩ both pass an entered 10.000 kΩ ±5.00% reference, while 9.499 kΩ and 10.501 kΩ do not. An outside result is worded as `wrong value or degraded candidate`; measurement alone cannot distinguish those causes.

## BJT gain reference

A valid active-region BJT result can optionally be compared with a user-entered part-specific hFE range. The entered minimum and maximum must be ordered and remain inside the tester's characterized hFE 20–500 interval.

Without that reference, the result states that the transistor operated in the active region and reports indicative hFE. It does not make a universal health judgement. A gain outside the entered range remains a candidate finding because temperature, collector current, device grade, and manufacturer spread matter.

## Diode result handling

The result model preserves:

- forward drop in millivolts;
- anode A or anode B orientation;
- observed versus type-conditional confidence;
- distinct open-calibration setup results;
- ambiguous open/no-contact and leaky/bidirectional findings.

A broad silicon-like, Schottky-like, or LED-like band does not become a guaranteed device identity.

## Safety and technical failures

Capacitor discharge failures receive safety severity and the action `remove power and discharge safely`. Calibration-required, ADC-failure, AFE-failure, and invalid-configuration results receive system-error severity and never produce a component-health claim.

The text helpers return fixed strings for every diagnostic code, action, and unit. Phase 6F can render those strings directly or replace them with localized strings while retaining the stable enums.

## Verification

The Phase 6E workbook contains 56 rule vectors covering:

- resistor and capacitor tolerance boundaries and the first values outside them;
- disabled, zero, excessive, and invalid references;
- all Phase 6C and Phase 6D measurement statuses;
- diode orientation with known and uncertain type classification;
- BJT gain with no reference, inside range, outside range, and invalid ranges;
- candidate, ambiguous, safety, and system-error mappings.

Source checks verify:

- every upstream measurement status appears in the diagnostic mapping;
- the diagnostic text and action tables match their enum counts;
- all C delimiters and public APIs are present;
- diagnostic code does not access GPIO, ADC HAL, or AFE mode functions;
- tolerance arithmetic uses 64-bit intermediates;
- the charging-loop ADC failure now reaches a safe discharge path.

Fifteen independent numerical assertions verify inclusive tolerance boundaries, displayed limits, signed deviation rounding, BJT range endpoints, and outside-range cases.

The test workbook was recalculated, inspected for its formulas and values, scanned for spreadsheet formula errors, rendered, and visually checked. Its 56-row count formula evaluates correctly and the scan reports no formula errors.

## Acceptance gate and next step

Phase 6E passes at source level. Phase 6F is next: ST7735 display rendering, three-button navigation, the full application state machine, versioned calibration persistence, and USB/serial diagnostic logging. Target compilation and physical validation remain mandatory before Phase 6 is accepted on hardware.
