# SECDB — Phase 5A Shared Architecture, GPIO, and Switch Audit

## Status and decision

Phase 5A is complete. The separate Phase 4 virtual circuits have been consolidated into a candidate shared analog-front-end architecture and every required STM32 signal has been assigned without overlap.

The main consolidation decision is:

```text
Phase 4 provisional design: four TMUX1104 devices
Phase 5 candidate design:   one TMUX1104 source mux
                            one TMUX1108 8:1 sense mux
                            one shared OPA391
                            one shared PA0 ADC channel
```

This reduces switch count and duplicated resistors while preserving every verified measurement equation. It is the architecture selected for the Phase 5B interaction simulation; it is not yet the final PCB freeze.

## Phase 5 work breakdown

| Step | Work | Completion gate |
|---|---|---|
| 5A | Shared architecture, GPIO, switch, socket, and startup-state audit | Complete in this document |
| 5B | Consolidated LTspice interaction simulation | All modes retain Phase 3/4 accuracy and fault separation with inactive networks present |
| 5C | Protection, power/decoupling, calibration, and component-value freeze | No unsafe test-node path; complete preliminary BOM |
| 5D | Master schematic specification and design review | All nets, pins, mode states, and acceptance rules frozen for firmware and KiCad |

## Consolidated system architecture

```text
USB-C power/protection --> AP2112K-3.3 --> 3V3_A/D rails
                                           |
                                  STM32F401CCU6
                                   |     |     |
                                   |     |     +--> ST7735 display
                                   |     +--------> drive GPIOs
                                   +--------------> mux-control GPIOs

3V3/GND --> TMUX1104 source mux --> R/C socket node

R/C node ---------+
Diode A/B ---------+--> TMUX1108 8:1 sense mux --> OPA391 --> 100 Ω --> PA0
BJT E/B/C ---------+
1.1/2.2 V cal -----+
```

Three labelled socket groups are retained because they make the Version 1 user procedure and protection clearer:

- two pins for resistor/capacitor;
- two pins for diode;
- three labelled pins for known E-B-C BJT insertion.

They may later be implemented as separate headers or as labelled regions of one multi-position ZIF socket. They are electrically separate nets.

## U_SRC — shared resistor/capacitor source multiplexer

Retain one TMUX1104DGS. Its common `D` pin connects to `RC_SENSE`, the upper terminal of the resistor/capacitor socket. The other socket terminal is ground.

| TMUX1104 channel | Connection | Use |
|---|---|---|
| S1 | 3.3 V through nominal 1 kΩ, 0.1% | Resistor low range |
| S2 | 3.3 V through nominal 10 kΩ, 0.1% | Resistor middle range and capacitor charge |
| S3 | 3.3 V through nominal 100 kΩ, 0.1% | Resistor high range |
| S4 | Ground through nominal 1 kΩ, 0.1% | Capacitor controlled discharge |
| D | `RC_SENSE` | Shared R/C test node |

This reuses the verified 10 kΩ resistor path for capacitor charging and removes the separate Phase 4C capacitor TMUX and duplicate 10 kΩ charging resistor.

### U_SRC control

| Signal | STM32 pin |
|---|---|
| `SRC_A0` | PB10 |
| `SRC_A1` | PA8 |
| `SRC_EN` | PB12 |

| EN | A1:A0 | Selected operation |
|---:|---:|---|
| 0 | XX | All R/C source paths disconnected |
| 1 | 00 | 1 kΩ resistor range |
| 1 | 01 | 10 kΩ resistor range or capacitor charge |
| 1 | 10 | 100 kΩ resistor range |
| 1 | 11 | 1 kΩ capacitor discharge |

## U_SENSE — universal ADC input multiplexer

Replace the separate Phase 4 diode and BJT sense TMUX1104 devices with one TMUX1108PWR 8:1 low-leakage multiplexer. The PWR 16-pin TSSOP package is preferred over the small QFN option for easier inspection and assembly.

Its common `D` pin feeds OPA391 `+IN`. The OPA391 remains a unity-gain follower, followed by the verified 100 Ω PA0 isolation resistor.

| TMUX1108 channel | Signal |
|---|---|
| S1 | `RC_SENSE` |
| S2 | `DIODE_A` |
| S3 | `DIODE_B` |
| S4 | `BJT_E` |
| S5 | `BJT_B` |
| S6 | `BJT_C` |
| S7 | `CAL_1V1` |
| S8 | `CAL_2V2` |

The two calibration voltages come from the already verified three-resistor 10 kΩ ladder.

### U_SENSE control

| Signal | STM32 pin |
|---|---|
| `SNS_EN` | PB9 |
| `SNS_A0` | PB13 |
| `SNS_A1` | PB14 |
| `SNS_A2` | PB15 |

