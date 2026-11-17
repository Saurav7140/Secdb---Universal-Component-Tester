# Phase 8C5 - USB-C and HSE Routing

Date: 2026-09-20  
Project: SECDB Component Tester  
KiCad version: 10.0.6

## Outcome

Phase 8C5 completes the USB-C configuration-channel connections, the reversible
connector-side D+/D- breakout, the protected differential path through U6 and
R4/R5, the STM32 USB stubs, and the complete HSE crystal/load-capacitor loop.

KiCad DRC reports **0 violations**, **69 unconnected pads**, and **0 footprint
errors**. The unconnected count is down by 18 from Phase 8C4's 87.

## USB routing

- Both CC pins are connected to their 5.1 kOhm pull-down resistors and local
  ground returns.
- Both reversible D+ contacts and both reversible D- contacts are joined before
  entering the USB ESD device.
- The protected D+/D- pair follows a quiet B.Cu perimeter corridor, away from
  the precision analog section.
- R4 and R5 were rotated 180 degrees in place so protected pads face outward
  and MCU pads face inward; their positions did not change.
- Connector-side D+/D- length mismatch: **0.248 mm**.
- Protected-pair D+/D- length mismatch: **0.038 mm**.
- MCU-stub D+/D- length mismatch: **0.409 mm**.
- All USB tracks use the provisional 0.20 mm geometry defined in Phase 8C1.

The final controlled-impedance width and spacing cannot be certified until the
chosen PCB manufacturer supplies the actual two-layer stack-up, dielectric
thickness, and copper weight. Recalculate the USB rule before fabrication.

## HSE routing

- HSE_IN connects the STM32, crystal input, and C13 load capacitor.
- HSE_XTAL_OUT connects the crystal output, C14, and the R6 damping resistor.
- HSE_OUT connects the damped clock signal to the STM32.
- C13 and C14 use local ground vias.
- The C10 ground dogleg was replaced by a shorter tented via-in-pad return to
  open the adjacent clock breakout corridor.

The existing crystal placement was preserved. Its distance from the MCU is
acceptable for this verified prototype checkpoint, but a production respin
should move the crystal and load capacitors closer to the oscillator pins.

## Deferred USB monitor

`USB_VBUS_FUSED` and `USB_VBUS_SENSE` remain unrouted. They are low-speed
monitoring signals and are assigned to Phase 8D, where they can be integrated
with the remaining MCU control routes without crossing the matched USB pair.

## Verification

- Automated structural, connectivity, via, and length audit: PASS.
- KiCad DRC after zone refill: 0 violations.
- Footprint errors: 0.
- Phase 8C5 target nets remaining in the unconnected report: none.
- Two-layer stack-up and the B.Cu GND plane are retained.

## Next step

Phase 8D should complete the remaining low-speed MCU/control routing: VBUS
monitor, source/sense MUX controls, DUT-drive GPIO signals, SWD/reset/BOOT,
buttons, status LED, and display interface.
