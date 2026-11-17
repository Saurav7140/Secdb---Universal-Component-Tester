# SECDB Phase 7A — Design Validation Plan

Date: 2026-09-13  
Applies to: schematic capture, PCB layout, fabrication outputs, and first-board bring-up.

## Gate 0 — Frozen-source reconciliation (completed for Phase 7A)

- Reconcile all 151 rows of `PHASE_5D_MASTER_NETLIST.csv` with the schematic specification.
- Preserve all 23 hardware states in `PHASE_5D_MODE_TRUTH_TABLE.csv`.
- Carry forward all 50 items in `PHASE_5D_ERC_AND_PIN_AUDIT.csv`.
- Compare every MCU function against Phase 6 `Core/Inc/secdb_board.h`.
- Mandatory pin lock: `SRC_A1 = PA8`; search all KiCad outputs for `PB11` and fail if it is assigned.

## Gate 1 — Schematic capture review (future Phase 7B)

1. Capture one hierarchy sheet at a time, beginning with power, then MCU, AFE, and display.
2. After each sheet, compare every pin and passive connection against the corresponding master-netlist rows.
3. Verify manufacturer pinouts and exact packages for STM32F401CCU6, TMUX1104DGSR, TMUX1108PWR, OPA391DBVR, AP2112, TPD4E1B06DCKR, USBLC6-2SC6, USB4105-GF-A, and ABLS-8.000MHZ-B2-T.
4. Verify reset defaults: seven 100 kΩ pull-downs on mux enables/addresses and analog/no-pull DUT drives.
5. Verify `ADC_BUFFERED` only reaches PA0 and is never driven digitally.
6. Run ERC with no unexplained errors. Every suppression requires a written reason in the report.
7. Produce a schematic PDF and a pin/net cross-check report.

## Gate 2 — Footprint and BOM audit (future Phase 7C)

- Match each footprint to the frozen manufacturer package and datasheet land-pattern dimensions.
- Lock U1 to UFQFPN48 with exposed pad connected to GND.
- Check socket/header pin numbering from the component side and board side.
- Check polarity/orientation for TVS, Schottky diode, LDO, ESD arrays, USB-C receptacle, LEDs, and capacitors.
- Reconcile BOM values, tolerance, dielectric, voltage rating, MPN, manufacturer, lifecycle, and assembly option.
- Do not substitute analog/protection parts without a documented electrical review.

## Gate 3 — PCB placement and routing review (future Phase 7D)

- Keep OPA391, U_SENSE, ADC filter, precision references, and the PA0 side of U1 in a compact analog cluster.
- Place TPD4E1B06 devices at the socket edge with short ESD return vias.
- Place USBLC6-2SC6 at the USB connector; route D+/D− through it without stubs.
- Keep USB and TFT return currents away from the analog cluster.
- Use one unbroken ground plane; do not split analog and digital grounds.
- Keep the HSE loop short, symmetric, and preferably via-free.
- Give each MCU VDD pin its own nearby 100 nF capacitor.
- Use a solid U1 exposed-pad ground area and thermal-via array.
- Place the required safety text and socket pin labels on silkscreen.
- Run DRC, inspect unconnected items, and review the 3D model/orientation of every connector.

## Gate 4 — Fabrication-output review (future Phase 7E)

- Generate Gerbers, drill files, position files, BOM, schematic PDF, board PDF, and fabrication notes from a clean tagged revision.
- Open every Gerber layer in an independent viewer and check outline, solder mask, paste, silkscreen, copper, drills, and NPTH/PTH classification.
- Confirm no plot contains stale data and archive the exact DRC/ERC reports with the outputs.

## Gate 5 — First-board electrical validation (future hardware phase)

- Bring up with no DUT inserted and current-limited USB power.
- Verify shorts, 3V3_D, 3V3_A, AP2112 temperature, and rail droop; maintain the initial 150 mA load ceiling until measured.
- Verify HSE startup margin with the fitted 30 pF capacitors and 0 Ω tuning link.
- Confirm both muxes are disabled during reset and all DUT drives are high impedance.
- Check calibration-channel monotonicity, ADC offset/gain, 200 µs settling, and first-sample discard.
- Validate resistor, capacitor, diode, and known-pin BJT modes using the Phase 6 test vectors.
- Perform ESD and misuse testing only within an approved lab plan. The board is not protected for powered circuits, batteries, charged capacitors, mains-referenced parts, or in-circuit measurements.

## Acceptance rule

No gate advances on an undocumented mismatch. Differences from Phase 5D or `secdb_board.h` must be recorded, justified, reviewed, and explicitly approved before editing the electrical design.

