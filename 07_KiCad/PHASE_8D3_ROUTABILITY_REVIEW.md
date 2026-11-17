# Phase 8D3 - Low-speed interface routability review

## Result

Phase 8D3 routing is **not installed**. The current PCB remains at the audited
Phase 8D2B checkpoint and still passes KiCad DRC with **0 violations**, **0
footprint errors**, and **52 unconnected items**.

The remaining debug, button, LED, and display nets cannot be added safely under
the present "add only Phase 8D3 nets" scope. The first constrained display
escape, `TFT_SCK`, is boxed in by previously completed MCU fanout on both copper
layers. Isolated routes can be forced into the area only by violating clearance
or trapping the next adjacent display pin.

## Verified congestion

The display pins occupy adjacent STM32 bottom-edge pads:

- pin 14: `TFT_CS`
- pin 15: `TFT_SCK`
- pin 16: `DIODE_A_GPIO`
- pin 17: `TFT_MOSI`
- pin 18: `TFT_RST`
- pin 19: `TFT_DC`

The existing B.Cu escape corridor is crossed by these retained routes:

| Y coordinate | Retained route |
|---:|---|
| 69.75 mm | `DIODE_A_GPIO` |
| 70.50 mm | `HSE_OUT` |
| 71.25 mm | `SRC_EN` |
| 72.00 mm | `SRC_A0` |
| 73.00 mm | `USB_VBUS_FUSED` |
| 74.00 mm | `USB_DP_ESD` |

The F.Cu side of the same corridor contains the VCAP and ground escapes,
regulated-power routing, source-control fanout, and the completed BJT drive
fanout. With a 0.45 mm signal via, 0.20 mm trace, and 0.20 mm clearance, a via
center must remain at least 0.525 mm from a foreign trace center. The 0.75 mm
spacing between several retained B.Cu rows therefore contains no legal via
centerline.

Trial routing confirmed that a legal staggered display fanout requires locally
rebuilding at least `DIODE_A_GPIO`, `BJT_B_GPIO`, and `BJT_C_GPIO`. Even after
those short stubs are removed in a temporary copy, the retained source-control,
VCAP, power, HSE, and USB routes still block the necessary layer transitions.

## Scope required to continue

One of these changes must be authorized before copper is edited:

1. **Coordinated two-layer re-fan** - rebuild the complete lower MCU escape as a
   group, including selected completed source-control, DUT-drive, support, and
   power stubs. This preserves two-layer fabrication cost but has the largest
   regression surface and must repeat preservation checks for every touched
   net.
2. **Convert to four layers** - keep the external placement and most existing
   F.Cu/B.Cu routing, add an internal ground plane plus an internal signal/power
   layer, and escape the remaining MCU pins internally. This is the cleaner
   electrical solution, but it increases fabrication cost and requires a real
   stack-up plus USB impedance-rule review.

The four-layer option is the recommended engineering choice because it restores
ground continuity and avoids another broad re-fan of already verified analog,
USB, and DUT-drive routing. If two-layer fabrication is a hard project
constraint, the coordinated two-layer re-fan is still possible only as a new,
explicitly broader checkpoint.

## Preserved baseline

- `SECDB_Component_Tester.kicad_pcb` was not changed.
- Footprint positions and orientations were not changed.
- No trial copper was copied into the project board.
- Baseline DRC remains 0 violations and 52 unconnected items.

