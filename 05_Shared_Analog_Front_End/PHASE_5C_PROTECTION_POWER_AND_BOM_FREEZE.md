# SECDB — Phase 5C Protection, Power, Calibration, and Preliminary BOM Freeze

## Status

Phase 5C is complete at the simulation/design-review level. The shared analog front end now has a frozen first-pass protection network, a complete USB-C-to-3.3 V power and decoupling plan, frozen calibration components, and a preliminary BOM.

This is not a hardware safety certification. LTspice verifies measurement loading and settling; physical PCB tests must verify ESD behavior, rail noise, regulator temperature, USB operation, and misuse survivability.

## 1. Frozen test-terminal protection

There are six exposed non-ground measurement terminals: `RC_SOCKET`, `DIODE_A`, `DIODE_B`, `BJT_E`, `BJT_B`, and `BJT_C`. Two TPD4E1B06DCKR arrays provide eight channels, so six are used and two are spare.

The selected array has the required precision-node properties:

- maximum leakage: 0.5 nA at 2.5 V and 25 °C;
- maximum line capacitance: 0.95 pF;
- bidirectional working range: -5.5 V to +5.5 V;
- IEC 61000-4-2 rating: ±12 kV contact and ±15 kV air-gap.

Place each ESD array beside its socket, before long traces. Its ground pin needs a very short, wide route and nearby ground via(s). Do not route the ESD return through the analog ground path.

### Series isolation

Every one of the eight U_SENSE source branches receives a 1 kΩ series resistor before the TMUX1108. Because the OPA391 input is high impedance, this resistor does not carry the DUT excitation current and produces only microvolt-scale error from modeled leakage. With about 100 pF at the mux input, its first-order RC contribution is about 0.1 µs.

The R/C socket also receives a calibrated 330 Ω guard resistor between `RC_SOCKET` and the U_SRC common pin. This is the only exposed node that otherwise touched an analog-switch pin directly. The resistor is part of the R/C excitation path and therefore must be included in its calibrated path constant:

```text
RPATH_1K   = measured 1 kΩ reference path + U_SRC Ron + R_RC_GUARD
RPATH_10K  = measured 10 kΩ reference path + U_SRC Ron + R_RC_GUARD
RPATH_100K = measured 100 kΩ reference path + U_SRC Ron + R_RC_GUARD
```

For capacitor mode:

```text
C = t / [-(RCHARGE_PATH) ln(1 - ADC_threshold/4095)]
RCHARGE_PATH = measured 10 kΩ path + U_SRC Ron + 330 Ω guard
```

The same guard increases the 100 µF discharge-to-1% result to approximately 0.617 s, still well below the frozen 5 s timeout.

### Non-negotiable safety boundary

TPD4E1B06 is an ESD suppressor, not a low-voltage DC overvoltage disconnect. Its ESD clamp voltage is above the normal 3.3 V analog range, and TMUX1108 analog pins are specified only from approximately VSS - 0.5 V to VDD + 0.5 V at absolute maximum.

Therefore Version 1 accepts only loose, unpowered, discharged components. The PCB silkscreen and firmware start screen must say:

```text
UNPOWERED COMPONENTS ONLY
DISCHARGE CAPACITORS BEFORE INSERTION
MAX NORMAL TEST VOLTAGE: 3.3 V
```

Do not connect a powered circuit, battery, charged capacitor, mains-referenced part, or in-circuit component. Protection against those cases would require a fault-protected analog front end and is outside the Version 1 scope.

## 2. Frozen ADC-side filter

The shared buffer output is:

```text
TMUX1108 D -> OPA391 voltage follower -> 100 Ω -> PA0/ADC1_IN0
                                                   |
                                                  1 nF
                                                   |
                                                  GND
```

The 100 Ω resistor isolates the OPA391 from the 1 nF load and the STM32 sample-and-hold kickback. The official OPA391 macro-model remains stable in the Phase 5C decks. Firmware retains the 200 µs post-selection wait and discards the first ADC conversion.

Capacitor timing is still the special case: U_SENSE must already be selected and settled before U_SRC charge begins; the timer starts with the charge command, so no extra 200 µs delay is inserted after charging starts.

## 3. USB-C and 3.3 V power tree

The frozen power order is:

