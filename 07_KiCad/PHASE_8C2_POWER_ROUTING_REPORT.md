# Phase 8C2 - Power and Decoupling Routing

## Outcome

Phase 8C2 establishes the protected USB power chain, regulated digital and analog rails,
critical IC decoupling feeds/returns, the STM32 VCAP loop, and a continuous B.Cu GND plane.
KiCad DRC reports **0 violations**, **127 unconnected pads**, and **0 footprint errors**.
The unconnected count is down by 86 from Phase 8C1's unrouted value of 213.

## Routed power paths

- USB-C VBUS to F1, D1, D2, FB1, U5, and the USB ESD-device supply/bypass branch.
- U5 3V3_D output to its local capacitors, all STM32 VDD decouplers, and the display supply capacitors.
- FB2 3V3_A output to calibration-reference loads, both analog MUX supplies, the op-amp supply,
  the AFE capacitors, and STM32 VDDA decoupling.
- STM32 VCAP_1 to C12 with a short local loop.
- Short local ground returns from critical bypass capacitors and IC ground pins to the B.Cu plane.

## Placement refinements made for routing

- D2 was rotated 180 degrees for a linear fuse-to-Schottky-to-ferrite power path.
- C1 and C2 were rotated 270 degrees to face their power pads toward U5 and their ground pads toward local vias.
- C22 was rotated 270 degrees to shorten the display bypass connection.
- C6 was moved to (64.0, 59.0) mm and rotated 180 degrees for cleaner STM32 VDD fanout.
- No other footprint placement was changed.

## Routing strategy

- F.Cu carries the main power distribution and local decoupling connections.
- B.Cu is reserved primarily as the GND reference plane. Two controlled power crossovers are present:
  a 7.5 mm 3V3_D crossover near the upper power section and a 3V3_A crossover between the analog
  supply trunk and AFE. Their 0.80/0.40 mm vias comply with the POWER-class rule.
- All 27 GND vias are 0.60/0.30 mm. Dense final ground stitching and plane-return cleanup remain in Phase 8C5.
- Fine-pitch pad neck-downs use 0.20 or 0.30 mm tracks; main regulated rails use 0.50 mm and the
  USB/pre-regulator path uses up to 0.80 mm.

## Intentionally deferred low-current taps

Seven supply taps remain intentionally unrouted because they belong with their signal groups:
R3.1 (VBUS sensing), J2.1 (SWD supply), R7.1 (reset pull-up), R9.1/R10.1/R11.1
(button pull-ups), and R42.1 (display backlight link). Their ownership is recorded in
`PHASE_8C2_DEFERRED_TAPS.csv`.

## Verification

- Automated structural/copper audit: PASS.
- KiCad PCB DRC after zone refill: 0 violations.
- Footprint errors: 0.
- Remaining unconnected pads are expected because USB, clock, analog signal, and low-speed routing
  are assigned to later Phase 8C/8D steps.

## Next step

Phase 8C3 routes the precision analog measurement path, keeping source, sense, calibration,
buffer, and ADC connections short and separated from digital switching routes.
