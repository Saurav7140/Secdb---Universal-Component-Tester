# SECDB Updated Phase Plan

Last updated: 13 September 2026

## Progress summary

| Phase | Name | Status |
|---:|---|---|
| 1 | Project definition and architecture | Complete |
| 2 | Independent measurement-principle verification | Complete |
| 3 | Measurement ranges and non-ideal error budgets | Complete |
| 4 | Virtual breadboard integration | Complete virtually |
| 5 | Shared analog front end and schematic freeze | Complete at schematic level |
| 6 | Firmware architecture and safe bring-up | Complete at source level; target validation pending |
| 7 | KiCad capture and PCB engineering | Next |
| 8 | PCB manufacture and physical bring-up | Not started |
| 9 | Calibration, validation, and diagnostic tuning | Not started |
| 10 | Final integration, demonstration, and submission | In progress where documentation already exists |

Simulation completion does not replace physical validation. Phases 4 and 5 are verified virtually, while PCB parasitics, real ADC performance, thermal behavior, oscillator startup, ESD behavior, and hardware accuracy remain to be tested.

## Phase 1 — Project definition and architecture

Purpose: establish the Version 1 product boundary and core controller.

- Define a tester for disconnected, unpowered electronic components.
- Support resistors, capacitors, diodes, and known-pinout NPN/PNP BJTs.
- Select the STM32F401CCU6 as the controller.
- Use a 3.3 V, 12-bit ADC measurement baseline.
- Keep automatic BJT pin identification and capacitor ESR measurement outside Version 1.
- Develop the component measurement principles independently before combining them.

Status: **Complete**.

## Phase 2 — Independent measurement-principle verification

Purpose: prove that each component can be measured and its major fault signatures distinguished in LTspice.

- Verify resistor measurement with a voltage-divider equation.
- Verify capacitor measurement using the 63.212% RC threshold.
- Verify diode forward and reverse tests in both insertion directions.
- Verify NPN and PNP gain measurement only in the active region.
- Simulate healthy, open, short, and representative fault conditions.

Status: **Complete**.

## Phase 3 — Measurement ranges and non-ideal error budgets

Purpose: replace ideal demonstrations with realistic Version 1 performance targets.

### 3A — Requirements freeze

- Resistance: 100 ohms to 1 megaohm.
- Capacitance: 100 nF to 100 microfarads.
- Diodes: low-voltage silicon, Schottky, and basic LEDs.
- BJTs: small-signal NPN/PNP with known E-B-C order and indicative hFE of about 20–500.
- Define accuracy targets, ADC rules, timer requirements, thresholds, timeouts, and calibration needs.

### 3B — Resistor range and switch study

- Freeze 1 kohm, 10 kohm, and 100 kohm reference ranges.
- Model resistor tolerance, TMUX on-resistance, leakage, ADC error, and high-impedance loading.
- Select TMUX1104 provisionally, with ADG704 retained as a fallback.
- Define autoranging plus open- and short-circuit rules.

### 3C — Capacitor error budget

- Model charge-path tolerance, threshold error, timing error, leakage, and timeout behavior.
- Define mandatory discharge verification and safe timeout wording.

### 3D — Diode error budget

- Model two-polarity testing, ADC error, temperature variation, and resistive leakage.
- Define provisional silicon, Schottky, LED, open, and short classifications.

### 3E — BJT error budget

- Verify the 1 Mohm base and 1 kohm collector paths over hFE about 20–500.
- Model gain error, temperature, loading, and terminal faults.
- Require an ultra-low-input-bias buffer, active-region gating, and a base-off C-E short check.

Status: **Complete**.

## Phase 4 — Virtual breadboard integration

Purpose: integrate each measurement subsystem with the planned STM32 ADC path before physical hardware is available.

### 4A — ADC and buffer bring-up

