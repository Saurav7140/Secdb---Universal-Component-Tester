# SECDB — Resistor Measurement, Stage 1

## Status

The required single-range resistor divider has been simulated successfully in LTspice 26.0.2. Seven stepped operating points converged with no floating-node or circuit errors.

The earlier Desktop file `Resistor_Measurement_Module.asc` is a later autoranging experiment, not the required first-stage circuit. It contains 1 kΩ, 10 kΩ, and 100 kΩ reference branches, voltage-controlled switches, a 100 nF capacitor, `.tran 5m`, and only `.param Rx=100k`. Its saved log reports `V(ADC_IN)=1.65016 V`, which is reasonable for equal 100 kΩ arms, but it does not verify the requested seven-value 10 kΩ-reference sweep.

## Verified circuit

```text
        3.3 V
          |
       RREF = 10 kΩ
          |
          +------ ADC_IN
          |
       RX = unknown
          |
         GND
```

Connections:

1. Connect the positive terminal of the ideal 3.3 V source to the upper end of `RREF`.
2. Set `RREF` to `10k`.
3. Join the lower end of `RREF` and the upper end of `RX`; label this node `ADC_IN`.
4. Connect the lower end of `RX` to ground.
5. Connect the negative terminal of the 3.3 V source to the same ground.
6. Set the value of `RX` to `{Rx}`.

Simulation directives:

```text
.param Rx=1k
.step param Rx list 220 1k 4.7k 10k 47k 100k 1Meg
.op
```

After running, click the `ADC_IN` node to inspect the stepped voltage, or use **View → SPICE Error Log**. The LTspice 26 stepped `.meas OP` text display can show only an endpoint for this deck, so the full verification table below was read from all seven operating points in the generated RAW data.

## Equation

The same current flows through both resistors:

```text
I = VCC / (RREF + RX)
```

The ADC measures the voltage across `RX`:

```text
VADC = I × RX
     = VCC × RX / (RREF + RX)
```

Rearranging:

```text
RX = RREF × VADC / (VCC - VADC)
```

For this simulation:

```text
RX = 10000 × VADC / (3.3 - VADC) Ω
```

If the divider is powered from the same 3.3 V rail used as the STM32 ADC reference, firmware can use the ADC code directly:

```text
RX = RREF × ADC_code / (4095 - ADC_code)
```

This ratiometric form avoids assuming that the rail is exactly 3.300 V.

## LTspice results

| Set RX | Simulated VADC | Calculated RX | Ideal 12-bit ADC code |
|---:|---:|---:|---:|
| 220 Ω | 0.071037 V | 220 Ω | 88 |
| 1 kΩ | 0.300000 V | 1.000 kΩ | 372 |
| 4.7 kΩ | 1.055102 V | 4.700 kΩ | 1309 |
| 10 kΩ | 1.650000 V | 10.000 kΩ | 2048 |
| 47 kΩ | 2.721053 V | 47.000 kΩ | 3377 |
| 100 kΩ | 3.000000 V | 100.000 kΩ | 3723 |
| 1 MΩ | 3.267327 V | approximately 1.000 MΩ | 4054 |

The voltages increase monotonically with resistance, and substituting each voltage back into the divider equation reproduces the stepped resistance. This verifies the ideal circuit and formula.

## Engineering interpretation

The 10 kΩ reference gives its best resolution near `RX = 10 kΩ`, where `VADC = 1.65 V`. The 220 Ω and 1 MΩ cases sit near the ADC rails. They are detectable, but they will be more sensitive to ADC quantization, noise, resistor tolerance, and leakage.

For example, an ideal 12-bit conversion of the 1 MΩ result is only about 41 counts below full scale. A one-count change near this point corresponds to roughly a 2.5% resistance change. Therefore, a fixed 10 kΩ reference should not be claimed as an accurate 220 Ω-to-1 MΩ measurement system. Autoranging may later solve this, but it should remain outside Stage 1 until breadboard and ADC tests confirm this simple range.

## Simulation parts versus final PCB parts

| Stage | 3.3 V source | RREF | RX | ADC input |
|---|---|---|---|---|
| LTspice | Ideal 3.3 V source | Ideal 10 kΩ resistor | Parameter `{Rx}` | Voltage label/probe |
| Prototype/PCB | Regulated 3.3 V/VDDA rail | Prefer 0.1% reference resistor | Test socket component | STM32 ADC pin, with suitable protection added later |

No diode, capacitor, BJT, switching matrix, or protection network is included in this verification stage.

## Files

- `Resistor_Measurement_Verified.asc` — graphical LTspice schematic.
- `Resistor_Measurement_Verified.cir` — equivalent text netlist.
- `Resistor_Measurement_Verified.raw` and `.log` — generated verification outputs.

Stage 1 simulation is complete. The next resistor-stage activity should be reviewing the curves/results together or validating the same divider with the STM32 ADC/breadboard; the project should not advance to the diode circuit until this result is accepted.
