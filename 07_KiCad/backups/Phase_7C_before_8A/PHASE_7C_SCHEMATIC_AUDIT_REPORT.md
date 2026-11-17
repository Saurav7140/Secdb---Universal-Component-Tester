# Phase 7C - Schematic Pin, Footprint and ERC Audit

Date: 2026-09-14  
Project: SECDB Component Tester  
Review gate: Final schematic audit before PCB creation

## Outcome

Phase 7C passes. The complete five-page KiCad hierarchy now reports **0 ERC errors and 0 ERC warnings**. All 90 components, 291 schematic pins/terminals, and 23 unique assigned footprints passed automated cross-checks.

The project is ready to enter PCB setup and placement. This does not mean the PCB is ready for fabrication; layout, routing, DRC, mechanical review, and first-board measurements remain.

## Corrections completed

1. Added the project-local `SECDB.kicad_sym` library and `sym-lib-table`, resolving all 90 missing-library warnings without relying on a machine-global library entry.
2. Standardized the shared two-pin project symbol across all four sheets.
3. Snapped every generated symbol origin and attached label endpoint to KiCad's 50 mil connection grid, removing all 32 off-grid warnings.
4. Updated the root page and all four sheet title blocks to identify the Phase 7C audited state.
5. Re-exported and checked the whole-project netlist and BOM.

## Audit coverage

- Every captured reference is present exactly once: 90 expected, 90 found.
- No unexpected references are present.
- Every expected connected pin has the exact frozen net name.
- Every intentionally unused pin has an explicit no-connect net.
- Every assigned footprint exists in the installed KiCad 10 footprint libraries.
- Every schematic pin number exists as a pad number in its assigned footprint.
- The critical MCU, multiplexer, op-amp, LDO, ESD and connector mappings were checked against the Phase 5D manufacturer-pin audit and the official manufacturer documents listed below.

Detailed results are recorded in `PHASE_7C_PIN_FOOTPRINT_AUDIT.csv`.

## Critical package results

| Reference | Device | Pin/package result |
|---|---|---|
| U1 | STM32F401CCU6 | UFQFPN48 pins 1-48 plus exposed GND pad 49 match the 49-pad thermal-via footprint |
| U2 | TMUX1104DGSR | DGS/VSSOP-10 pin order verified |
| U3 | TMUX1108PWR | PW/TSSOP-16 pin order verified |
| U4 | OPA391DBVR | DBV/SOT-23-5 pin order verified; unity-feedback net is correct |
| U5 | AP2112K-3.3TRG1 | SOT25 pins VIN/GND/EN/NC/VOUT verified |
| U6 | USBLC6-2SC6 | SOT-23-6 flow-through USB pin order verified |
| ESD1-ESD2 | TPD4E1B06DCKR | DCK/SC70-6 IO and NC pin order verified |
| J1 | USB4105-GF-A | All A/B contact and shield identifiers match the assigned footprint pads |

## ERC result

```text
ERC messages: 0
Errors: 0
Warnings: 0
```

The project configuration continues to ignore generic checks that are unsuitable for this label-based connectivity-first capture, including single-use global labels and footprint-filter matching. Phase 7C compensates for the footprint-filter check with a stricter direct test: every assigned footprint file and every required pad number were verified.

## Items that remain hardware-dependent

- Confirm the exact purchased ST7735 module's supply voltage, connector orientation, pin order, and whether its backlight already includes current limiting.
- Verify AP2112 temperature and 3.3 V droop with the real display current.
- Verify the 8 MHz HSE oscillator starts reliably and supports USB operation across expected conditions.
- Confirm the exact physical tactile-switch and socket parts before ordering the PCB, even though the selected generic footprints are electrically and dimensionally consistent with the frozen plan.

These are not unresolved schematic nets and do not block PCB placement, but they must be closed before fabrication approval.

## Official pinout sources

- STM32F401xB/xC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- TMUX1108 datasheet: https://www.ti.com/lit/ds/symlink/tmux1108.pdf
- OPA391 datasheet: https://www.ti.com/lit/ds/symlink/opa391.pdf
- TPD4E1B06 datasheet: https://www.ti.com/lit/ds/symlink/tpd4e1b06.pdf
- AP2112 datasheet: https://www.diodes.com/assets/Datasheets/AP2112.pdf
- USBLC6-2 datasheet: https://www.st.com/resource/en/datasheet/usblc6-2.pdf
- GCT USB4105 product page: https://gct.co/connector/usb4105
- Abracon ABLS series data: https://abracon.com/parametric/crystals/ABLS-8.000MHZ-B2-T

## Next phase

Phase 8A should create the `.kicad_pcb` file and freeze the mechanical foundation before placement:

1. Select the layer stack and design rules appropriate for the intended PCB manufacturer.
2. Define the board outline, mounting holes, connector edges and keep-out regions.
3. Import the 90 audited footprints from the schematic.
4. Lock USB, socket, display, SWD and button positions before analog/power placement.
