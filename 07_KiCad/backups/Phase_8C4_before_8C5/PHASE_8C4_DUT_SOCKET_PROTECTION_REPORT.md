# Phase 8C4 - DUT Socket and Protection Fanout

Date: 2026-09-20  
Project: SECDB Component Tester  
KiCad version: 10.0.6

## Outcome

Phase 8C4 completes the dense external test-terminal fanout between the R/C,
diode, and BJT sockets; the ESD1/ESD2 protection arrays; the current-limited
drive resistors; and the independent Kelvin-sense resistors.

KiCad DRC reports **0 violations**, **87 unconnected pads**, and **0 footprint
errors**. The unconnected count is down by 18 from Phase 8C3's 105, matching
the expected completion of the six multi-pad terminal networks.

## Completed terminal networks

- `RC_SOCKET`: J3, ESD1, R24 guarded source, and R25 Kelvin sense.
- `DIODE_A`: J4, ESD1, R36 limited drive, and R26 Kelvin sense.
- `DIODE_B`: J4, ESD1, R37 limited drive, and R27 Kelvin sense.
- `BJT_E`: J5, ESD1, R38 limited drive, and R28 Kelvin sense.
- `BJT_B`: J5, ESD2, R39 limited drive, and R29 Kelvin sense.
- `BJT_C`: J5, ESD2, R40 limited drive, and R30 Kelvin sense.

## Routing decisions

- ESD-array pads escape horizontally before any layer change or direction turn,
  preserving clearance at the 0.65 mm-pitch devices.
- Drive and Kelvin-sense paths remain electrically independent until they meet
  at the DUT terminal net.
- Short B.Cu crossovers are used only where required to pass stacked socket
  pins or existing power trunks; the B.Cu ground plane remains intact and was
  refilled after routing.
- All new signal tracks are 0.20 mm wide and use ordinary plated through-vias.
- No footprint positions or orientations were changed from Phase 8C3.

## Verification

- Automated structural and manifest audit: PASS.
- KiCad DRC after zone refill: 0 violations.
- Footprint errors: 0.
- DUT terminal nets remaining in the unconnected report: none.
- Two-layer stack-up and the single B.Cu GND plane preserved.

## Next step

Phase 8C5 should complete the USB differential pair and USB-C support signals,
followed by the HSE crystal loop. General MUX controls, UI, display, SWD, reset,
and status signals can then be completed in Phase 8D.
