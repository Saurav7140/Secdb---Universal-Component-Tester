# SECDB — BJT Measurement, Stage 4

## Status

The independent BJT measurement stage has been simulated successfully in LTspice 26.0.2. The tests cover:

- the original saturated 2N3904 circuit;
- a corrected active-region 2N3904 gain measurement;
- an artificially degraded low-gain NPN;
- an open-base fault;
- a collector-emitter short;
- an active-region 2N3906 PNP test.

## Important correction to the original circuit

The existing Desktop schematic uses a 3.3 V base source through 10 kΩ and a 3.3 V collector source through 1 kΩ. Its saved results are:

```text
IB   = 260 µA
IC   = 3.26 mA
VBE  = 0.700 V
VCE  = 0.040 V
IC/IB = 12.5
```

The circuit is functioning, but the transistor is saturated because `VCE` is only approximately 40 mV and the base-collector junction is forward biased. The ratio of 12.5 is a forced saturation beta, not the transistor's active-region `hFE`. It must not be interpreted as a degraded transistor.

## Corrected NPN circuit

```text
                  3.3 V
                    |
                 RC = 1 kΩ
                    |
              ADC_C +------ Collector
                              |\
        3.3 V                 | > 2N3904
          |                   |/
       RB = 1 MΩ               Emitter
          |                      |
    ADC_B +------ Base          GND
```

The 1 MΩ base resistor reduces base current sufficiently to keep the tested 2N3904 in its active region.

Connections:

1. Connect the NPN emitter to ground.
2. Connect its collector to 3.3 V through `RC = 1k`.
3. Measure the collector node using `ADC_C`.
4. Connect its base to 3.3 V through `RB = 1Meg`.
5. Measure the base node using `ADC_B`.
6. Use the known E-B-C pin order for this stage. Automatic pin identification is not included.

## NPN equations

The STM32 can calculate all required values from the two node voltages:

```text
VBE = VADC_B
VCE = VADC_C

IB = (3.3 - VADC_B) / RB
IC = (3.3 - VADC_C) / RC

hFE = IC / IB
```

The firmware must check that the transistor is in the active region before accepting `hFE`. For an NPN device, the collector should be above the base and `VCE` should be comfortably above saturation.

## Verified NPN results

| Test | VBE | VCE | IB | IC | Calculated ratio |
|---|---:|---:|---:|---:|---:|
| Original 10 kΩ base circuit | 0.6998 V | 0.0404 V | 260.0 µA | 3.2596 mA | 12.54, invalid as hFE because saturated |
| Corrected 2N3904 active test | 0.6493 V | 2.4918 V | 2.6507 µA | 0.80825 mA | hFE = 304.92 |
| Artificial degraded model | 0.5902 V | 3.2166 V | 2.7098 µA | 83.43 µA | hFE = 30.79 |
| Open-base approximation | 0.3115 V | 3.3000 V | approximately 3 pA | approximately 1.75 nA | No collector response |
| C-E short approximation | 0.5383 V | 0.000033 V | 2.7617 µA | 3.29997 mA | C-E short signature |

The degraded model uses an artificial model parameter `BF=30`. It demonstrates how low gain appears electrically; it is not a universal failure boundary for every transistor type.

## Corrected PNP circuit

The polarities are reversed for a PNP device:

```text
                   3.3 V
                     |
                   Emitter
                    /|
          Base ----< | 2N3906
                    \|
                  Collector
                     |
               ADC_C +
                     |
                  RC = 1 kΩ
                     |
                    GND

          Base
            +------ ADC_B
            |
         RB = 1 MΩ
            |
           GND
```

PNP calculations:

```text
VEB = 3.3 - VADC_B
VEC = 3.3 - VADC_C

IB = VADC_B / RB
IC = VADC_C / RC

hFE = IC / IB
```

Verified 2N3906 result:

| VEB | VEC | IB | IC | hFE |
|---:|---:|---:|---:|---:|
| 0.6389 V | 2.7572 V | 2.6611 µA | 0.54277 mA | 203.97 |

## Provisional diagnostic interpretation

```text
Normal VBE/VEB + active-region VCE/VEC + collector response
    -> functioning BJT; report measured hFE

Normal base drive + very small collector response
    -> open, incorrect pins, wrong polarity, or faulty device

Collector-emitter voltage near zero with high collector current
    -> possible C-E short, or a saturated test caused by excessive base drive

Active-region operation + unusually low hFE relative to an expected part
    -> low-gain/degraded candidate
```

The system must distinguish saturation caused by the tester from an actual short. That is why the active-region validity check is mandatory.

## Limits of health classification

- `hFE` varies greatly between individual devices, collector current, temperature, and transistor part number.
- A measured hFE of 30 is not automatically faulty for every BJT.
- If the exact transistor type or expected range is unknown, the board should report the measured value and a broad functional result rather than claim degradation with certainty.
- The first version requires the user to insert the known emitter, base, and collector pins correctly.
- Automatic pin identification should remain future work.

## STM32 and final-hardware considerations

- A 1 MΩ base resistor keeps the simulation in the active region, but it creates a relatively high-impedance measurement node.
- The STM32 ADC will require an appropriately long sampling time, and input leakage must be checked against the selected MCU datasheet.
- Later breadboard tests may show that 470 kΩ or another value gives a better speed/accuracy compromise while preserving the active region.
- Use measured node voltages and the actual supply/ADC reference rather than assuming exactly 3.300 V.
- Add ADC and socket protection only during analog-front-end integration; it is not part of this ideal test.

## Files

- `BJT_NPN_Active_Test.asc` — corrected graphical active-region NPN test.
- `BJT_Bias_Verification.cir` — saturation, good NPN, degraded NPN, open, C-E short, and PNP verification deck.
- Corresponding `.log` and `.raw` files — LTspice results.

## Stage conclusion

NPN and PNP measurement principles are verified at the ideal-simulation level. The four independent component simulations—resistor, diode, capacitor, and BJT—are now complete. The next project phase is to consolidate the verified files and define realistic measurement ranges, accuracy targets, and acceptance criteria before designing the shared analog switching front end.
