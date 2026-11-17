# SECDB — Capacitor Measurement, Stage 3

## Status

The independent RC-charging measurement has been simulated successfully in LTspice 26.0.2 for 100 nF, 1 µF, 10 µF, and 100 µF capacitors. Open- and short-circuit reference cases were also verified.

The earlier Desktop file `capacitor.asc` reported `T63 = 1.00137 s` for 100 µF with a 10 kΩ reference, which is a correct result. That file already contains switched 1 kΩ, 10 kΩ, and 100 kΩ ranges plus a discharge switch. The clean Stage 3 baseline uses only one 10 kΩ charging resistor.

## Verified circuit

```text
       PULSE: 0 V to 3.3 V
                 |
          RCHARGE = 10 kΩ
                 |
                 +------ ADC_IN
                 |
              CX unknown
                 |
                GND
```

Connections:

1. Connect the negative terminal of the pulse source to ground.
2. Configure the source as `PULSE(0 3.3 0 1n 1n 10s 20s)`.
3. Connect its positive terminal to a 10 kΩ resistor.
4. Label the lower resistor node `ADC_IN`.
5. Connect the unknown capacitor from `ADC_IN` to ground.
6. Set the capacitor value to `{Cx}`.
7. Add the simulation directives below.

```text
.param Cx=1u
.step param Cx list 100n 1u 10u 100u
.options plotwinsize=0 reltol=1e-6
.tran 0 5s
.meas TRAN T63 WHEN V(ADC_IN)=2.08599784 RISE=1
```

The `plotwinsize=0` option prevents waveform compression from reducing the accuracy of the threshold-time measurement.

## Measurement equation

The capacitor charges according to:

```text
VADC(t) = VCC × (1 - exp(-t/(RCHARGE × C)))
```

At one time constant:

```text
t = RCHARGE × C
VADC = VCC × (1 - exp(-1))
VADC = 0.6321206 × VCC
```

For a 3.3 V supply:

```text
VTHRESHOLD = 0.6321206 × 3.3
           = 2.085998 V
```

Therefore:

```text
C = T63 / RCHARGE
C = T63 / 10,000 farads
```

The formula `C = I × t / ΔV` applies to constant-current charging. This circuit uses resistor-based exponential charging, so the 63.2% threshold formula above must be used instead.

## Verified LTspice results

| Set capacitance | Ideal T63 | Simulated T63 | Calculated capacitance | Approx. error |
|---:|---:|---:|---:|---:|
| 100 nF | 1.000 ms | 1.000290 ms | 100.029 nF | +0.029% |
| 1 µF | 10.000 ms | 10.000524 ms | 1.000052 µF | +0.005% |
| 10 µF | 100.000 ms | 99.999083 ms | 9.999908 µF | −0.001% |
| 100 µF | 1.000 s | 1.000001 s | 100.000097 µF | approximately 0% |

The four curves rise exponentially. Increasing capacitance by a factor of ten increases the threshold time by approximately a factor of ten.

## Fault-reference results

| Condition | Simulated response | Probable diagnosis |
|---|---|---|
| Open/no capacitor | ADC reached approximately 3.3 V almost immediately | Open, absent, poor contact, or capacitance below the supported range |
| 10 mΩ short | ADC remained at approximately 3.3 µV | Shorted component or test node short |
| Lower-than-nominal capacitance | Threshold reached earlier than expected | Possible degraded capacitor, wrong value, or tolerance variation |
| Threshold never reached | Voltage remains low or settles below threshold | Short, excessive leakage, wrong connection, or circuit fault |

Real ADC input capacitance and stray PCB capacitance mean an open socket will not charge in literally zero time. A minimum-time threshold must be established during hardware calibration.

## STM32 implementation concept

If the charging supply and ADC reference use the same 3.3 V rail, the firmware threshold is approximately 63.212% of full scale. For a 12-bit ADC:

```text
Threshold code ≈ 0.6321206 × 4095 ≈ 2589
```

A simple future firmware sequence is:

1. Drive the charge pin low long enough to discharge the capacitor.
2. Confirm with the ADC that `ADC_IN` is close to 0 V.
3. Start a hardware timer and drive the charge pin high.
4. Repeatedly sample the ADC.
5. Stop the timer when the ADC reading reaches approximately 2589.
6. Calculate `C = elapsed_time / 10000`.
7. Apply a timeout to detect a short, excessive leakage, or an unsupported large capacitance.
8. Drive the pin low again to discharge the capacitor before removal or another test.

The maximum initial GPIO current through 10 kΩ is only:

```text
3.3 V / 10 kΩ = 0.33 mA
```

This is modest, but the final GPIO and ADC connections still require protection review.

## Practical limitations

- A 10 kΩ range produces manageable timing from 1 ms to 1 s for 100 nF through 100 µF.
- A 1000 µF capacitor would require approximately 10 s to reach the threshold, which may be too slow for the initial product.
- The technique measures effective capacitance; it does not directly provide an accurate ESR measurement.
- Leakage current, dielectric absorption, tolerance, temperature, and ADC timing will affect physical measurements.
- A low capacitance result alone does not scientifically prove that an electrolytic capacitor is unhealthy. Diagnosis should report the measured deviation and charging behavior.
- Polarized capacitors must be inserted with correct polarity.
- A charged external capacitor must be safely discharged before it is connected to the board.

## Simulation versus final hardware

| Simulation | Prototype/final PCB |
|---|---|
| Ideal pulse source | STM32 GPIO or controlled 3.3 V switch |
| Ideal 10 kΩ resistor | Known 0.1% or 1% resistor |
| Ideal capacitor values | Physical test-socket component |
| Perfect zero initial voltage | Firmware-controlled discharge and ADC confirmation |
| Voltage probe | Protected STM32 ADC input |

The discharge switch and multiple resistance ranges remain later integration work. They are not required to validate the present RC principle.

## Files

- `Capacitor_Charging_Sweep.asc` — graphical LTspice parameter sweep.
- `Capacitor_Charging_Verified.cir` — four-value and fault-reference verification deck.
- Corresponding `.log` and `.raw` files — LTspice results.

## Stage conclusion

The RC method and firmware equation are verified at the ideal-simulation level for 100 nF to 100 µF. The next independent analog simulation is the BJT measurement circuit; no shared switching matrix should be designed until that test is complete.
