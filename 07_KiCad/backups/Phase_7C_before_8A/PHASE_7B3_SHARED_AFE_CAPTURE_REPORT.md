# Phase 7B3 - Shared Analog Front End and Component Sockets

Date: 2026-09-13  
Toolchain: KiCad 10.0.6 CLI plus deterministic S-expression generator  
Status: Connectivity-first draft complete; GUI symbol, footprint, placement and polarity audit remain

## Completed scope

- TMUX1104DGSR source multiplexer with 1 kOhm, 10 kOhm and 100 kOhm precision source ranges plus the 1 kOhm discharge path.
- Calibrated 330 ohm guard between the source-mux common pin and exposed R/C socket.
- Separate R/C, diode A-B, and transistor E-B-C turned-pin sockets.
- Two TPD4E1B06DCKR socket-edge ESD arrays covering all six exposed non-ground terminals.
- Five limited DUT-drive paths: diode A/B and BJT E/B/C.
- Eight independent 1 kOhm sense branches feeding TMUX1108PWR.
- Three-resistor matched 10 kOhm calibration ladder producing `CAL_2V2` and `CAL_1V1`.
- OPA391DBVR unity-gain follower with output tied directly to its inverting input.
- 100 ohm/1 nF ADC isolation and anti-kickback filter feeding STM32 PA0 through `ADC_BUFFERED`.
- Dedicated 100 nF decoupling for each AFE IC plus 1 uF analog bulk capacitance.

## Pin-level verification

- TMUX1104 DGS pins 1-10 match the frozen Phase 5D source-mux assignment.
- TMUX1108 PW pins 1-16 match the frozen eight-channel sense assignment; pins 3 and 14 are both grounded.
- OPA391 DBV mapping confirmed: OUT=1, V-=2, +IN=3, -IN=4, V+=5.
- TPD4E1B06 mapping confirmed: IO1=1, IO2=2, GND=3, IO3=4, IO4=5, NC=6.
- J_RC pin order is R/C-GND; J_DIODE is A-B; J_BJT is E-B-C.
- ESD1 pin 6 and ESD2 pins 4/5/6 are explicitly unconnected as specified.

## Automated connectivity validation

- All 31 critical AFE nets contain every expected component pin in the exported KiCad netlist.
- Supply and ground assertions passed for U2, U3, U4, ESD1, ESD2, C16-C20, source references and calibration ladder.
- `ADC_BUFFERED` joins R41 pin 2, C16 pin 1 and MCU U1 pin 10.
- `SENSE_COM` joins TMUX1108 pin 8 and OPA391 pin 3.
- `OPA_OUT` joins OPA391 pins 1/4 and R41 pin 1.

## KiCad validation

- Complete five-page hierarchy parses in KiCad 10.0.6.
- ERC result: 0 errors and 123 warnings.
- Warning breakdown: 86 temporary custom-library warnings, 32 off-grid endpoint warnings, and 5 labels isolated until the display sheet is captured.
- The invalid initial VSSOP footprint alias was replaced with the installed `Package_SO:MSOP-10_3x3mm_P0.5mm` land pattern.
- Netlist, BOM and five-page PDF exports completed.
- The A3 analog page was rendered and revised to remove touching reference/value labels.

## Important limitations before fabrication

1. All temporary custom-symbol pins are passive, so ERC verifies structural connectivity rather than full electrical pin types.
2. Replace or formally validate the TMUX1104, TMUX1108, OPA391, ESD and socket symbols in KiCad GUI.
3. Compare the MSOP-10 footprint with the exact TI DGS/VSSOP-10 package drawing; compare the TSSOP-16, SOT-23-5 and SC70-6 land patterns likewise.
4. Confirm socket pin-one orientation, BJT E-B-C labelling and LED/ESD/polarized-part orientation before PCB routing.
5. Place ESD1/ESD2 directly beside the socket edge with short ground returns and nearby vias.
6. Keep the OPA391, TMUX1108, ADC filter, and MCU PA0 path compact and away from USB/display signals.
7. Preserve one continuous ground plane and prevent USB/display return currents from crossing the precision analog cluster.
8. This CLI-generated connectivity draft is not fabrication-ready.

## Gate decision

Phase 7B3 is accepted as a connectivity-first CLI draft. Its pin-level topology matches the Phase 5D master netlist and it is suitable for continuing to Phase 7B4.

## Next step

Phase 7B4: capture the eight-pin ST7735 display connector, local decoupling, and backlight link using the frozen board-side order.