```text
USB-C VBUS
  -> MF-MSMF050-2 0.50 A PPTC
  -> SMBJ5.0A-TR TVS to ground
  -> B5819W series Schottky
  -> BLM21PG221SN1D ferrite bead
  -> 10 µF || 100 nF
  -> AP2112K-3.3TRG1
  -> 10 µF || 100 nF
  -> 3V3_D
```

USB-C configuration and data:

- CC1 and CC2 each use 5.1 kΩ, 1% to ground;
- D+ and D- route as a matched USB 2.0 pair through USBLC6-2SC6 near the connector to PA12 and PA11;
- PA9 senses fused VBUS through 1 kΩ and uses no pull-up/pull-down;
- D+ and D- each include a 22 Ω series resistor close to the MCU;
- connector shield connects to ground according to the PCB EMI plan, with short ESD return paths;
- the AP2112 EN pin is tied to its protected input rail for always-on operation.

The PPTC rating is a fault limit, not the board's thermal design current. Keep the first PCB's expected continuous load at or below 150 mA until AP2112 junction temperature and 3.3 V droop are measured with the actual display backlight.

## 4. Digital and analog rails

`3V3_D` powers MCU VDD, USB logic, the display, and digital loads. A second BLM21PG221SN1D bead creates `3V3_A` from `3V3_D`. Its maximum 0.045 Ω DCR gives negligible DC drop at the analog-front-end current while attenuating high-frequency digital noise.

`3V3_A` powers:

- STM32 VDDA/VREF+;
- TMUX1104 and TMUX1108 supplies;
- OPA391;
- resistor source paths and the calibration ladder.

Use one continuous ground plane. Do not split analog and digital ground planes. Control noise through placement and return-current routing: keep TFT/USB currents away from the socket, reference resistors, OPA391, and PA0 region.

## 5. Decoupling freeze

The following connections are mandatory in the master schematic:

| Load | Frozen local decoupling |
|---|---|
| AP2112 input | 10 µF + 100 nF |
| AP2112 output | 10 µF + 100 nF |
| STM32 VDD pins | one 100 nF at each VDD pin + one 10 µF package bulk capacitor |
| STM32 VBAT | tie to VDD and add 100 nF because no battery is used |
| STM32 VDDA/VREF+ | 1 µF + 100 nF on `3V3_A` |
| STM32 VCAP_1 | one 4.7 µF low-ESR ceramic, directly to ground |
| TMUX1104 | 100 nF at VDD |
| TMUX1108 | 100 nF at VDD |
| OPA391 | 100 nF at V+ plus shared 1 µF local analog bulk |
| ST7735 connector | 10 µF + 100 nF |

The STM32F4 hardware guidance specifies at least one 4.7 µF package bulk capacitor, one 100 nF per VDD pin, 100 nF + 1 µF at VDDA, and a single 4.7 µF low-ESR VCAP capacitor for packages with only one VCAP pin.

## 6. Calibration-component freeze

- Range references: 1 kΩ, 10 kΩ, and 100 kΩ, each 0.1%, 25 ppm/°C thin film.
- Calibration ladder: three equal 10 kΩ, 0.1%, 25 ppm/°C resistors, preferably from the same series or network lot.
- R/C guard: 330 Ω, 0.1%, 25 ppm/°C.
- Diode current-limit resistors: two 1 kΩ, 0.1%.
- BJT base/collector/emitter paths: 1 MΩ, 1 kΩ, and 100 Ω respectively.
- Store ADC offset/gain calibration separately from each effective source-path resistance.

The 1/3 and 2/3 calibration taps are ratiometric to `3V3_A`; they check the ADC/buffer chain but do not replace source-path resistance calibration.

## 7. LTspice regression results

The protection network was added to new Phase 5C decks using the official OPA391 macro-model. TPD4E1B06 was represented by its datasheet maximum 0.5 nA leakage and 0.95 pF capacitance because no vendor transient macro-model was used.

### Resistor mode

Nineteen values from 100 Ω through 1 MΩ were rerun with the calibrated 330 Ω guard, 1 kΩ sense isolation, 2 nA mux allocation, 0.5 nA ESD leakage, 1 nF ADC filter, and the established ADC/buffer error allocation.

