# SECDB — Phase 5D Master Schematic Specification and Final Design Review

## Status and scope

Phase 5D is complete at the schematic-specification level. Every electrical net, IC pin, connector pin, reset state, measurement mode, and supply connection required for Version 1 is frozen in an implementation-ready form.

The deliverables are:

- `PHASE_5D_MASTER_SCHEMATIC_SPECIFICATION.md` — design intent and review record;
- `PHASE_5D_MASTER_NETLIST.csv` — exact pin-to-net connection list;
- `PHASE_5D_MODE_TRUTH_TABLE.csv` — hardware states for firmware;
- `PHASE_5D_ERC_AND_PIN_AUDIT.csv` — pin/package/electrical-rules review;
- updated Phase 5A pin map and Phase 5C BOM.

This phase does not claim that a manufactured PCB has passed electrical, thermal, USB, oscillator, or ESD testing. It freezes the schematic that must be captured in KiCad and then validated physically.

## Critical package correction found in Phase 5D

The STM32F401CCU6 UFQFPN48 package does not expose PB11. Phase 5A had provisionally assigned PB11 to `SRC_A1`; that assignment was impossible on the selected package.

The corrected and frozen assignment is:

```text
SRC_A1 = PA8, MCU pin 29
SRC_A0 = PB10, MCU pin 21
SRC_EN = PB12, MCU pin 25
```

PA8 was unused and is bonded out, so this correction introduces no conflict. It changes only a digital mux-control net and does not invalidate any Phase 5B or 5C analog simulation.

## Master schematic page organization

Capture the design as four hierarchical sheets or four clearly separated schematic regions.

### Sheet A — USB-C and power

```text
USB4105-GF-A
  VBUS -> 0.50 A PPTC -> USB_VBUS_FUSED
                             |-> SMBJ5.0A TVS -> GND
                             |-> 1 kΩ -> PA9 VBUS sense
                             |-> USBLC6-2 VBUS pin
                             +-> B5819W -> ferrite -> AP2112 input

AP2112 3.3 V output -> 3V3_D
3V3_D -> ferrite -> 3V3_A
```

CC1 and CC2 each have their own 5.1 kΩ, 1% resistor to ground. D+ and D− use the flow-through USBLC6-2SC6 and then 22 Ω series resistors close to the MCU. The connector shell stakes connect directly to the continuous ground plane.

The 0.50 A PPTC is a fault limiter. The initial continuous board-load target remains 150 mA maximum until the display and AP2112 temperature are measured.

### Sheet B — STM32 core, clock, reset, debug, and UI

Use STM32F401CCU6 in the 48-pin UFQFPN footprint with the exposed pad assigned to ground. The exact power connections are:

| MCU supply | Connection |
|---|---|
| VDD pins 24, 36, 48 | `3V3_D`, one 100 nF at each pin |
| VSS pins 23, 35, 47 | Ground plane |
| Exposed pad | Ground plane with thermal-via array |
| VDDA/VREF+ pin 9 | `3V3_A`, 1 µF + 100 nF |
| VSSA/VREF− pin 8 | Ground plane |
| VCAP_1 pin 22 | 4.7 µF low-ESR to ground only |
| VBAT pin 1 | `3V3_D`, 100 nF to ground |

The board uses an 8 MHz ABLS-8.000MHZ-B2-T crystal. PH0 is `HSE_IN`; PH1 reaches the other crystal pin through a fitted 0 Ω tuning resistor. Start with 30 pF C0G capacitors from both crystal nodes to ground. Keep the loop short and verify startup margin on the first PCB.

NRST uses 10 kΩ to `3V3_D`, 100 nF to ground, a pushbutton to ground, and a connection to the SWD header. BOOT0 uses 10 kΩ to ground and a labelled service pad.

The three active-low UI buttons use PC13, PC14, and PC15 with external 10 kΩ pull-ups. No 32.768 kHz LSE crystal is fitted. The status LED uses PB8 through 1 kΩ.

### Sheet C — shared analog front end and sockets

