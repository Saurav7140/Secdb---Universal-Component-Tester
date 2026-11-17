# Phase 8D1 - Source/Sense MUX Control Routing

Date: 2026-09-23  
Project: SECDB Component Tester  
KiCad version: 10.0.6

## Outcome

Phase 8D1 completes the seven source/sense multiplexer selection and enable
signal nets between the STM32, their pull-down resistors, and U2/U3:

- `SRC_EN`, `SRC_A0`, and `SRC_A1`
- `SNS_EN`, `SNS_A0`, `SNS_A1`, and `SNS_A2`

KiCad DRC reports **0 violations**, **58 unconnected pads**, and **0 footprint
errors**. The unconnected count is down by 11 from Phase 8C5's 69.

## Routing method

- Component positions and orientations are unchanged from Phase 8C5.
- The Phase 8C5 USB and HSE routing is unchanged.
- Existing non-target copper is unchanged.
- Fine-pitch STM32 and MUX exits use 0.20 mm tracks and staggered layer changes.
- Longer control paths use clearance-aware two-layer corridors that avoid the
  completed power, analog, USB, and clock networks.
- The B.Cu GND zone and two-layer stack-up are retained.

## Scope boundary

The pull-down resistor ground returns remain part of the final ground-stitching
pass. They are intentionally not forced into local B.Cu islands created by the
new signal corridors. The VBUS monitor, RC controls, DUT-drive GPIO, debug/UI,
display, and final power/ground completion are also deferred as listed in
`PHASE_8D1_DEFERRED_GROUPS.csv`.

## Verification

- Automated structure, placement, net inventory, and copper-preservation audit: PASS.
- KiCad DRC after zone refill: 0 violations.
- Footprint errors: 0.
- Phase 8D1 target nets remaining in the unconnected report: none.
- Footprint placement changes from Phase 8C5: none.

## Next step

Phase 8D2 should complete the VBUS monitor, RC internal/discharge controls, and
DUT-drive GPIO fanout. That checkpoint may revise only the short USB MCU stubs
if needed to release the adjacent PA12 corridor; the long matched protected USB
pair should remain unchanged.