- Define Black Pill/STM32 power and PA0 ADC connections.
- Integrate the OPA391 unity-gain buffer.
- Verify 1.1 V and 2.2 V calibration channels.
- Freeze a conservative post-switch settling approach.

### 4B — Resistor virtual breadboard

- Integrate three autoranges, mux switching, OPA391 buffering, and PA0 sampling.
- Verify 19 supported and boundary values.
- Demonstrate approximately -1.54% to +1.59% conservative simulated error before Phase 5 protection changes.

### 4C — Capacitor virtual breadboard

- Integrate mutually exclusive charge, discharge, and isolation states.
- Verify 100 nF, 1 microfarad, 10 microfarads, and 100 microfarads.
- Verify open, short, leakage, above-range, and discharge behavior.

### 4D — Diode virtual breadboard

- Integrate protected two-polarity drive and sequential terminal sensing.
- Verify silicon, Schottky, illustrative LED, reversed, open, short, and leakage cases.
- Retain ratiometric blocking and two-polarity short rules.

### 4E — BJT virtual breadboard

- Integrate mirrored NPN/PNP drive paths and E/B/C sensing.
- Verify hFE about 20–500, temperature behavior, and representative terminal faults.
- Retain active-region gating and base-off confirmation.

Status: **Complete virtually**.

## Phase 5 — Shared analog front end and schematic freeze

Purpose: combine all virtual subsystems into one safe, implementation-ready hardware architecture.

### 5A — Architecture, GPIO, and switch audit

- Reduce the design to one TMUX1104 source mux and one TMUX1108 universal sense mux.
- Share the R/C socket and common reference/charging path.
- Assign all sense channels and freeze exclusive mode ownership.
- Audit the STM32F401CCU6 UFQFPN48 GPIO map.
- Correct `SRC_A1` to PA8 because PB11 is unavailable on the selected package.

### 5B — Shared-front-end interaction simulation

- Exercise resistor, capacitor, calibration, diode, BJT, isolation, and rail-fault modes.
- Verify that subsystem performance survives consolidation.
- Freeze a 200 microsecond post-switch delay and first-ADC-sample discard.

### 5C — Protection, power, and preliminary BOM freeze

- Add TPD4E1B06 protection to exposed test terminals.
- Add a calibrated 330 ohm R/C guard and 1 kohm sense isolation resistors.
- Freeze USB-C protection, AP2112 regulation, analog/digital rail distribution, and decoupling.
- Verify approximately -1.92% to +1.90% protected resistor-path error in simulation.
- Produce the preliminary BOM.

### 5D — Master schematic specification and audit

- Freeze the master pin-to-net list and firmware mode truth table.
- Complete the package, pin, ERC, reset-state, and connection audit.
- Freeze sockets, display connector, USB, clock, reset, SWD, protection, and analog-front-end connections.
- Leave AP2112 thermal/droop and crystal startup margin as mandatory first-PCB tests.

Status: **Complete at schematic-specification level**.

## Phase 6 — Firmware architecture and safe bring-up

Purpose: build firmware from the frozen GPIO map and mode truth table without depending on unfinished physical calibration.

### 6A — Firmware foundation and safe AFE driver

- Define the exact STM32F401CCU6 pin map, including `SRC_A1 = PA8`.
- Configure unused pins and all DUT-drive pins to safe analog/no-pull states at startup.
- Add the DWT microsecond timebase and CubeMX integration foundation.

### 6B — ADC acquisition and calibration

- Bind ADC1 and enforce connected-sense acquisition.
- Read and validate the 1.1 V and 2.2 V calibration channels.
- Add trimmed averaging, first-sample discard, timeouts, and correction.
- Keep all DUT modes locked until calibration passes.

### 6C — Resistor and capacitor algorithms

- Implement resistor trial reading, autoranging, calibrated calculation, open, and short detection.
- Implement capacitor discharge verification, timed charge, threshold conversion, and timeout logic.
- Preserve raw diagnostics and force final capacitor discharge handling.

