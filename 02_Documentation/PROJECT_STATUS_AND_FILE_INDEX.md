# Smart Electronic Component Diagnostic Board (SECDB)

## Current project status

The four independent LTspice measurement principles have been completed and verified:

| Module | Status | Main result |
|---|---|---|
| Resistor | Complete | 10 kΩ voltage-divider equation verified from 220 Ω to 1 MΩ |
| Diode | Complete | Forward, reverse, open, and short signatures verified |
| Capacitor | Complete | 10 kΩ RC timing verified from 100 nF to 100 µF |
| BJT | Complete | Active-region NPN/PNP gain tests and fault signatures verified |

These results are ideal simulations. They establish the measurement principles but do not yet include all ADC errors, tolerances, protection components, analog switches, or PCB parasitics.

## Folder structure

```text
ece final year project/
├── 01_LTspice_Verified/
│   ├── Resistor/
│   ├── Diode/
│   ├── Capacitor/
│   └── BJT/
├── 02_Documentation/
├── 03_Range_Design/
└── 04_Virtual_Breadboard/
```

Each LTspice folder contains the relevant graphical schematic or text deck plus generated LOG and RAW simulation results. The documentation folder contains the engineering report for each module.

## Important design decisions already established

- Keep resistor, diode, capacitor, and BJT circuits independent until their measurement ranges are frozen.
- A single 10 kΩ resistor range cannot measure 220 Ω through 1 MΩ with uniform accuracy.
- Diode open/healthy diagnosis requires tests in both polarities.
- Capacitor measurement uses the 63.212% RC threshold, not a constant-current equation.
- A BJT gain measurement is valid only in the active region.
- The original BJT circuit with a 10 kΩ base resistor saturates the transistor; the corrected gain test uses 1 MΩ in simulation.
- Automatic BJT pin identification is not part of Version 1.

## Current phase

The project is now in **Measurement Requirements and Range Definition**. The following Phase 3 studies are complete:

1. resistor autorange switch model and full-range ADC/leakage error map;
2. capacitor timing, threshold, leakage, and timeout error budget;
3. diode two-polarity architecture, temperature sweep, ADC error allocation, and fault thresholds.

The BJT gain-range and loading study is also complete:

- calibrated 1 MΩ base and 1 kΩ collector paths support indicative hFE≈20–500;
- worst modeled electronics error is approximately ±7.2% at the low end;
- the BJT base measurement requires an ultra-low-input-bias ADC buffer;
- active-region and base-off confirmation rules prevent saturated ratios from being mislabeled as hFE.

All four Phase 3 component error budgets are now complete. The frozen design set defines:

1. supported range for each component type;
2. target accuracy or classification resolution;
3. required ADC and timer resolution;
4. reference-resistor values;
5. valid measurement thresholds and timeouts;
6. which results require a user-entered nominal value;
7. acceptance criteria for moving to breadboard hardware.

The project has advanced to **Phase 4 — virtual breadboard and STM32 validation planning** because physical breadboard access is not currently available.

Phase 4A is complete virtually:

- Black Pill power and PA0 ADC connections are defined;
- the OPA391DBV unity-gain buffer wiring is defined;
- a 1.1 V/2.2 V calibration ladder is defined;
- nominal ADC codes 1365 and 2730 were verified in LTspice;
- conservative voltage windows including buffer offset, output loading, and ±4 LSB ADC error were documented.
- TI's official OPAx391 PSpice macro-model was successfully run in LTspice, producing less than 0.12 mV modeled static error and approximately 0.381 µs settling to within 1 mV for a 1.1-to-2.2 V step.

Phase 4B virtual resistor integration is also complete:

- TMUX1104DGS pin-level wiring and GPIO truth table are defined;
- 1 kΩ, 10 kΩ, and 100 kΩ calibrated reference paths are integrated with the OPA391 and PA0;
- 19 supported and boundary values were simulated using TI's OPAx391 macro-model;
- the conservative error remains between approximately -1.54% and +1.59% from 100 Ω to 1 MΩ;
- 10 kΩ trial-code autoranging and open/short thresholds are documented.

Phase 4C virtual capacitor integration is complete:

- a second TMUX1104 provides mutually exclusive charge, discharge, and isolation states;
- the 10.0195 kΩ modeled charge path and exact code-2589 threshold factor are included in the calibrated equation;
- 100 nF, 1 µF, 10 µF, and 100 µF were verified through TI's OPA391 macro-model;
- the separate 1.0105 kΩ discharge path brings 100 µF below 1% in approximately 0.465 s;
- open, short, leakage, and above-range signatures were verified with non-unique timeout wording.

Phase 4D virtual diode integration is complete:

- PA8 and PA9 provide protected two-polarity drive through separate 1 kΩ resistors;
- a third TMUX1104 selects terminal A or B for sequential reading through the shared OPA391/PA0 path;
- silicon, Schottky, and illustrative LED forward signatures were verified from -10 °C to 60 °C;
- reversed healthy, open, short, and resistive-leakage cases were verified;
- a ratiometric 90% blocking threshold and two-polarity short rule remain mandatory.

Phase 4E virtual known-pinout BJT integration is complete:

- PA1, PA2, and PA3 provide mirrored NPN/PNP emitter, base, and collector drive paths;
- a fourth sense TMUX selects E, B, or C for the shared OPA391/PA0 ADC path;
- matched-model hFE≈20–500 sweeps remain active and within the ±10% indicative electronics target;
- 2N3904/2N3906 temperature behavior and representative terminal faults were verified;
- active-region gating and base-off C-E-short confirmation are mandatory.

Phase 4 virtual breadboard integration is now complete. These results remain simulation-verified, not hardware-validated.

Phase 5 has started. Step 5A architecture/GPIO/switch audit is complete:

- the four provisional TMUX1104 devices are reduced to one TMUX1104 source mux and one TMUX1108 universal 8:1 sense mux;
- resistor and capacitor modes share one R/C socket, the 10 kΩ charging/reference path, and a four-state source mux;
- all eight sense-mux channels are assigned to R/C, diode A/B, BJT E/B/C, and the 1.1/2.2 V calibration taps;
- the PA9 diode-drive conflict was removed because PA9 is reserved for USB VBUS sensing; diode drive is reassigned to PA6/PA10;
- the complete STM32F401CCU6 GPIO map, safe reset state, and exclusive mode-ownership sequence are documented;
- the exact final MCU package is recognized as UFQFPN48 with an underside ground pad.

Phase 5B consolidated analog-front-end interaction simulation is complete:

- four focused LTspice decks exercise resistor, capacitor, calibration, diode, BJT, isolation, rail faults, and high-impedance settling through the Phase 5A architecture;
- resistor error remains between approximately -1.536% and +1.583% across 100 Ω–1 MΩ;
- the 100 pF universal-sense capacitance adds only approximately +0.052% at the 100 nF capacitor endpoint;
- calibration, diode and active-region BJT results remain consistent with Phase 4;
- worst modeled static high-impedance recovery is below 73 µs, so a conservative 200 µs post-switch delay is frozen for initial firmware;
- the capacitor sequence keeps the sense mux selected before the charge timer begins.

The next task is Phase 5C input protection, power/decoupling, calibration-component, and preliminary BOM freeze. Physical validation remains mandatory before the design is treated as final hardware.
