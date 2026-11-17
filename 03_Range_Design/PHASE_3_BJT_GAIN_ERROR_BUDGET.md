# SECDB — Phase 3D BJT Gain Range and Error Budget

## Status

Phase 3D is complete at the LTspice design level. The proposed known-pin NPN/PNP test was verified over an approximate hFE range of 20–500, with nonideal resistor/switch paths, conservative ADC error, temperature variation, ADC input loading, active-region rejection, and representative terminal faults.

Version 1 still requires the user to insert the emitter, base, and collector correctly. Automatic pin identification remains outside scope.

## Test topology and equations

The modeled effective paths are:

```text
RC path = 1 kΩ at +0.1% + 9.5 Ω switch resistance = 1010.5 Ω
RB path = 1 MΩ at +0.1% + 9.5 Ω switch resistance = 1001009.5 Ω
emitter switch path = 9.5 Ω
```

The emitter voltage is measured rather than assumed to be exactly at the rail. For NPN:

```text
VBE = VB - VE
VCE = VC - VE
VCB = VC - VB
IB  = (VDD - VB) / RB_PATH
IC  = (VDD - VC) / RC_PATH
hFE = IC / IB
```

For PNP:

```text
VEB = VE - VB
VEC = VE - VC
VBC = VB - VC
IB  = VB / RB_PATH
IC  = VC / RC_PATH
hFE = IC / IB
```

The complete path resistances must be calibrated and stored in firmware. Ignoring the modeled switch resistance produced an hFE error of approximately +0.95% across the gain sweep.

## Gain-range simulation

File: `BJT_Gain_Range_Error_Budget.cir`

A matched custom NPN/PNP model was stepped through nominal `BF` values of 20, 30, 50, 100, 200, 300, and 500 at 25 °C.

| Nominal BF | Simulated hFE | NPN VBE | NPN VCE | NPN VCB |
|---:|---:|---:|---:|---:|
| 20 | 20.532 | 0.5838 V | 3.2432 V | 2.6594 V |
| 30 | 30.786 | 0.5941 V | 3.2151 V | 2.6210 V |
| 50 | 51.276 | 0.6071 V | 3.1593 V | 2.5522 V |
| 100 | 102.396 | 0.6247 V | 3.0211 V | 2.3964 V |
| 200 | 204.211 | 0.6424 V | 2.7480 V | 2.1057 V |
| 300 | 305.477 | 0.6526 V | 2.4783 V | 1.8257 V |
| 500 | 506.408 | 0.6656 V | 1.9471 V | 1.2815 V |

Every point remains comfortably active: the NPN collector-base voltage stays positive and the PNP base-collector voltage gives the corresponding positive margin. The 1 MΩ base path is therefore retained for the Version 1 range of approximately hFE 20–500.

Below hFE≈20, the collector voltage change becomes too small relative to ADC uncertainty for a ±10% gain claim. Such a device may still be reported as functional low gain, but the numeric hFE result should be marked low confidence.

## ADC and buffer error

The conservative per-node voltage error used in the gain deck is:

```text
STM32 ADC:  ±4 LSB at 3.3 V = ±3.223 mV
buffer:     ±0.750 mV
combined:   ±3.973 mV per measured node
```

Worst-case gain error places the collector reading and base reading in opposite error directions.

| Simulated hFE | ADC-low result | ADC-high result | Approx. worst relative error |
|---:|---:|---:|---:|
| 20.532 | 19.055 | 22.013 | ±7.2% |
| 30.786 | 29.288 | 32.289 | ±4.9% |
| 51.276 | 49.741 | 52.816 | ±3.0% |
| 102.396 | 100.774 | 104.024 | ±1.6% |
| 204.211 | 202.424 | 206.004 | ±0.9% |
| 305.477 | 303.529 | 307.431 | ±0.7% |
| 506.408 | 504.143 | 508.680 | ±0.5% |

This supports the existing ±10% indicative target across hFE 20–500 after path calibration. It does not turn the board into a precision transistor curve tracer: hFE itself varies strongly with current, temperature, and individual device.

## Base-node loading: buffer is mandatory

File: `BJT_Temperature_Loading_And_Faults.cir`

At 25 °C, the buffered 2N3904 model gives hFE≈301.868. A direct MCU input was then modeled with the STM32 worst-case ±1 µA pin leakage:

| Base-node load | Apparent hFE at 25 °C | Error from buffered result |
|---|---:|---:|
| Direct ADC, 1 µA drawn from base | 188.762 | -37.5% |
| Direct ADC, 1 µA injected into base | 415.247 | +37.6% |
| Buffer, 5 pA drawn | 301.867 | negligible |
| Buffer, 5 pA injected | 301.869 | negligible |

