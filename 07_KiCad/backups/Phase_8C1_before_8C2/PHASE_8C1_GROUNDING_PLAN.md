# Phase 8C1 - Routing Constraints and Grounding Plan

Date: 2026-09-14  
Project: SECDB Component Tester  
Status: approved routing baseline; copper routing has not started

## Ground architecture

- Use one continuous GND reference plane, primarily on B.Cu. Do not split the plane into analog and digital islands.
- Keep B.Cu free of ordinary signal routing wherever possible. Any unavoidable B.Cu crossing must be short and must not cut the return path beneath USB, HSE, or analog-sensitive traces.
- Control noise through component placement and routing separation, not through a ground-plane moat. The quiet analog region stays around U2/U3/U4, the DUT sockets, calibration divider, and ADC filter.
- Connect every decoupling-capacitor ground pad to the plane with a nearby via, normally within 1-2 mm. Avoid daisy-chaining capacitor returns through another component pad.
- Connect the MCU exposed pad directly to GND using its thermal-via array. The current 0.20 mm drills require confirmation with the chosen PCB assembler; tenting or filled via-in-pad may be needed to control solder wicking.
- Give the USB ESD return and USB connector shell a short, low-inductance path to the plane. The schematic currently places the shell on GND; review the final enclosure/EMI strategy before fabrication.
- Add ground stitching near the USB entry, board perimeter, display connector, and analog-region boundary without forming isolated copper islands.

## Routing order

1. Local capacitor-to-IC power/ground loops.
2. USB input power, protection, LDO input/output, 3V3_D, and 3V3_A distribution.
3. HSE crystal loop and its two load capacitors.
4. Precision analog source/sense path, buffer, ADC filter, and calibration references.
5. USB D+/D- segments on F.Cu over uninterrupted GND.
6. Remaining control, display, button, LED, and debug signals.
7. B.Cu GND zone, local F.Cu GND pours where useful, stitching vias, refill, and DRC.

## Separation and keep-clear rules

- Keep the HSE loop entirely on F.Cu, short, without vias, and away from USB, button, LED, and source-drive routing. Do not place plane voids beneath it.
- Keep analog-sensitive routes away from USB D+/D-, TFT clock/data, LED current, and button traces. Cross unavoidable noisy signals at approximately 90 degrees on the opposite layer.
- Route USB D+/D- together, with matched topology and the fewest possible vias. The current 0.20 mm width / 0.20 mm gap is a planning value, not a production impedance calculation.
- Do not route unrelated signals between an IC supply pin and its decoupling capacitor.
- The unpowered-component-only safety boundary remains mandatory: no DUT socket may be connected to raw USB 5 V.

## Net-class summary

- POWER_INPUT: 0.80 mm tracks, 1.00/0.50 mm vias, and 0.25 mm track-to-track separation.
- POWER: 0.50 mm tracks, 0.80/0.40 mm vias, and 0.20 mm track-to-track separation.
- GROUND_RETURN: 0.60 mm default trunks, 0.80/0.40 mm vias, plus a solid plane.
- USB_DIFF: 0.20 mm width and provisional 0.20 mm pair gap on F.Cu.
- ANALOG_SENSITIVE: 0.25 mm nominal traces and 0.20 mm track-to-track separation.
- CLOCK_CRITICAL: 0.20 mm nominal traces on F.Cu with no vias.
- Default: 0.25 mm tracks for low-speed digital/control nets.

All net classes retain a 0.15 mm package-compatible base clearance where required by the selected fine-pitch footprints. Wider routing separation is enforced only between routed tracks, preventing false DRC failures inside those footprints.

## Exit condition for Phase 8C1

Phase 8C1 is complete when all named classes and assignments load in KiCad, the custom-rule file parses, the Phase 8B placement remains unchanged, and DRC reports no board-rule violations. The expected unrouted-connection count may remain until Phase 8C2 begins.
