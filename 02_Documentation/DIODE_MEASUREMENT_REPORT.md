# SECDB — Diode Measurement, Stage 2

## Status

The independent diode-measurement circuit has been simulated successfully in LTspice 26.0.2. Forward, reverse, open, and short conditions are distinguishable when the device is measured in both orientations.

The earlier Desktop file `Diode.asc` produced a plausible 1N4148 forward voltage of approximately 0.630189 V. However, it included a 100 nF capacitor and used `.tran 5m`. The clean baseline circuit does not require the capacitor or transient analysis; `.op` is sufficient.

## Basic test circuit

```text
        3.3 V
          |
      RLIMIT = 1 kΩ
          |
          +------ ADC_IN
          |
       anode
        diode
       cathode
          |
         GND
```

The resistor limits the maximum possible test current to approximately:

```text
IMAX = 3.3 V / 1 kΩ = 3.3 mA
```

For a conducting diode:

```text
IDIODE = (3.3 V - VADC) / 1 kΩ
```

## LTspice construction

1. Place a 3.3 V DC voltage source.
2. Connect its negative terminal to ground.
3. Connect its positive terminal to a `1k` resistor.
4. Label the lower resistor node `ADC_IN`.
5. Place a diode with its **anode** connected to `ADC_IN` and its **cathode/bar** connected to ground.
6. Set the diode value to `1N4148`.
7. Add these directives:

```text
.op
.meas OP VFORWARD FIND V(ADC_IN)
.meas OP IFORWARD FIND I(RLIMIT)
```

Run the simulation and open **View → SPICE Error Log**.

## Verified results

| Test case | ADC voltage | Test current | Interpretation |
|---|---:|---:|---|
| 1N4148 forward | 0.630189 V | 2.66981 mA | Normal silicon forward conduction |
| 1N4148 reverse | 3.299998 V | 2.52 nA | Reverse blocking |
| Open-device model | 3.299998 V | 2.03 nA | No conduction |
| 10 mΩ short model | 0.000033 V | 3.29997 mA | Short circuit |

The original Desktop result of 0.630189 V agrees with the clean forward simulation.

## Diode-type comparison

All devices were driven from 3.3 V through the same 1 kΩ resistor.

| LTspice model | Simulated forward voltage | Comment |
|---|---:|---|
| 1N4148 | 0.630189 V | Small-signal silicon diode |
| 1N4007 | 0.623227 V | Silicon rectifier diode |
| 1N5819 | 0.163679 V | Schottky diode at this relatively low current |
| Illustrative red-LED model | 2.049007 V | Model-dependent; not tied to a specific LED part number |

Forward voltage changes with current, temperature, and the selected SPICE model. These values are simulation references, not universal datasheet limits.

## Reverse the diode

For the reverse test, rotate the diode by 180 degrees:

```text
        3.3 V
          |
         1 kΩ
          |
          +------ ADC_IN
          |
       cathode/bar
          diode
        anode
          |
         GND
```

A healthy diode now leaves `ADC_IN` close to 3.3 V because almost no current flows through the 1 kΩ resistor.

## Why both directions are mandatory

A reverse-biased healthy diode and an open diode both give an ADC reading close to 3.3 V. One reading therefore cannot distinguish them.

| Forward-orientation reading | Reverse-orientation reading | Probable result |
|---:|---:|---|
| Normal diode drop | Near 3.3 V | Healthy diode |
| Near 3.3 V | Near 3.3 V | Open, disconnected, or wrongly contacted |
| Near 0 V | Near 0 V | Shorted diode |
| Conducting in both directions but not near 0 V | Conducting in both directions | Leaky/incorrect device; investigate |

Provisional simulation rules for a silicon diode could be:

```text
Forward 0.50-0.85 V and reverse >3.0 V  -> healthy silicon diode
Both readings >3.0 V                   -> open/no contact
Both readings <0.10 V                  -> shorted
```

For Schottky diodes and LEDs, separate forward-voltage bands are required. Final thresholds must be calibrated using real components and temperature tests; they should not yet be treated as final PCB specifications.

## Simulation versus final hardware

| Simulation | Prototype/final PCB |
|---|---|
| Ideal 3.3 V source | Measured STM32 VDDA/3.3 V rail |
| Ideal 1 kΩ resistor | Prefer a 0.1% or 1% known resistor |
| Library diode model | Physical component in the test socket |
| Manual diode reversal/separate branches | User reverses the part initially; automatic polarity switching comes later |
| Ideal ADC voltage probe | Protected STM32 ADC input |

Automatic terminal swapping is deliberately excluded from this independent verification stage.

## Files

- `Diode_Forward_1N4148.asc` — clean graphical LTspice schematic.
- `Diode_Condition_Verification.cir` — forward, reverse, open, short, silicon, Schottky, and illustrative LED verification deck.
- Corresponding `.log` and `.raw` files — LTspice outputs.

## Stage conclusion

The circuit has verified the required electrical behavior. A reliable diode diagnosis requires two polarity measurements. Stage 2 can be considered complete at the ideal-simulation level, without yet designing the automatic switching hardware.
