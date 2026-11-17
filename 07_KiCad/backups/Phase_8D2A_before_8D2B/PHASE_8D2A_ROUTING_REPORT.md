# Phase 8D2A — Fused-VBUS and RC-control routing

## Outcome

Phase 8D2A is complete and passes KiCad DRC with **0 violations** and **0 footprint errors**.

Completed nets:

- `USB_VBUS_FUSED` — routed from the protected/fused USB input network to the VBUS-monitor divider input using 0.50 mm copper and 1.00/0.50 mm power vias.
- `RC_INTERNAL` — routed from the source multiplexer to the RC test resistor.
- `SRC_DISCHARGE` — routed from the source multiplexer to the discharge resistor.

The checkpoint adds 21 track segments and 6 vias. All 94 footprints, all 71 named nets, and all non-target pre-existing copper are preserved.

## Verification

- KiCad DRC violations: **0**
- KiCad footprint errors: **0**
- DRC unconnected items: **57**
- Independent audit: **PASS**

The raw unconnected-item count is not strictly monotonic because refilling the B.Cu ground zone after a new cross-board power route changes how KiCad groups remaining ground ratsnest islands. The three Phase 8D2A target nets do not appear in the unresolved-item report.

## Why Phase 8D2 was split

`USB_VBUS_SENSE` and the five DUT-drive GPIOs leave adjacent fine-pitch STM32 pins through the already-routed USB and MUX-control corridor. Trial routes produced genuine USB, analog-clearance, and track-crossing errors, so they were rejected and were not copied into this checkpoint.

Phase 8D2B will deliberately re-fan only the necessary short MCU-side stubs, route those six signals, and repeat the independent preservation and DRC checks. The long USB differential-pair routing remains protected.

