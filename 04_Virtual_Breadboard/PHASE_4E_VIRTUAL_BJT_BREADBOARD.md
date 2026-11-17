# SECDB — Phase 4E Virtual Breadboard: Known-Pinout BJT Integration

## Status

Phase 4E is complete at the virtual/LTspice level. Mirrored NPN and PNP bias paths were integrated with a known E-B-C test socket, sequential emitter/base/collector sensing, TI's OPA391 macro-model, PA0, calibrated path equations, active-region validation, and a mandatory base-off confirmation for suspected C-E shorts.

This is simulation verification, not physical validation. Version 1 still requires the user to select NPN or PNP and insert the emitter, base, and collector into the labelled terminals correctly.

## Virtual circuit

```text
PA1 / E_DRIVE -- 100 Ω -------- E socket
PA2 / B_DRIVE -- 1 MΩ, 0.1% --- B socket       known E-B-C insertion
PA3 / C_DRIVE -- 1 kΩ, 0.1% --- C socket
                                  | | |
                                  S1 S2 S3
                                TMUX1104 #4
                                     D
                                     |
                              OPA391 follower
                                     |
                                   100 Ω
                                     |
                                PA0 / ADC1_IN0
```

The approximately 50 Ω GPIO output resistance is included in each simulated path. The emitter voltage is measured instead of being assumed to equal ground or 3.3 V.

The three OPA391 instances per polarity in the gain deck represent the three sequential TMUX read states. The physical design continues to use one shared OPA391 and one PA0 input.

## Virtual parts added in Phase 4E

| Quantity | Part | Function |
|---:|---|---|
| 1 | TMUX1104DGS | Select emitter, base, or collector for ADC sensing |
| 1 | VSSOP-10 to DIP adapter | Required for a future solderless breadboard |
| 1 | 100 Ω resistor | Emitter-drive protection/current limiting |
| 1 | 1 MΩ, 0.1% resistor | Base-bias path |
| 1 | 1 kΩ, 0.1% resistor | Collector-load path |
| 1 | 100 nF ceramic capacitor | TMUX1104 local supply decoupling |
| 1 | 100 kΩ resistor | Pull TMUX `EN` low during reset |
| 1 | labelled E-B-C transistor socket | Known-pinout insertion |

The Phase 4A OPA391, its decoupling capacitor, the 100 Ω PA0 isolation resistor, and the shared 3.3 V/GND rails are reused.

## GPIO allocation and effective paths

| STM32 pin | External connection | Firmware role |
|---|---|---|
| PA1 | 100 Ω to E terminal | `E_DRIVE`: high, low, or analog/high-impedance |
| PA2 | nominal 1 MΩ, 0.1% to B terminal | `B_DRIVE`: high, low, or analog/high-impedance |
| PA3 | nominal 1 kΩ, 0.1% to C terminal | `C_DRIVE`: high, low, or analog/high-impedance |
| PA0 | OPA391 output through 100 Ω | Shared ADC1_IN0 input |

The provisional simulated complete paths are:

```text
REMIT = 100 Ω + 50 Ω GPIO resistance = 150 Ω
RBASE = 1.001 MΩ modeled tolerance corner + 50 Ω GPIO = 1.00105 MΩ
RCOL  = 1.001 kΩ modeled tolerance corner + 50 Ω GPIO = 1.051 kΩ
```

These are simulation values. Physical calibration must characterize the complete base and collector paths and the actual drive-high voltage before the equations are frozen.

## BJT sense-multiplexer connections

> Phase 5A supersession: the separate BJT sense TMUX1104 is replaced by channels S4/S5/S6 of the universal TMUX1108. Use the Phase 5 master architecture for later schematic and firmware work.

> Phase 5D package correction: the PB11 entries below apply only to the provisional Black Pill/Phase 4 wiring. PB11 is not bonded out on the final STM32F401CCU6 UFQFPN48 package. The final shared `U_SENSE` address pins are PB13, PB14, and PB15; use the Phase 5D master netlist and truth table for PCB and firmware work.

The table applies to the TMUX1104DGS 10-pin VSSOP package.

| Pin | Name | Virtual connection |
|---:|---|---|
| 1 | A0 | Black Pill PB10 |
| 2 | S1 | E terminal |
| 3 | GND | Ground rail |
| 4 | S3 | C terminal |
| 5 | EN | Black Pill PB15 and 100 kΩ pull-down to ground |
| 6 | VDD | 3.3 V rail |
| 7 | S4 | Leave unconnected in Version 1 |
| 8 | D | Shared OPA391 `+IN` measurement node |
| 9 | S2 | B terminal |
| 10 | A1 | Black Pill PB11 |

Place 100 nF directly between pins 6 and 3.