| EN | A2:A1:A0 | ADC source |
|---:|---:|---|
| 0 | XXX | No input selected |
| 1 | 000 | R/C node |
| 1 | 001 | Diode terminal A |
| 1 | 010 | Diode terminal B |
| 1 | 011 | BJT emitter |
| 1 | 100 | BJT base |
| 1 | 101 | BJT collector |
| 1 | 110 | 1.1 V calibration tap |
| 1 | 111 | 2.2 V calibration tap |

At 3.3 V, the TMUX1108 datasheet specifies a maximum on-resistance of 8.75 Ω at 25 °C and 9.5 Ω through 85 °C. Because it drives the ultra-low-bias OPA391 input, this resistance produces negligible DC drop. Its typical on-capacitance is about 65 pF and must be included in the Phase 5B timing/settling simulation.

## STM32F401CCU6 GPIO allocation

The final target part number `STM32F401CCU6` is a **UFQFPN48**, 7 mm × 7 mm, 0.5 mm-pitch device—not an LQFP48. The underside exposed pad must be soldered to PCB ground. The Black Pill remains only a prototype placeholder.

### Measurement and display pins

| MCU pin | Final Phase 5 candidate function | Status |
|---|---|---|
| PA0 | `ADC_BUFFERED`, ADC1_IN0 | Fixed |
| PA1 | `BJT_E_DRIVE` through 100 Ω | Retained from Phase 4E |
| PA2 | `BJT_B_DRIVE` through 1 MΩ | Retained from Phase 4E |
| PA3 | `BJT_C_DRIVE` through 1 kΩ | Retained from Phase 4E |
| PA4 | TFT chip select | Fixed |
| PA5 | SPI1 clock to TFT | Fixed |
| PA6 | `DIODE_A_DRIVE` through 1 kΩ | Reassigned in Phase 5A |
| PA7 | SPI1 MOSI to TFT | Fixed |
| PA9 | USB OTG FS VBUS sense | Reserved for USB |
| PA10 | `DIODE_B_DRIVE` through 1 kΩ | Reassigned in Phase 5A |
| PA11 | USB OTG FS DM | Reserved for USB |
| PA12 | USB OTG FS DP | Reserved for USB |
| PA13 | SWDIO | Fixed for programming/debug |
| PA14 | SWCLK | Fixed for programming/debug |
| PB0 | TFT reset | Fixed |
| PB1 | TFT data/command | Fixed |
| PB8 | Status LED, through resistor | Candidate |
| PB9 | `SNS_EN` | Fixed for Phase 5B |
| PB10 | `SRC_A0` | Fixed for Phase 5B |
| PA8 | `SRC_A1` | Corrected in Phase 5D; PB11 is not bonded out on UFQFPN48 |
| PB12 | `SRC_EN` | Fixed for Phase 5B |
| PB13 | `SNS_A0` | Fixed for Phase 5B |
| PB14 | `SNS_A1` | Fixed for Phase 5B |
| PB15 | `SNS_A2` | Fixed for Phase 5B |
| PC13 | UI button 1, active low | Candidate |
| PC14 | UI button 2, active low | Candidate; requires no LSE crystal |
| PC15 | UI button 3, active low | Candidate; requires no LSE crystal |

PA9 was provisionally used as a diode drive in Phase 4D. That assignment is superseded because PA9 is the STM32F401 USB VBUS-sense input. The Phase 5 diode pins are PA6 and PA10.

### Reserved and spare pins

| Pins | Purpose/status |
|---|---|
| PH0/PH1 | Reserve for HSE crystal/clock source |
| NRST | Reset circuit and SWD header |
| BOOT0 | Defined boot pull-down and optional service jumper/button |
| VBAT | Tie according to the final no-battery power scheme |
| PA15, PB2–PB7 | Spare GPIO after the present allocation |
| PC14/PC15 | Not available for an LSE crystal while used as buttons |

The internal timer used for capacitor timing does not require an external timer pin.

## GPIO audit result

There are no remaining overlaps in the Phase 5 candidate map:

- SPI1 display signals remain on PA4/PA5/PA7 plus PB0/PB1;
- PA0 is the only analog measurement ADC input;
- USB keeps PA9/PA11/PA12;
- SWD keeps PA13/PA14;
- BJT drives use PA1/PA2/PA3;
- diode drives use PA6/PA10;
- six multiplexer controls use PB9, PB10, and PB12–PB15; `SRC_A1` uses PA8;
- UI buttons use PC13/PC14/PC15 if no LSE crystal is fitted;
- several GPIOs remain available for revision changes.

## Safe startup and switching state

Hardware defaults:

