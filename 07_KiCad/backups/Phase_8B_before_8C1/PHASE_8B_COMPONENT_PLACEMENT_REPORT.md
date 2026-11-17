# Phase 8B - Engineered Component Placement Report

Date: 2026-09-14  
Project: SECDB Component Tester  
KiCad version: 10.0.6

## Result

Phase 8B is complete. All 90 schematic footprints have been moved from the Phase 8A coarse groups into an electrically and mechanically organized placement. Four mounting holes and eleven external interface/control footprints are locked so later routing work cannot accidentally move them.

The board remains intentionally unrouted. Phase 8B establishes component position and orientation only.

## Placement strategy

- USB-C J1 is rotated toward the left board edge using the library footprint's PCB-edge datum.
- The USB ESD device remains close to J1; the 22 ohm USB series resistors are placed close to the MCU side of the pair.
- Fuse, TVS, Schottky diode, ferrite bead, LDO, rail capacitors, and analog-rail bead follow the power-flow direction across the upper-left area.
- R/C, diode, and BJT sockets are arranged across the upper edge in J3/J4/J5 order for vertical DUT insertion.
- Socket ESD protection and drive/sense resistors are directly below the DUT sockets.
- The precision source MUX, sense MUX, OPA391 buffer, ADC filter, and calibration ladder follow the analog signal path toward the MCU ADC input.
- U1 is central, with the HSE crystal and its network on the oscillator-pin side.
- MCU decoupling capacitors surround the package; VCAP, VDDA, and digital VDD capacitors all satisfy the Phase 8B pad-distance limits.
- SWD and BOOT0 access are placed at the lower-left edge.
- RESET, MODE, UP, and DOWN switches form a spaced lower-edge control row.
- The ST7735 header is positioned along the right side with local bulk/HF capacitance and its backlight link.

## Verified results

- Schematic footprints checked: 90
- Mechanical footprints: 4
- Locked mechanical/interface footprints: 15
- Total schematic footprint pads checked: 346
- Named nets preserved: 71
- Critical capacitor placements checked: 18 of 18 passed
- Estimated minimum-span ratsnest length, Phase 8A: 3220.5 mm
- Estimated minimum-span ratsnest length, Phase 8B: 2336.1 mm
- Estimated placement improvement: 27.5 percent
- KiCad DRC board-rule violations: 0
- Tracks/vias: 0
- Copper zones: 0

The project-level minimum clearance is 0.15 mm, minimum hole clearance is 0.15 mm, and minimum through-hole drill is 0.20 mm. These limits accommodate the selected USB-C connector, 0.5 mm-pitch VSSOP-10, and STM32 exposed-pad thermal vias. The POWER and ANALOG_SENSITIVE net classes retain 0.20 mm clearance; USB_DIFF and the general fine-pitch-compatible Default class use 0.15 mm.

KiCad still lists 213 unconnected items. That is expected because routing has not started; the count is not a placement-rule failure.

## Important physical checks still required

- Confirm the purchased ST7735 module dimensions, header pin-1 side, board overhang, and backlight circuit.
- Confirm enclosure dimensions and hand clearance around the three DUT sockets and four buttons.
- Confirm the selected USB4105 connector's shell alignment against the intended enclosure opening.
- Recalculate USB differential-pair geometry using the final PCB manufacturer's stackup before fabrication.

## Files produced

- `SECDB_Component_Tester.kicad_pcb` - Phase 8B editable placement
- `SECDB_Component_Tester.kicad_pro` - preserved project/net classes with footprint-compatible minimum constraints
- `PHASE_8B_PLACEMENT_MANIFEST.csv` - exact coordinates, rotations, locks, and schematic paths
- `PHASE_8B_CRITICAL_DISTANCES.csv` - decoupling/target pad-distance audit
- `PHASE_8B_AUDIT.txt` - placement, lock, distance, and ratsnest audit
- `PHASE_8B_DRC.rpt` - KiCad design-rule output
- `SECDB_Component_Tester_Phase8B.pdf` - six-layer placement review
- `SECDB_Component_Tester_Phase8B_3D.png` - headless KiCad board preview

## Phase 8C entry condition

Phase 8C can now establish the power and ground distribution, local ground-return strategy, analog/digital partitioning, USB pair corridor, crystal keep-clear area, and critical first routes. General signal routing and final copper pours should wait until those high-priority paths are validated.
