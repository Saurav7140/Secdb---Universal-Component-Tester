# SECDB — Phase 4A Virtual Breadboard: STM32 ADC and Buffer Bring-Up

## Scope and status

This is a virtual breadboard connection plan for the STM32F401 Black Pill measurement platform. It verifies the wiring logic and expected electrical values in LTspice, but it is not a physical hardware validation.

Only the power rails, one OPA391 voltage follower, STM32 ADC input PA0, and a known-voltage calibration ladder are connected in this step. Unknown components, the TFT, analog switches, and the final shared measurement network are deliberately excluded until this foundation is checked.

## Virtual parts list

| Quantity | Part | Virtual/prototype purpose |
|---:|---|---|
| 1 | STM32F401CCU6/CEU6 Black Pill | Temporary controller module |
| 1 | OPA391DBV, SOT-23-5 | Ultra-low-bias ADC voltage follower |
| 1 | SOT-23-5 to DIP adapter | Required if this is ever assembled on a real breadboard |
| 3 | 10 kΩ resistors, preferably 0.1% | 3.3 V calibration ladder |
| 1 | 100 nF ceramic capacitor | OPA391 local supply decoupling |
| 1 | 100 Ω resistor | Buffer-output isolation to PA0 |
| 1 | 10 µF capacitor | Breadboard 3.3 V rail stabilization |

The 10 kΩ ladder is a validation fixture, not part of the final measurement circuit.

## Frozen Phase 4A pin allocation

| Black Pill pin | Phase 4A function | Configuration |
|---|---|---|
| 3V3 | Breadboard positive rail | 3.3 V supply output when board is USB powered |
| GND | Breadboard ground rail | Common reference |
| PA0 | `ADC_BUFFERED` | ADC1_IN0, analog mode, no pull-up/down |
| PA1 | Reserved | Future measurement channel |
| PA2 | Reserved | Future measurement channel |
| PA3 | Reserved | Future measurement channel |

PA4, PA5, PA7, PB0, and PB1 remain reserved for the ST7735 display. PA13 and PA14 remain reserved for SWD programming/debug. PA11 and PA12 should remain available for USB if USB data is retained.

## Power connections

For this virtual prototype, assume the Black Pill is powered through its own USB connector:

```text
Black Pill 3V3  -> breadboard positive rail
Black Pill GND  -> breadboard ground rail
10 uF capacitor -> positive rail to ground rail
```

Do not connect the Black Pill `5V` pin to the 3.3 V rail. If a real prototype later uses the external AP2112K supply, do not power the board simultaneously from USB and an external 3.3 V source until the exact board power-path schematic has been checked.

## OPA391DBV connection

The pin numbering below is specifically for the **DBV SOT-23-5 package**. The DCK SC70 package uses different signal pin numbers and must not be substituted using this table.

| OPA391DBV pin | Name | Connection |
|---:|---|---|
| 1 | OUT | Connect to pin 4 and then through 100 Ω to Black Pill PA0 |
| 2 | V- | Ground rail |
| 3 | +IN | Calibration selector/input node |
| 4 | -IN | Connect directly to pin 1 for unity gain |
| 5 | V+ | 3.3 V rail |

Place 100 nF directly between OPA391 pins 5 and 2. This forms a unity-gain voltage follower:

```text
measured node -> pin 3 (+IN)
pin 1 (OUT)   -> pin 4 (-IN)
pin 1 (OUT)   -> 100 Ω -> PA0
```

## Calibration ladder

Connect three equal 10 kΩ resistors in series:

```text
3.3 V --- 10k --- TAP_2V2 --- 10k --- TAP_1V1 --- 10k --- GND
```

With ideal equal resistors:

```text
TAP_1V1 = 3.3 × 1/3 = 1.1 V
TAP_2V2 = 3.3 × 2/3 = 2.2 V
```

The virtual jumper connects one ladder tap at a time to OPA391 pin 3. Never connect the 1.1 V and 2.2 V taps together.

## Virtual connection sequence

1. Connect the Black Pill 3V3 and GND rails.
2. Add the 10 µF rail capacitor with the correct polarity if it is electrolytic.
3. Connect OPA391 pins 5 and 2 to 3.3 V and GND.
4. Add the 100 nF capacitor directly across OPA391 pins 5 and 2.
5. Connect OPA391 pin 1 to pin 4.
6. Connect pin 1 through 100 Ω to PA0.
7. Build the three-resistor ladder.
8. First jumper `TAP_1V1` to OPA391 pin 3 and predict ADC code 1365.
9. Move the jumper to `TAP_2V2` and predict ADC code 2730.
10. Remove the jumper before changing the circuit. The buffer input must not be left floating during an actual measurement; firmware should only sample when a defined source is selected.