## Sense truth table

| PB15/EN | PB11/A1 | PB10/A0 | ADC source |
|---:|---:|---:|---|
| 0 | X | X | No BJT terminal selected |
| 1 | 0 | 0 | S1: emitter |
| 1 | 0 | 1 | S2: base |
| 1 | 1 | 0 | S3: collector |
| 1 | 1 | 1 | S4 unused; firmware must avoid |

Every other measurement multiplexer must remain disabled throughout the BJT test.

## Drive states

| State | PA1 / emitter | PA2 / base | PA3 / collector |
|---|---|---|---|
| Inactive/changeover | Analog input, no pull | Analog input, no pull | Analog input, no pull |
| NPN active test | Low | High through 1 MΩ | High through 1 kΩ |
| NPN base-off confirmation | Low | Low through 1 MΩ | High through 1 kΩ |
| PNP active test | High | Low through 1 MΩ | Low through 1 kΩ |
| PNP base-off confirmation | High | High through 1 MΩ | Low through 1 kΩ |

For an active test, configure the emitter drive first, the collector drive second, and the base drive last. Remove base drive first when leaving the state. The sense TMUX remains disabled during every transition.

## Calibrated equations

All three terminal voltages must be measured. For NPN:

```text
VBE = VB - VE
VCE = VC - VE
VCB = VC - VB
IB  = (VDRIVE_HIGH - VB) / RBASE
IC  = (VDRIVE_HIGH - VC) / RCOL
hFE = IC / IB
```

For PNP:

```text
VEB = VE - VB
VEC = VE - VC
VBC = VB - VC
IB  = (VB - VDRIVE_LOW) / RBASE
IC  = (VC - VDRIVE_LOW) / RCOL
hFE = IC / IB
```

`VDRIVE_HIGH`, `VDRIVE_LOW`, `RBASE`, and `RCOL` must use calibrated values. Assuming an ideal 3.300 V source and printed resistor values is not the final calculation method.

## LTspice gain integration

Main file: `Virtual_Breadboard_BJT_Integration.cir`

It includes:

- matched custom NPN and PNP models stepped through nominal `BF` 20, 30, 50, 100, 200, 300, and 500;
- the complete provisional GPIO and resistor paths;
- a 9.5 Ω TMUX sensing path;
- TI OPAx391 PSpice macro-model Rev. B for E, B, and C readback;
- 100 Ω OPA391-to-PA0 isolation and 7 pF ADC loading;
- a conservative ±3.973 mV per-node ADC/buffer allocation.

Every step completed without convergence errors.

## Verified NPN gain results

| Nominal BF | Estimated hFE | VBE | VCE | VCB | Conservative hFE interval |
|---:|---:|---:|---:|---:|---:|
| 20 | 20.527 | 0.5837 V | 3.2329 V | 2.6492 V | 19.102–21.957 |
| 30 | 30.780 | 0.5940 V | 3.2002 V | 2.6062 V | 29.331–32.232 |
| 50 | 51.262 | 0.6069 V | 3.1353 V | 2.5284 V | 49.772–52.757 |
| 100 | 102.350 | 0.6244 V | 2.9761 V | 2.3517 V | 100.762–103.943 |
| 200 | 204.052 | 0.6416 V | 2.6682 V | 2.0266 V | 202.273–205.836 |
| 300 | 305.162 | 0.6516 V | 2.3725 V | 1.7210 V | 303.192–307.138 |
| 500 | 505.745 | 0.6639 V | 1.8130 V | 1.1492 V | 503.385–508.114 |

All points satisfy the active-region voltage rules. The hFE≈20 lower endpoint has the largest electronic interval, approximately ±7%, and remains inside the ±10% indicative target.

## Verified PNP gain results

The mirrored PNP circuit produced hFE estimates from 20.522 to 505.743 over the same sweep. Its `VEB`, `VEC`, and `VBC` values mirror the NPN results within the modeled buffer differences, and all points remain active.

This agreement verifies the equations and routing symmetry; it does not imply that real NPN and PNP devices have matched gain.

## Real-model temperature results

Fault/real-model file: `Virtual_Breadboard_BJT_Faults.cir`

| Device | -10 °C hFE | 25 °C hFE | 60 °C hFE |
|---|---:|---:|---:|
| 2N3904 | 250.42 | 301.58 | 355.66 |
| 2N3906 | 167.44 | 201.77 | 238.11 |

For the 2N3904 model, VBE changed from 0.7092 V at -10 °C to 0.5933 V at 60 °C. This change is much larger than the electronics uncertainty. Therefore, hFE is reported only as an indicative value at the SECDB test conditions; it is not a universal health score.

## Frozen active-region checks

