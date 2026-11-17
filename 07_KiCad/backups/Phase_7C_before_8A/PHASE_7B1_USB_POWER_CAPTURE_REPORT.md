# Phase 7B1 - USB-C and Power Schematic Capture

Date: 2026-09-13  
Toolchain: KiCad 10.0.6 CLI plus deterministic S-expression generator  
Status: Connectivity-first draft complete; GUI cleanup and manufacturer-symbol audit remain

## Completed scope

- USB4105-GF-A USB-C receptacle with duplicated VBUS, GND, D+/D- pins and shield connectivity.
- CC1 and CC2 each terminated by 5.1 kOhm to ground.
- VBUS protection and regulation path: MF-MSMF050-2 PPTC, SMBJ5.0A TVS, B5819W series Schottky, BLM21PG221SN1D bead, AP2112K-3.3 LDO, and input/output decoupling.
- USBLC6-2SC6 USB data ESD protection and 22 ohm MCU-side series resistors.
- 3V3_D to 3V3_A ferrite isolation.
- 1 kOhm USB VBUS sense feed for MCU PA9.
- Frozen MCU interface labels retained: USB_VBUS_SENSE=PA9, USB_DM_MCU=PA11, USB_DP_MCU=PA12. PB11 remains unused.
- Exact intended PCB footprint strings assigned to every component.

## Validation performed

- Phase 7B2 netlist audit corrected the custom-symbol local-Y/page-Y transform and regenerated this sheet before Phase 7B2 installation.
- Confirmed `USB_VBUS_RAW`: J1 A4/A9/B4/B9 and F1 pin 1.
- Confirmed `USB_DP_CONN`: J1 A6/B6 and U6 pin 1; `USB_DM_CONN`: J1 A7/B7 and U6 pin 3.
- Confirmed U6 system pins: pin 6 to `USB_DP_ESD`, pin 4 to `USB_DM_ESD`, and pin 5 to `USB_VBUS_FUSED`.
- Confirmed AP2112K: pins 1/3 to `LDO_IN`, pin 2 to GND, pin 5 to `3V3_D`, and pin 4 not connected.
- The full five-page hierarchy parses in KiCad 10.0.6.
- ERC result: 0 errors, 40 warnings.
- Warning breakdown: 18 custom-library warnings, 18 off-grid endpoint warnings, and 4 intentionally isolated cross-sheet labels.
- Hierarchical PDF export completed and all five pages were rendered for visual review.
- Phase 7B1 page is readable with no clipping or overlapping blocks that prevent interpretation.
- KiCad BOM and netlist exports completed.

## Important limitations to repair later in the KiCad GUI

1. The symbols are embedded connectivity-first symbols. Replace or formally validate them against manufacturer/library symbols before fabrication.
2. All custom symbol pins are currently passive. Therefore ERC confirms file integrity and net connectivity, not full electrical pin-type correctness.
3. Connections use global labels at pins instead of polished wire routing. This is electrically usable for netlisting but should be redrawn for presentation quality.
4. Several coordinates are off KiCad's preferred connection grid; snap symbols and labels to grid during cleanup.
5. Add a project-local `SECDB` symbol library or replace the temporary symbols to clear the 18 library warnings.
6. The four isolated labels are expected until the MCU and analog sheets are captured: USB_DP_MCU, USB_DM_MCU, USB_VBUS_SENSE, and 3V3_A.
7. Footprint pad numbering, physical orientation, USB-C shell strategy, diode polarity, ESD flow-through orientation, and AP2112K pinout must be visually cross-checked before PCB layout.

## Gate decision

Phase 7B1 is accepted as a connectivity-first CLI draft and is suitable as the source for the next schematic sheet. It is not fabrication-ready and must not be treated as a final electrical or PCB review.

## Next step

Phase 7B2: capture the STM32F401CCU6 core, power/decoupling, reset/boot, SWD, USB MCU pins, control GPIO, buttons, and status LED while preserving the frozen PA8 assignment and excluding PB11.