- conservative envelope: -1.9188% to +1.9044%;
- target: ±2%;
- result: pass, with the 100 Ω endpoint now the tightest margin;
- nominal 100 Ω ADC voltage: 0.2291 V, about code 284 on a 12-bit 3.3 V ADC.

The 100 Ω endpoint must be a mandatory hardware acceptance test because its simulated margin is only about 0.08 percentage point on the low side.

### Capacitor mode

| DUT | Protected-path time to code 2589 | Estimated capacitance | Error |
|---:|---:|---:|---:|
| 100 nF | 1.035937 ms | 100.0644 nF | +0.0644% |
| 1 µF | 10.353623 ms | 1.000089 µF | +0.00888% |
| 10 µF | 103.530394 ms | 10.000324 µF | +0.00324% |
| 100 µF | 1.035298 s | 100.002695 µF | +0.00270% |

These numbers isolate electronic-path effects; real capacitor tolerance, dielectric absorption, ESR, and leakage will dominate for many parts. ESR is still not claimed.

### Diode and BJT active modes

- 1N4148: VF = 0.600193 V at 1.28440 mA;
- 2N3904: VBE = 0.651442 V, VCE = 2.383753 V, modeled hFE = 301.29;
- 2N3906: VEB = 0.641506 V, VEC = 2.674705 V, modeled hFE = 201.88.

All remain in the same valid classes as Phase 5B. BJT hFE remains indicative and active-region gating remains mandatory.

### Settling

The protected 100 kΩ-range/1 MΩ-DUT path reaches within 1 mV at:

- R/C socket node: approximately 69.39 µs;
- PA0 after OPA391 and the 100 Ω/1 nF filter: approximately 147.10 µs.

At 200 µs the modeled PA0 error is approximately 0.057 mV. The frozen 200 µs wait plus discarded first conversion therefore remains valid in simulation, but must be checked on hardware.

## 8. Preliminary BOM and items intentionally left preliminary

The detailed BOM is in `PHASE_5C_PRELIMINARY_BOM.csv`. Analog ICs, protection parts, power parts, precision resistor values, and decoupling values are frozen for Phase 5D.

The following remain preliminary because they depend on mechanical placement or the exact purchased module:

- test-socket/ZIF mechanical choice;
- ST7735 connector pin order;
- tactile-switch and header footprints.

Phase 5D subsequently freezes the USB-C receptacle as GCT USB4105-GF-A and the HSE network as ABLS-8.000MHZ-B2-T with two 30 pF C0G load capacitors plus a fitted 0 Ω series-tuning link. The load value remains a hardware-startup verification item.

These are not measurement-architecture changes, but Phase 5D must resolve them before a PCB can be released.

## Acceptance decision

Phase 5C passes for schematic integration. The protection loading stays inside the frozen measurement goals, the 200 µs delay remains adequate in simulation, power and decoupling are fully specified, and a preliminary BOM exists.

The next step is Phase 5D: write the master schematic/net specification, perform a pin-by-pin electrical-rules review, resolve the preliminary connector/crystal footprints, and freeze the design for Phase 6 firmware and later KiCad capture.

## Official references

- TI TPD4E1B06 datasheet: https://www.ti.com/lit/ds/symlink/tpd4e1b06.pdf
- TI TMUX1108 datasheet: https://www.ti.com/lit/ds/symlink/tmux1108.pdf
- TI OPA391 datasheet/model page: https://www.ti.com/lit/ds/symlink/opa391.pdf
- ST STM32F401xB/xC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
- ST STM32F4 hardware-development note AN4488: https://www.st.com/resource/en/application_note/dm00115714-getting-started-with-stm32f4xxxx-mcu-hardware-development-stmicroelectronics.pdf
- Diodes Incorporated AP2112 datasheet: https://www.diodes.com/assets/Datasheets/AP2112.pdf
- ST USBLC6-2 datasheet: https://www.st.com/resource/en/datasheet/usblc6-2.pdf
- Bourns MF-MSMF PPTC datasheet: https://www.bourns.com/docs/product-datasheets/mf-msmf.pdf
- Murata BLM21PG221SN1D data: https://www.murata.com/en-eu/api/pdfdownloadapi?cate=cgsubChipFerriBead&partno=BLM21PG221SN1D