Accept an NPN hFE result only if all conditions pass:

```text
0.45 V <= VBE <= 0.85 V
VCE > 0.30 V
VCB = VC - VB > 0.20 V
calculated hFE >= 20 with adequate ADC margin
```

For PNP:

```text
0.45 V <= VEB <= 0.85 V
VEC > 0.30 V
VBC = VB - VC > 0.20 V
calculated hFE >= 20 with adequate ADC margin
```

If any condition fails, firmware must not display `IC/IB` as a valid hFE value.

## Verified fault signatures at 25 °C

The fault deck uses behavioral unity readback through the same 9.5 Ω TMUX and 100 Ω/7 pF ADC structure. The vendor OPA391 model is already exercised in the gain deck; behavioral readback keeps the large fault matrix deterministic at rail extremes.

| Condition | Important simulated readings | Interpretation |
|---|---|---|
| Healthy 2N3904 | VBE=0.6515 V, VCE=2.3829 V, VCB=1.7314 V, hFE=301.58 | Active; indicative hFE valid |
| Open base | external VBE≈3.3000 V, IC=1.63 nA | Base connection absent/open |
| Open collector | VBE=0.5480 V, IC≈45 pA | Base junction responds; collector path absent |
| C-E short | VCE=27.51 µV, IC=2.747 mA | C-E short or saturation candidate |
| B-E short | VBE≈32.95 nV, IC≈45 pA | B-E short candidate |
| B-C short | VBE≈VCE≈0.6792 V, VCB≈0 V | Fails active-region VCB rule |
| Excessive 10 kΩ base drive | VCE=0.0401 V, VCB=-0.6571 V, ratio=12.46 | Tester-induced saturation; ratio is not hFE |

## Mandatory base-off confirmation

A low VCE reading cannot independently distinguish saturation from a physical C-E short. The base-off repeat produced:

| Base-off case | VCE | Collector current | Decision |
|---|---:|---:|---|
| Healthy 2N3904 | 3.3000 V | approximately 45 pA | Transistor turns off |
| 10 mΩ C-E short | 27.48 µV | 2.7477 mA | Short remains after base drive is removed |

Firmware may report `C_E_SHORT_CANDIDATE` only when low VCE/high collector current persists during the base-off confirmation.

## Firmware sequence

1. Ask the user to select NPN or PNP and confirm known E-B-C insertion.
2. Disable every measurement TMUX and configure PA1/PA2/PA3 as analog inputs with no pulls.
3. Apply the selected NPN or PNP drive state, with base drive enabled last.
4. Wait for settling.
5. Sequentially select E, B, and C on the BJT sense TMUX. For each node, discard the first ADC conversion and average the reading.
6. Disable the sense TMUX.
7. Remove base drive first, then return all drive pins to analog/no-pull mode.
8. Calculate junction voltages and currents using calibrated drive levels and path resistances.
9. Apply every active-region validity check before calculating or displaying hFE.
10. If VCE/VEC is abnormally low, run the corresponding base-off confirmation and reread E and C.
11. Report polarity, junction voltage, collector current, and `indicative hFE at SECDB test current`, or an honest invalid/fault candidate message.

## Diagnostic wording

Allowed examples:

```text
NPN junction detected
VBE: 0.65 V
Collector current: 0.77 mA
Indicative hFE: 305
Active-region check: passed
```

```text
Invalid BJT measurement
Possible wrong E-B-C insertion, open connection, saturation, or damaged device
No valid hFE reported
```

Do not label a transistor degraded solely because its hFE differs from another device. A part-specific expected range or known-good comparison is required.

## Safety and limitations

- Insert only an unpowered, discharged transistor.
- Version 1 does not automatically identify E, B, C, NPN, or PNP.
- The test does not determine voltage, current, frequency, noise, or power ratings.
- hFE varies strongly with part, current, temperature, and individual sample.
- A wrong pin assignment may produce an invalid signature; firmware must not force a health label.
- GPIO resistance, drive levels, analog-switch leakage, socket contacts, and real ADC performance still require physical validation.

Official component references:

- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- OPA391 product/model page: https://www.ti.com/product/OPA391
- STM32F401CC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf

## Phase 4E conclusion

The known-pinout BJT subsystem is ready for virtual acceptance. NPN/PNP bias, E/B/C readback, hFE≈20–500, electronics uncertainty, active-region validity, temperature behavior, terminal faults, and base-off confirmation are defined and numerically verified.

Phase 4 virtual breadboard integration is now complete. The next stage is Phase 5: consolidate the provisional resistor, capacitor, diode, and BJT networks into one shared analog-front-end schematic, audit GPIO and switch resources, and remove avoidable duplicated circuitry before firmware implementation and PCB design.
