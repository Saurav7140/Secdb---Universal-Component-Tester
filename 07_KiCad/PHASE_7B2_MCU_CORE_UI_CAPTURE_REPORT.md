# Phase 7B2 - STM32 Core and UI Schematic Capture

Date: 2026-09-13  
Toolchain: KiCad 10.0.6 CLI plus deterministic S-expression generator  
Status: Connectivity-first draft complete; GUI symbol, footprint, and physical-layout audit remain

## Completed scope

- STM32F401CCU6 with all 48 package pins plus exposed pad represented.
- VDD pins 24/36/48 on `3V3_D` with one 100 nF capacitor per pin.
- VSS pins 23/35/47 and exposed pad on GND.
- VBAT pin 1 on `3V3_D` with 100 nF.
- VDDA/VREF+ pin 9 on `3V3_A` with 1 uF and 100 nF; VSSA/VREF- pin 8 on GND.
- VCAP_1 pin 22 with dedicated 4.7 uF low-ESR capacitor to GND.
- 8 MHz ABLS crystal, fitted 0 ohm tuning link, and two 30 pF C0G load capacitors.
- NRST 10 kOhm pull-up, 100 nF capacitor, reset pushbutton, and SWD connection.
- BOOT0 10 kOhm pull-down and labelled service test point.
- Five-pin SWD connector: `3V3_D`, `SWDIO`, GND, `SWCLK`, and `NRST`.
- Three active-low UI buttons on PC13/PC14/PC15 with 10 kOhm external pull-ups; no LSE crystal.
- PB8 status LED through 1 kOhm.
- Seven 100 kOhm safe-state pull-downs on source/sense mux enable and address nets.
- Spare PB2-PB7 and PA15 explicitly marked unconnected.

## Frozen pin-map checks

- PA0 pin 10 -> `ADC_BUFFERED`.
- PA8 pin 29 -> `SRC_A1`; PB11 is not present or assigned.
- PA9 pin 30 -> `USB_VBUS_SENSE`.
- PA11 pin 32 -> `USB_DM_MCU`; PA12 pin 33 -> `USB_DP_MCU`.
- PA13 pin 34 -> `SWDIO`; PA14 pin 37 -> `SWCLK`.
- PB8 pin 44 -> `STATUS_LED`.
- NRST pin 7, VDDA pin 9, and VCAP_1 pin 22 confirmed in the exported KiCad netlist.

## Validation performed

- Corrected and regenerated Phase 7B1 and Phase 7B2 after detecting the custom-symbol local-Y/page-Y coordinate transform during netlist auditing.
- The complete hierarchy parses in KiCad 10.0.6.
- ERC result: 0 errors and 81 warnings.
- Warning breakdown: 51 temporary custom-library warnings, 19 off-grid endpoint warnings, and 11 labels currently isolated until the AFE/display sheets are captured.
- KiCad netlist and BOM exports completed.
- All critical package pins and USB cross-sheet nets were checked against exported netlist nodes.
- The five-page hierarchical PDF was rendered and visually reviewed.

## Important limitations to repair before fabrication

1. Temporary connectivity-first symbols use passive pins; ERC does not yet validate electrical pin types.
2. Replace or formally validate the custom MCU, SWD, switch, crystal, and other temporary symbols in the GUI.
3. The assigned installed-library MCU footprint is `QFN-48-1EP_7x7mm_P0.5mm_EP5.3x5.3mm_ThermalVias`. Confirm every land-pattern dimension against the exact STM32F401CCU6 UFQFPN48 package drawing before PCB layout.
4. Confirm exposed-pad paste segmentation and thermal-via design with the assembler.
5. Validate crystal load capacitance and startup margin on the first PCB; 30 pF is the frozen starting value, not a measured final value.
6. Confirm LED polarity, tactile-switch pad pairing, connector pin-one orientation, and all physical footprints in KiCad GUI.
7. Snap remaining off-grid endpoints to the schematic grid and add a project-local SECDB symbol library or replace the custom symbols.
8. This connectivity-first schematic is not fabrication-ready.

## Gate decision

Phase 7B2 is accepted as a connectivity-first CLI draft. Its package-pin connectivity matches the Phase 5D master netlist and it is suitable for continuing to Phase 7B3.

## Next step

Phase 7B3: capture the shared analog front end, reference network, protection devices, and R/C, diode, and BJT sockets.
