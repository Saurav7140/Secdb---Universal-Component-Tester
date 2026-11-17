# SECDB Component Tester — Phase 7A KiCad Project Setup Report

Date: 2026-09-13  
KiCad target: 10.0.6  
Status: Complete for planning and hierarchy setup; detailed schematic capture has not started.

## Outcome

A new KiCad project baseline has been created directly for Phase 7. It contains a top-level hierarchy and four empty functional sheets, project-level PCB/net-class defaults, frozen net-naming rules, and a staged validation plan. No Phase 1–6 file was edited or overwritten.

The Phase 7A plan matches the frozen Phase 5 analog-front-end architecture and the Phase 6 board pin map. Detailed schematic capture remains gated pending user confirmation.

## Sources inspected

Primary frozen hardware sources:

- `PHASE_5A_ARCHITECTURE_GPIO_SWITCH_AUDIT.md`
- `PHASE_5C_PROTECTION_POWER_AND_BOM_FREEZE.md`
- `PHASE_5C_PRELIMINARY_BOM.csv`
- `PHASE_5D_MASTER_SCHEMATIC_SPECIFICATION.md`
- `PHASE_5D_MASTER_NETLIST.csv` — 151 connection rows
- `PHASE_5D_MODE_TRUTH_TABLE.csv` — 23 hardware states
- `PHASE_5D_ERC_AND_PIN_AUDIT.csv` — 50 review checks

Primary firmware sources:

- `PHASE_6A_FIRMWARE_FOUNDATION.md`
- `Phase_6_Firmware/Core/Inc/secdb_board.h`
- `Phase_6_Firmware/Core/Inc/secdb_afe.h`
- `Phase_6_Firmware/Core/Src/secdb_afe.c`

Existing Phase 7 access note:

- `Phase_7_KiCad/PHASE_7_PRECHECK_KICAD_ACCESS.md`

All inspected references remain in `C:\Users\Saurav\Documents\ChatGPT\ECe mini prject`; they were read-only during this phase.

## Frozen-design confirmation

The Phase 5 and Phase 6 sources agree on these implementation-critical facts:

- MCU is STM32F401CCU6 in UFQFPN48.
- `SRC_A1` is PA8, package pin 29.
- PB11 is not bonded out on UFQFPN48 and must not be assigned.
- `SRC_A0` is PB10 and `SRC_EN` is PB12.
- Source selection uses TMUX1104DGSR.
- Sense selection uses TMUX1108PWR.
- The shared buffer is OPA391DBVR in voltage-follower configuration.
- ADC path is OPA391 output → 100 Ω → `ADC_BUFFERED` / PA0, with 1 nF C0G from PA0 to GND.
- Seven 100 kΩ pull-downs define disabled/address-zero mux states during reset.
- Firmware uses a 2 µs break-before-make interval, a 200 µs sense-settling interval, and discards the first ADC conversion.
- The unit tests only loose, unpowered, discharged components; the BJT socket is E-B-C and requires a known pinout.

No conflict was found between the frozen AFE specification and `secdb_board.h`.

## KiCad hierarchy established

Top-level file: `SECDB_Component_Tester.kicad_sch`

| Page | File | Planned scope |
|---:|---|---|
| 1 | `SECDB_Component_Tester.kicad_sch` | Hierarchy overview and Phase 7A gate notes |
| 2 | `01_usb_power.kicad_sch` | USB-C input, protection, AP2112, 3V3_D/3V3_A |
| 3 | `02_mcu_core_ui.kicad_sch` | STM32 core, clock, reset, SWD, buttons, LED, GPIO |
| 4 | `03_shared_afe_sockets.kicad_sch` | TMUX1104, TMUX1108, OPA391, ADC chain, sockets, ESD |
| 5 | `04_display_connector.kicad_sch` | Frozen 1×8 ST7735 connector |

The child sheets contain planning notes only. They intentionally contain no symbols, wires, labels, or footprints.

## Project conventions

- File and net names use ASCII; user-visible titles may use punctuation.
- Schematic pages use A3 landscape for consistent review printouts.
- Reference designators and functional names must follow the frozen master netlist (`U1` MCU, `U2` source mux, `U3` sense mux, `U4` buffer).
- Required component fields for populated parts: Value, Footprint, Datasheet, Manufacturer, MPN, tolerance/rating where relevant, and assembly status.
- Global labels are reserved for frozen cross-sheet rails/signals. Short internal analog nodes remain local where possible.
- No new net, alias, part substitution, or pin reassignment is permitted without a documented deviation review.
- Initial project net classes are `Default`, `POWER`, `USB_DIFF`, and `ANALOG_SENSITIVE`. Final widths/impedance geometry will be set only after stack-up and fabricator constraints are selected.
- PCB work will use one continuous ground plane and placement-based control of return currents.

## Validation plan

The detailed gate-by-gate plan is in `PHASE_7A_VALIDATION_PLAN.md`. It covers frozen-source reconciliation, schematic/ERC review, footprint/BOM review, PCB placement/DRC review, fabrication-output review, and first-board electrical validation.

Mandatory automated checks during later phases include:

1. Search all KiCad design/output files for any PB11 assignment.
2. Cross-check all 151 frozen master-netlist rows.
3. Verify all 23 mode truth-table states remain electrically realizable.
4. Re-run and disposition ERC/DRC with no unexplained errors.
5. Export and visually review schematic PDFs and all fabrication layers.

## KiCad validation result

- KiCad 10.0.6 parsed the top-level project and all four child sheets successfully.
- `kicad-cli sch erc --severity-all` reported **0 violations**.
- KiCad exported a five-page A3 PDF containing the hierarchy overview and the four planned child sheets.
- All five rendered PDF pages were visually inspected. Sheet titles, planning notes, critical PA8/PB11 warning, and unpowered-component safety text are legible with no clipping or overlap.

The reports are saved as `PHASE_7A_KICAD_ERC.rpt` and `SECDB_Component_Tester_Phase7A.pdf`.

## KiCad access note

KiCad 10.0.6 is installed. The native-window control connection exposed no targetable Windows applications during this phase, so live GUI inspection could not be completed from the session. The project files were instead parsed, ERC-checked, and PDF-exported by KiCad 10.0.6, followed by visual inspection of every rendered PDF page. The project should still be opened in KiCad for an interactive hierarchy review before Phase 7B capture.

This access limitation did not alter the frozen design or cause any detailed capture to proceed.

## Phase 7A acceptance gate

Phase 7A is ready for user review. Proceed to detailed schematic capture only after the user confirms:

- the four-sheet hierarchy is acceptable;
- the net-naming rules are acceptable;
- the validation gates are acceptable; and
- the frozen AFE confirmation above matches the intended design.