```text
3V3_A -> precision resistor bank -> TMUX1104 U_SRC
TMUX1104 D -> RC_INTERNAL -> 330 Ω calibrated guard -> RC_SOCKET

RC_SOCKET ----1 kΩ---- S1
DIODE_A ------1 kΩ---- S2
DIODE_B ------1 kΩ---- S3
BJT_E --------1 kΩ---- S4     TMUX1108 U_SENSE
BJT_B --------1 kΩ---- S5            D
BJT_C --------1 kΩ---- S6            |
CAL_1V1 ------1 kΩ---- S7         OPA391 follower
CAL_2V2 ------1 kΩ---- S8            |
                                      100 Ω
                                         +---- PA0
                                         |
                                        1 nF
                                         |
                                        GND
```

U_SRC is TMUX1104DGSR, using its DGS/VSSOP-10 pinout. U_SENSE is TMUX1108PWR, using its PW/TSSOP-16 pinout. U_SENSE VSS and GND both connect to ground for single-supply operation.

The six exposed non-ground test terminals use two TPD4E1B06DCKR devices. The ESD parts must sit beside the socket headers, with their ground pins connected by short traces and nearby vias. The 1 kΩ sense resistors belong after the exposed terminal and before U_SENSE.

The three socket groups are physically separate:

- `J_RC`: pin 1 `RC_SOCKET`, pin 2 ground;
- `J_DIODE`: pin 1 `DIODE_A`, pin 2 `DIODE_B`;
- `J_BJT`: pins 1–3 are E, B, C in that order.

Use 2.54 mm female turned-pin headers for the first board. This makes the electrical pin order deterministic and avoids committing the first PCB to a particular ZIF socket footprint.

### Sheet D — display connector

The board-side 1×8, 2.54 mm female header order is frozen as:

| Pin | Signal |
|---:|---|
| 1 | GND |
| 2 | `3V3_D` |
| 3 | `TFT_SCK` |
| 4 | `TFT_MOSI` |
| 5 | `TFT_CS` |
| 6 | `TFT_DC` |
| 7 | `TFT_RST` |
| 8 | `TFT_BL` |

`TFT_BL` is fed from `3V3_D` through a fitted 0 Ω link. If the purchased module exposes a bare LED without a current-limiting resistor, replace the link with a calculated resistor before connection. Use a short adapter cable if the module's physical header order differs; do not silently change the PCB net order.

## Frozen STM32F401CCU6 functional pin map

| Function | MCU pin | Package pin number |
|---|---|---:|
| ADC input | PA0 | 10 |
| BJT emitter drive | PA1 | 11 |
| BJT base drive | PA2 | 12 |
| BJT collector drive | PA3 | 13 |
| TFT chip select | PA4 | 14 |
| TFT clock | PA5 | 15 |
| Diode A drive | PA6 | 16 |
| TFT MOSI | PA7 | 17 |
| Source-mux A1 | PA8 | 29 |
| USB VBUS sense | PA9 | 30 |
| Diode B drive | PA10 | 31 |
| USB D− | PA11 | 32 |
| USB D+ | PA12 | 33 |
| SWDIO | PA13 | 34 |
| SWCLK | PA14 | 37 |
| TFT reset | PB0 | 18 |
| TFT data/command | PB1 | 19 |
| Source-mux A0 | PB10 | 21 |
| Source-mux enable | PB12 | 25 |
| Sense-mux A0 | PB13 | 26 |
| Sense-mux A1 | PB14 | 27 |
| Sense-mux A2 | PB15 | 28 |
| Status LED | PB8 | 44 |
| Sense-mux enable | PB9 | 46 |
| Mode button | PC13 | 2 |
| Up button | PC14 | 3 |
| Down button | PC15 | 4 |
| HSE input/output | PH0/PH1 | 5/6 |

Spare PA15 and PB2–PB7 remain unconnected and should default to analog/no-pull in firmware. They may have labelled copper test pads but no populated headers are required.

## Hardware reset and firmware transition safety

Seven 100 kΩ resistors pull down:

- `SRC_EN`, `SRC_A0`, `SRC_A1`;
- `SNS_EN`, `SNS_A0`, `SNS_A1`, `SNS_A2`.

Both multiplexers are therefore disabled and address zero is defined while the MCU is in reset. PA1, PA2, PA3, PA6, and PA10 reset as inputs and must be explicitly configured as analog/no-pull before any test mode begins.

Every mode change uses this order:

