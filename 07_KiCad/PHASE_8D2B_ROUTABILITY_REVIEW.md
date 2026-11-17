# Phase 8D2B — Two-layer routability review

## Status

Phase 8D2B signal routing is **not installed**. The current Desktop PCB remains at the audited Phase 8D2A checkpoint and still passes KiCad DRC with **0 violations** and **0 footprint errors**.

The rejected trial boards exist only in the workspace `tmp` development area. None of their copper was copied into the project folder.

## Nets under review

- `USB_VBUS_SENSE`
- `DIODE_A_GPIO`
- `DIODE_B_GPIO`
- `BJT_E_GPIO`
- `BJT_B_GPIO`
- `BJT_C_GPIO`

## Confirmed routing constraints

### STM32 right side

Pins 29–33 leave the 0.5 mm-pitch MCU edge in this order:

1. `SRC_A1`
2. `USB_VBUS_SENSE`
3. `DIODE_B_GPIO`
4. `USB_DM_MCU`
5. `USB_DP_MCU`

The F.Cu exit is occupied by the short USB-DM stub and the SRC-A1 fanout. The corresponding B.Cu window is occupied by the SNS-A2, SNS-A0, SNS-EN, and SRC-A1 routes. A fine-grid search could route an individual new signal only by breaking one of those validated nets or by adding a crossing/clearance violation.

### STM32 left and lower sides

The three BJT GPIO exits are constrained by the local VDDA/`3V3_A` stub on F.Cu and the HSE/ADC fanout on B.Cu. Trial routes that forced these signals below the USB area crossed the retained USB pair, MUX-control routes, or the fused-VBUS power corridor.

## Rejected trial evidence

The following approaches were generated and checked with KiCad DRC:

- Direct surface routing around the short USB pair.
- Immediate fine-via escapes beside MCU pins 30 and 31.
- Local SNS-A2 rip-up and re-fan.
- Short USB-DM rip-up and B.Cu re-fan.
- Ordered below-USB corridors for the three BJT GPIOs.

Each produced real clearance, track-crossing, shorting, or duplicate/dangling-via errors. These attempts were rejected.

## Required design decision

Continuing safely requires expanding the authorized scope beyond adding only the six Phase 8D2B nets:

### Recommended: coordinated two-layer local re-fan

Temporarily rip up and jointly reroute the short MCU-side portions of:

- `USB_DM_MCU`
- `SRC_A1`
- `SNS_A2`, and possibly `SNS_A0`, `SNS_A1`, and `SNS_EN`
- the local `3V3_A`, `HSE_IN`, `HSE_OUT`, and `ADC_BUFFERED` fanout

Footprint positions and the long USB differential-pair routes would remain fixed. Every changed pre-existing net would be listed in the manifest and independently audited.

### Alternative: four-layer conversion

Add internal ground and signal/power layers, then escape the congested MCU pins through the inner routing layer. This is electrically cleaner but increases fabrication cost and requires a new stack-up and USB impedance review.

### Alternative: schematic pin-remap ECO

Reassign suitable MCU GPIO/ADC functions so destination groups leave compatible MCU edges. This can produce the cleanest two-layer layout but changes the schematic and firmware pin map and therefore requires returning to the Phase 7 design baseline.

