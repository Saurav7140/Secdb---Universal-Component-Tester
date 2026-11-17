# Phase 6D — Diode and Known-Pinout BJT Firmware

## Completion status

Phase 6D is complete at source level. The firmware now measures an unpowered diode in both directions and evaluates a user-selected NPN or PNP transistor inserted with a known E/B/C pin order. It reports only conclusions supported by the measured signatures; ambiguous cases retain explicit uncertain or wrong-connection results.

This is not a claim of target compilation or hardware accuracy. The current workspace has no ARM GCC/CubeIDE toolchain and no physical board. Phase 7 must replace modeled path values with measured values and validate all thresholds on hardware.

## Delivered files

- `Phase_6_Firmware/Core/Inc/secdb_diode.h`
- `Phase_6_Firmware/Core/Src/secdb_diode.c`
- `Phase_6_Firmware/Core/Inc/secdb_bjt.h`
- `Phase_6_Firmware/Core/Src/secdb_bjt.c`
- `Phase_6_Firmware/Core/Src/phase6d_integration_example.c`
- `PHASE_6D_DIODE_BJT_TEST_VECTORS.csv`

`secdb_afe.h` and `secdb_afe.c` were also extended with NPN and PNP base-off emitter-read modes. The diode drive helper was corrected to configure the low-side terminal before enabling the high-side terminal in either polarity.

## Mandatory preconditions

1. Test only unpowered and discharged parts.
2. Complete the Phase 6B ADC calibration successfully.
3. For a diode, remove the part and run the empty-socket open calibration after power-up or after the analog path changes.
4. For a BJT, the user must select NPN or PNP and insert the leads in the declared emitter/base/collector order. Phase 6D does not discover an unknown pinout.
5. Replace the default voltage and resistance constants with measured-board calibration in Phase 7.

Every success and failure path returns the AFE to the disabled safe-transition state.

## Diode algorithm

The final drive pins are PA6 for terminal A and PA10 for terminal B. Four corrected ADC readings are taken through U_SENSE and the OPA391:

1. A high and B low; read terminal A.
2. A high and B low; read terminal B.
3. A low and B high; read terminal A.
4. A low and B high; read terminal B.

The voltage-difference codes are:

```text
diff_AB = |A_AB - B_AB|
diff_BA = |B_BA - A_BA|
```

With the socket empty, each open difference must be at least 3800 codes. The stored blocking threshold for each direction is 90% of its own open-socket difference, rounded to the nearest code. At an ideal 4095-code open reading this is 3686 codes.

The decision table is:

| AB result | BA result | Report |
|---|---|---|
| blocking | blocking | open or no contact |
| at most 124 codes | at most 124 codes | short |
| conducting | blocking | diode with anode at A |
| blocking | conducting | diode with anode at B |
| conducting | conducting | leaky, bidirectional, or unknown |

For a one-way device, the conducting-direction difference is converted to millivolts using the measured ADC full-scale voltage. The descriptive type bands are:

- Schottky-like: 0.05–0.50 V;
- silicon-like: 0.45–0.90 V;
- LED-like: 1.2–2.9 V.

A ±20 mV guard is applied around every band boundary, and the 0.45–0.50 V overlap is always reported as uncertain. These are descriptive signatures, not definitive semiconductor identification.

## BJT algorithm

The default Phase 5C protected-path model is:

| Quantity | Default |
|---|---:|
| low drive | 0 mV |
| high drive | 3300 mV |
| ADC full scale | 3300 mV |
| base path | 1,001,050 Ω |
| collector path | 1,051 Ω |

The ADC full-scale voltage is intentionally separate from the GPIO high-drive voltage. On a real board those values can differ and must be measured separately.

For NPN, emitter is driven low while base and collector are driven high through their protected paths. For PNP, the arrangement is mirrored. E, B, and C are sensed sequentially.

NPN calculations:

```text
VBE = VB - VE
VCE = VC - VE
VCB = VC - VB
IB  = (VHIGH - VB) / RBASE
IC  = (VHIGH - VC) / RCOLLECTOR
```

PNP calculations:

```text
VEB = VE - VB
VEC = VE - VC
VBC = VB - VC
IB  = (VB - VLOW) / RBASE
IC  = (VC - VLOW) / RCOLLECTOR
```

The indicative gain is calculated only after the active-region gates pass:

```text
hFE = IC / IB
```

The gates require:

- forward base-emitter junction from 450 to 850 mV;
- collector-emitter voltage greater than 300 mV;
- collector-base reverse margin greater than 200 mV;
- at least 40 mV collector-path drive drop;
- at least 1000 mV base-path drive margin;
- indicative hFE of at least 20.

An active-region result from hFE 20 through 500 is returned as `ACTIVE_INDICATIVE_HFE`. A value above 500 receives `ACTIVE_GAIN_ABOVE_CHARACTERIZED_RANGE`; it is not silently accepted as a calibrated gain. A value below 20 or a weak drive margin receives `LOW_GAIN_OR_INADEQUATE_MARGIN`.

No universal degraded/pass judgement is made from hFE alone. Device type, operating current, temperature, and manufacturer spread make such a judgement invalid without a part-specific reference.

## Base-off C-E short confirmation

If the initial collector-emitter voltage is 300 mV or lower, the firmware removes the base drive and measures emitter and collector again. A C-E short candidate requires both:

- base-off collector-emitter voltage at most 100 mV; and
- base-off collector current at least 500 µA.

If both conditions do not persist, the result is `SATURATION_OR_INVALID_ACTIVE_REGION`, not a confirmed short. This avoids mislabelling a saturated transistor as a C-E short.

## Failure and ambiguity handling

- ADC calibration missing: measurement is rejected before a DUT mode is entered.
- Invalid or corrupt calibration values: invalid argument.
- AFE mode error or ADC timeout/error: a distinct failure is retained.
- Very low base-emitter voltage: B-E short or wrong connection.
- Excessive junction voltage: open base, wrong pin, or wrong polarity.
- Low collector response: open collector or no response.
- Low collector-base margin: B-C short, saturation, or wrong connection.
- Non-forward junction below 450 mV: junction out of range.

The wording deliberately groups electrically non-unique faults.

## Verification performed

The 30 cases in `PHASE_6D_DIODE_BJT_TEST_VECTORS.csv` cover:

- valid and failed diode open calibration;
- both diode orientations;
- silicon-like, Schottky-like, LED-like, boundary-uncertain, open, short, and bidirectional/leaky signatures;
- healthy NPN and PNP examples matching the Phase 5C operating points;
- the hFE 20 and 500 characterized-range edges;
- open base, open collector, B-E short, B-C ambiguity, low junction, low gain, saturation, and NPN/PNP C-E-short candidates;
- missing calibration, invalid selection, AFE failure, and ADC failure paths.

Static source checks confirm that all new AFE modes have switch cases, no stale PB11 assignment was introduced, all exits invoke the safe transition, and the base-off current conversion is in microamps. Numerical reference checks use the same rounded integer equations as the C implementation.

## Acceptance gate

Phase 6D passes at source level. Phase 6E may now combine the Phase 6C and Phase 6D results into a rule-based diagnostic engine. Target compilation, oscilloscope verification of switching order, calibration with measured board values, and component testing remain required in Phase 7.