```text
1. Disable U_SENSE.
2. Disable U_SRC.
3. Put PA1, PA2, PA3, PA6, and PA10 in analog/no-pull mode.
4. Wait for the required discharge or changeover condition.
5. Configure the intended source or GPIO drive state.
6. Select the required U_SENSE address.
7. Enable U_SENSE.
8. Wait 200 µs and discard the first ADC conversion.
9. Acquire and validate the measurement.
10. Disable sensing and remove drive before returning to idle.
```

The capacitor timing exception from Phase 5B/5C remains: select and settle U_SENSE first, then start the timer at the U_SRC charge-enable event.

The exact digital states are frozen in `PHASE_5D_MODE_TRUTH_TABLE.csv` and become the direct input to the Phase 6 hardware-abstraction layer.

## Electrical-rules review result

The pin-by-pin audit is recorded in `PHASE_5D_ERC_AND_PIN_AUDIT.csv`. Fifty checks were completed. The outcomes are:

- one package error found and corrected: PB11 to PA8;
- no remaining GPIO conflicts;
- no unpowered or floating enable inputs;
- no missing MCU supply, ground, VBAT, VCAP, VDDA, reset, boot, or SWD connections;
- mux, op-amp, LDO, ESD, and USB-protection pinouts match their selected packages;
- all normal measurement currents remain resistor-limited;
- USB device-only routing is consistent with PA9/PA11/PA12;
- the analog and digital rails use one continuous ground plane;
- exposed measurement pins have ESD suppression and series isolation.

Two items cannot pass by inspection or LTspice and remain first-PCB tests:

1. AP2112 temperature and 3.3 V droop at the real TFT backlight current;
2. crystal startup margin with the fitted 30 pF load capacitors.

These are validation requirements, not unresolved schematic nets.

## PCB capture constraints carried forward

When this specification is entered into KiCad:

- place the OPA391, U_SENSE, ADC filter, reference resistors, and PA0 side of U1 as one compact analog cluster;
- keep USB and TFT traces out of the cluster;
- place TPD4E1B06 devices at the socket edge, not beside the mux;
- place USBLC6-2 at the USB connector and route D+/D− through it without stubs;
- keep HSE traces short, symmetric, and free of vias if possible;
- give every MCU VDD pin its own nearby 100 nF capacitor;
- connect the UFQFPN exposed pad to a solid ground area and thermal-via array;
- use one unbroken ground plane and control return-current paths through placement;
- label the sockets and safety warning on silkscreen.

Required safety text:

```text
UNPOWERED COMPONENTS ONLY
DISCHARGE CAPACITORS BEFORE INSERTION
R/C: 100R–1M / 100nF–100uF
BJT SOCKET: E B C — KNOWN PINOUT ONLY
```

## Phase 5D acceptance and Phase 6 handoff

Phase 5D passes. The hardware architecture is now frozen strongly enough to begin firmware without guessing pin assignments or measurement states.

Phase 6 should begin with the board-support package and safe-state driver:

1. define the corrected GPIO map, especially `SRC_A1 = PA8`;
2. implement a single transition function that disables both muxes and tri-states every DUT drive;
3. implement U_SRC and U_SENSE address helpers from the frozen truth table;
4. bring up calibration channels before enabling any component test;
5. add ADC settling, first-sample discard, timeout, and active-region gates;
6. only then add resistor, capacitor, diode, and BJT measurement algorithms.

## Official references

- STM32F401xB/xC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
- STM32F4 hardware design note AN4488: https://www.st.com/resource/en/application_note/dm00115714-getting-started-with-stm32f4xxxx-mcu-hardware-development-stmicroelectronics.pdf
- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- TMUX1108 datasheet: https://www.ti.com/lit/ds/symlink/tmux1108.pdf
- OPA391 datasheet: https://www.ti.com/lit/ds/symlink/opa391.pdf
- TPD4E1B06 datasheet: https://www.ti.com/lit/ds/symlink/tpd4e1b06.pdf
- AP2112 datasheet: https://www.diodes.com/assets/Datasheets/AP2112.pdf
- USBLC6-2 datasheet: https://www.st.com/resource/en/datasheet/usblc6-2.pdf
- GCT USB4105 product/drawing: https://gct.co/connector/usb4105
- Abracon ABLS-8.000MHZ-B2-T product data: https://abracon.com/parametric/crystals/ABLS-8.000MHZ-B2-T