## ADC configuration target

| Setting | Initial value |
|---|---|
| ADC peripheral | ADC1 |
| Channel | IN0 on PA0 |
| Resolution | 12 bit |
| Alignment | Right |
| Sampling time | 480 ADC cycles |
| ADC clock | 18 MHz or lower |
| Pull resistors on PA0 | None |
| Readings per reported value | 32 |
| Processing | Discard first sample after switching, median/outlier check, then average |

The maximum sample time is intentionally used for first bring-up. It can be shortened only after real-hardware repeatability is measured.

## LTspice verification

File: `Virtual_Breadboard_ADC_Buffer_Check.cir`

The simulation includes:

- the 10 kΩ calibration ladder;
- conservative 5 pA buffer input loading;
- an ideal unity-gain buffer with 10 Ω output resistance;
- the 100 Ω PA0 isolation resistor;
- worst-case ±750 µV buffer offset;
- worst-case ±1 µA STM32 pin leakage at the buffer output;
- an additional ±4 LSB ADC conversion-error allowance.

Expected nominal ADC codes are:

```text
code(1.1 V) = 1.1 / 3.3 × 4095 = 1365
code(2.2 V) = 2.2 / 3.3 × 4095 = 2730
```

Verified LTspice results:

| Input tap | Nominal buffered voltage | Worst buffer/output-loading range | Range including ±4 LSB ADC allowance |
|---:|---:|---:|---:|
| 1.1 V | 1.0999999 V | 1.0991399–1.1008600 V | 1.0959165–1.1040834 V |
| 2.2 V | 2.2000000 V | 2.1991398–2.2008600 V | 2.1959164–2.2040835 V |

The predicted nominal codes were verified as 1365 and 2730.

### TI vendor-model confirmation

File: `Virtual_Breadboard_OPA391_TI_Model.cir`

TI's official **OPAx391 PSpice Model Rev. B**, internal macro-model version Final 1.3, was downloaded from the OPA391 product page and included directly in LTspice. LTspice 26.0.2 accepted the model without syntax or convergence errors.

The confirmation circuit uses a 3.3 V supply, voltage-follower connection, 100 Ω output isolation, 7 pF ADC load, and opposite ±1 µA ADC leakage directions on the two test branches.

| Ladder input | OPA391 output | PA0 modeled voltage | Equivalent ideal ADC code | Error from nominal |
|---:|---:|---:|---:|---:|
| 1.1000000 V | 1.0999870 V | 1.0998870 V | 1364.860 | -0.1130 mV |
| 2.2000000 V | 2.1999969 V | 2.2000971 V | 2730.120 | +0.0971 mV |

A simulated input change from 1.1 V to 2.2 V reached within 1 mV of the final PA0 value in approximately **0.381 µs**. The planned firmware delay and discarded first ADC conversion therefore provide ample modeled settling margin. Real breadboard parasitics and analog-switch settling remain unverified.

The behavioral worst-case windows remain the acceptance limits because the vendor macro-model represents typical modeled behavior rather than every datasheet limit.

## Pass criteria for the future physical check

| Test input | Nominal code | Initial acceptable voltage window |
|---:|---:|---:|
| 1.1 V | 1365 | 1.0959–1.1041 V before physical ladder-tolerance allowance |
| 2.2 V | 2730 | 2.1959–2.2041 V before physical ladder-tolerance allowance |

A future hardware test passes only if readings are stable, no node exceeds 3.3 V, and the averaged result lies within the documented window after accounting for actual ladder tolerance and measured rail voltage.

## Safety and limitations

- This connection accepts only signals between ground and 3.3 V.
- Do not connect charged capacitors, powered circuits, negative voltages, or 5 V signals.
- OPA391 SOT-23-5 cannot plug directly into a solderless breadboard; an adapter is required.
- PA0 is verified by ST as ADC1_IN0, but Black Pill header placement varies between board revisions and clones. Follow the printed pin label, not a guessed physical row number.
- Virtual validation does not prove noise performance, contact quality, clone-board power routing, or real ADC calibration.

## Next step

Phase 4B resistor, Phase 4C capacitor, Phase 4D diode, and Phase 4E known-pinout BJT integration have now been completed separately. Phase 4 is complete; the next stage is Phase 5 shared analog-front-end consolidation and GPIO/switch-resource audit.