Therefore the 1 MΩ base node must not connect directly to an STM32 ADC input. Route it through the selected ultra-low-input-bias OPA391-class buffer. An analog switch placed before the buffer must also have nanoampere-class leakage; the previously selected TMUX1104 class remains suitable for breadboard evaluation.

## Temperature sensitivity

The LTspice library models were stepped from -10 °C through 25 °C to 60 °C.

| Device | -10 °C hFE | 25 °C hFE | 60 °C hFE |
|---|---:|---:|---:|
| 2N3904 | 250.63 | 301.87 | 356.03 |
| 2N3906 | 167.54 | 201.92 | 238.31 |

For the 2N3904 model, VBE changes from 0.7100 V to 0.5946 V over the same interval. This simulated hFE change is far larger than the electronics error. Consequently:

- no universal hFE threshold may be used to declare every BJT healthy or degraded;
- the board must report measured hFE, junction voltage, polarity, and test current;
- degradation claims require a user-selected part/reference range or comparison with a known-good device under the same test conditions;
- the result remains “indicative hFE at SECDB test current,” not a datasheet-bin guarantee.

## Frozen active-region validity rules

Accept an NPN gain result only if all of these are true:

```text
0.45 V <= VBE <= 0.85 V
VCE > 0.30 V
VCB = VC - VB > 0.20 V
collector voltage drop produces hFE >= 20 with adequate ADC margin
```

For PNP, use the mirrored conditions:

```text
0.45 V <= VEB <= 0.85 V
VEC > 0.30 V
VBC = VB - VC > 0.20 V
collector voltage produces hFE >= 20 with adequate ADC margin
```

If these checks fail, firmware must not display `IC/IB` as valid hFE. The thresholds are first-pass prototype values and will be tuned using real devices in Phase 4.

## Verified fault signatures at 25 °C

| Condition | Important simulated readings | Interpretation |
|---|---|---|
| Healthy 2N3904 | VBE=0.6525 V, VCE=2.4880 V, VCB=1.8354 V | Active; hFE valid |
| Open base lead | external VBE≈3.3000 V, IC≈1.73 nA | Base junction absent/open |
| Open collector lead | VBE=0.5480 V, VCE≈3.3000 V, IC≈3.3 pA | Base junction responds but collector does not |
| C-E short | VCE≈32.4 µV, IC≈3.235 mA | C-E short candidate |
| B-E short | VBE≈33 nV, IC≈3.3 pA | B-E short candidate |
| B-C short | VBE≈VCE≈0.6834 V, VCB≈0 V, IC≈2.565 mA | Fails active-region VCB rule |
| Excessive 10 kΩ base drive | VCE=0.0402 V, VCB=-0.6623 V, ratio=12.45 | Tester-induced saturation; ratio is not hFE |

A low VCE alone cannot distinguish a physical C-E short from saturation. If low VCE occurs during the 1 MΩ test, remove base drive and repeat the collector test:

- a healthy/saturated transistor should turn off and its collector return toward the rail;
- a C-E short continues drawing near-maximum collector current.

This base-off confirmation is mandatory before displaying “C-E short.”

## First-pass diagnostic flow

1. Verify that the component is unpowered and inserted with known E-B-C order.
2. Measure emitter, base, and collector nodes through buffered ADC sensing.
3. Confirm a plausible base-emitter junction voltage.
4. Calculate base and collector current using calibrated complete path resistances.
5. Apply the active-region rules before calculating hFE.
6. If active, report indicative hFE, VBE/VEB, IC, and test temperature if available.
7. If collector-emitter voltage is near zero, perform the base-off confirmation.
8. For invalid or conflicting signatures, report open/short candidate, wrong pins/polarity, or unknown rather than forcing a health label.

## Breadboard acceptance tests for Phase 4

1. Test at least three known NPN and three known PNP devices with documented pin order.
2. Compare repeated hFE results against a commercial multimeter/transistor tester only as a reference, not as an absolute truth.
3. Verify that the base buffer changes the measured result by less than 1% and that removing it demonstrates the predicted loading risk.
4. Verify open-base, open-collector, B-E short, B-C short, and C-E short fixtures safely.
5. Confirm every reported hFE passes the active-region voltage tests.
6. Repeat each device at least 20 times and record mean, spread, raw ADC codes, and calculated currents.
7. Confirm no test current or node voltage exceeds the selected MCU, buffer, switch, or DUT limits.

## Phase 3D conclusion

The hFE 20–500 range and ±10% electronics target are feasible with a calibrated 1 MΩ base path, calibrated 1 kΩ collector path, emitter-voltage sensing, and an ultra-low-bias ADC buffer. Direct ADC loading is unacceptable. BJT health must remain conditional on active-region validity and part-specific expectations.

With the resistor, capacitor, diode, and BJT error budgets now complete, Phase 3 is complete. The next phase is Phase 4: breadboard validation with the STM32F401 board and real components before designing the shared analog front end.
