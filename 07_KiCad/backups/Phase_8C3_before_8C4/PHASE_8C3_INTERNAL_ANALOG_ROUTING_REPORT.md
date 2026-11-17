# Phase 8C3 - Internal Precision Analog Routing

Date: 2026-09-20  
Project: SECDB Component Tester  
KiCad version: 10.0.6

## Outcome

Phase 8C3 is complete for the internal precision measurement chain. The source-range
selection, all eight sense-MUX inputs, calibration ladder, sense-MUX common node,
OPA391 buffer/feedback loop, ADC filter, and STM32 PA0 connection are routed.

KiCad DRC reports **0 violations**, **105 unconnected pads**, and **0 footprint
errors**. The unconnected count is down by 22 from Phase 8C2's 127. The remaining
connections belong to the DUT socket/ESD fanout, USB, crystal, mux controls,
buttons, display, SWD, LED, reset, and other Phase 8C4/8D routes.

## Fine-pitch breakout decision

The earlier draft used five 0.30/0.10 mm laser microvias. Those have been removed.
The final Phase 8C3 checkpoint uses five ordinary plated through-vias at
0.45 mm diameter with 0.20 mm mechanical drills for:

- SNS_BJT_B
- SNS_BJT_C
- SNS_CAL_1V1
- SNS_CAL_2V2
- ADC_BUFFERED

This matches the board's existing 0.20 mm minimum mechanical drill capability and
keeps the design two-layer, but the selected PCB fabricator must explicitly confirm
0.45/0.20 mm via capability before fabrication release.

## Routed internal analog paths

- 1 kOhm, 10 kOhm, and 100 kOhm precision source selections into U2.
- R/C, diode A/B, BJT E/B/C, and 1.1 V/2.2 V calibration inputs into U3.
- Calibrated 1.1 V and 2.2 V divider nodes and sense-resistor feeds.
- U3 common output to the OPA391 non-inverting input.
- OPA391 unity-feedback loop and output isolation resistor.
- ADC filter node, shunt capacitor, and STM32 PA0 input.
- Local quiet ground returns required by the added analog routes.

## Verification

- Automated structural and copper audit: PASS.
- KiCad DRC after zone refill: 0 violations.
- Footprint errors: 0.
- Microvias remaining: 0.
- Fine-pitch mechanical through-vias: 5.
- B.Cu GND plane retained and refilled.

## Next step

Phase 8C4 should route the DUT sockets through ESD1/ESD2 to the guarded source,
drive, and independent sense resistors. That dense protection fanout must be
completed as its own DRC-clean checkpoint before USB, HSE, and general low-speed
signals are routed in Phase 8D.