1. Add a 100 kΩ pull-down to `SRC_EN` and `SNS_EN` so both multiplexers are off during reset.
2. Add 100 kΩ pull-downs to all mux address lines so the disabled address state is defined.
3. STM32 measurement-drive pins PA1, PA2, PA3, PA6, and PA10 start as analog/no-pull inputs.
4. Do not sample PA0 while the sense multiplexer is disabled because the OPA391 input is intentionally not loaded by a low-value bias resistor.
5. After initialization, the safe idle read state may select `CAL_1V1`; all source and DUT-drive outputs remain inactive.

Mandatory mode-transition sequence:

```text
disable U_SENSE
disable U_SRC
set every DUT drive GPIO to analog/no-pull
wait for break-before-make and node discharge as required
configure the selected source/drive state
select the requested U_SENSE input
enable U_SENSE
wait for settling; discard first ADC conversion
measure
disable U_SENSE
remove DUT drive/source state
return to safe idle
```

## Mode ownership table

| Mode | U_SRC | U_SENSE channels | Active drive pins |
|---|---|---|---|
| Idle/self-test | Disabled | S7 or S8 calibration | None |
| Resistor | S1, S2, or S3 | S1 `RC_SENSE` | None |
| Capacitor discharge | S4 | S1 `RC_SENSE` | None |
| Capacitor charge | S2 | S1 `RC_SENSE` | None |
| Diode A-to-B/B-to-A | Disabled | S2 and S3 sequentially | PA6 and PA10 |
| NPN BJT | Disabled | S4, S5, S6 sequentially | PA1, PA2, PA3 |
| PNP BJT | Disabled | S4, S5, S6 sequentially | PA1, PA2, PA3 |

Only one component mode may own the front end at a time.

## Current-limiting audit

The candidate architecture preserves the bounded Phase 4 test currents:

| Condition | Approximate modeled maximum |
|---|---:|
| R/C node short on 1 kΩ source range | 3.27 mA |
| 100 µF discharge from 3.3 V through 1 kΩ | 3.27 mA initial |
| Diode-socket short through two protected GPIO legs | 1.57 mA |
| BJT C-E short with current paths active | 2.75 mA |

These figures are circuit-level simulation values. The Phase 5C protection review must still verify MCU pin-current limits, connector misuse, ESD, and externally charged-component cases.

## Resource reduction

| Resource | Phase 4 provisional total | Phase 5A candidate total |
|---|---:|---:|
| TMUX1104 | 4 | 1 |
| TMUX1108 | 0 | 1 |
| OPA391 | 1 shared conceptually | 1 physical |
| 10 kΩ R/C charge/reference paths | 2 | 1 shared |
| Mux-control GPIOs | 12 provisional | 7 |
| MCU ADC inputs | 1 | 1 |

The reduction does not yet count protection components, supply decoupling, test points, or socket connectors.

## Risks carried into Phase 5B/5C

1. TMUX1108 on-capacitance increases the capacitor timing-node parasitic above the earlier 50 pF placeholder.
2. Switching among rail-level and high-impedance nodes requires a measured settling delay and a discarded first ADC conversion.
3. Off-channel leakage and PCB contamination must be checked against the 1 MΩ resistor and BJT base paths.
4. The shared R/C node must not retain charge when resistor mode begins.
5. Separate socket nets simplify switching but require unmistakable silkscreen and UI instructions.
6. The STM32F401CCU6 UFQFPN48 package requires reflow-capable assembly and an exposed-pad ground layout.
7. Input protection must not add enough leakage or capacitance to invalidate the upper resistor range or lower capacitor range.
8. Physical validation is still mandatory even after the consolidated LTspice simulations pass.

## Phase 5A conclusion

> Phase 5D correction: the original audit assigned `SRC_A1` to PB11, but PB11 is not bonded out on the STM32F401CCU6 UFQFPN48 package. `SRC_A1` is reassigned to available pin PA8. This is a control-only correction and does not alter any Phase 5B/5C analog simulation result.

The Phase 5 candidate architecture is internally consistent and has sufficient STM32 resources. The former four-switch plan is replaced by a TMUX1104 source mux plus a TMUX1108 universal sense mux, one OPA391, and PA0. The PA9 USB conflict has been removed, safe reset behavior is defined, and all component modes have exclusive ownership rules.

Phase 5B consolidated interaction simulation has now been completed and accepted separately. The next step is Phase 5C input protection, power/decoupling, calibration-component, and preliminary BOM freeze.

Official references:

- STM32F401xB/xC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
- STM32F401CC product/package page: https://www.st.com/en/microcontrollers-microprocessors/stm32f401cc.html
- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- TMUX1108 datasheet: https://www.ti.com/lit/ds/symlink/tmux1108.pdf
- OPA391 product/model page: https://www.ti.com/product/OPA391