### 6D — Diode and known-pinout BJT algorithms

- Implement two-polarity diode testing and classification.
- Implement NPN/PNP tests with active-region and base-off validation.
- Preserve ambiguous fault wording and orientation guidance.

### 6E — Rule-based diagnostic result model

- Distinguish measured facts from classifications that require a user-entered nominal value and tolerance.
- Normalize severity, confidence, action, value, units, and reference comparisons.
- Provide fixed diagnostic and action text for the user interface.

### 6F — User interface and application integration

- Implement mode selection, ST7735 component-value display, polarity/orientation guidance, and fault messages.
- Add three-button navigation and the complete application state machine.
- Store calibration data with versioning and safe defaults.
- Add USB/serial raw diagnostic output and logging.

Exit criteria:

- Firmware compiles for the exact MCU.
- Every mode transition returns through the safe idle state.
- Host-side or simulated tests cover truth-table states, formulas, boundaries, and timeouts.
- Calibration channels and raw diagnostic output are ready for first-board bring-up.

Status: **Complete at source level; target compilation and physical validation pending.**

## Phase 7 — KiCad capture and PCB engineering

Purpose: convert the frozen Phase 5 specification into manufacturable design files.

- Capture the hierarchical schematic and import the preliminary BOM.
- Assign and verify exact symbols, footprints, exposed pad, and connector pin orders.
- Run ERC and reconcile it with the Phase 5D audit.
- Place the analog cluster compactly and isolate it from USB, TFT, and clock routing.
- Place ESD devices at connectors and provide a continuous ground plane.
- Route the PCB, add test points and required silkscreen safety text, and run DRC.
- Generate fabrication and assembly outputs after an independent design review.

Status: **Not started**.

## Phase 8 — PCB manufacture and physical bring-up

Purpose: safely power and verify the first physical board before testing unknown components.

- Inspect assembly, check resistance between rails, and perform current-limited first power-up.
- Verify 3V3_D, 3V3_A, reset, SWD, USB, and firmware programming.
- Test AP2112 temperature and voltage droop with the real display load.
- Verify 8 MHz crystal startup margin.
- Validate calibration taps, mux reset states, OPA391 behavior, ADC noise, and settling.
- Exercise each test mode first with precision resistors and safe dummy loads.

Status: **Not started**.

## Phase 9 — Calibration, validation, and diagnostic tuning

Purpose: replace simulation assumptions with measured hardware performance.

- Calibrate every reference and complete signal path.
- Test the full supported resistor and capacitor ranges.
- Test representative silicon, Schottky, LED, NPN, and PNP devices.
- Compare results with trusted DMM/LCR/reference equipment.
- Measure repeatability, temperature sensitivity, contact resistance, leakage, and noise.
- Finalize thresholds for healthy, wrong-value, open, short, reversed, leaky, above-range, and unsupported results.
- Record achieved accuracy and any range reduction required by hardware evidence.

Status: **Not started**.

## Phase 10 — Final integration, demonstration, and submission

Purpose: finish the user-facing product and academic deliverables.

- Integrate the PCB, display, buttons, sockets, enclosure, labels, and safety instructions.
- Run end-to-end regression tests for every supported component and fault class.
- Prepare demonstration components and a repeatable presentation sequence.
- Complete the report, equations, simulations, schematics, PCB documentation, firmware flowcharts, results, error analysis, limitations, and future work.
- Prepare the final presentation, poster, and demonstration material.

Status: **Documentation is partially complete; final integration awaits Phases 6–9**.

## Immediate next work

Start Phase 6A and 6B:

1. create the firmware project for STM32F401CCU6;
2. encode the corrected GPIO map and safe reset configuration;
3. implement the centralized safe-transition routine;
4. implement source- and sense-mux helpers from `PHASE_5D_MODE_TRUTH_TABLE.csv`;
5. bring up the two calibration channels before component algorithms.
