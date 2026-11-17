# SECDB Phase 3 — Resistor Autorange Switch-Resistance Study

## Question

Can an ordinary analog switch be placed in series with the 1 kΩ, 10 kΩ, and 100 kΩ resistor references without damaging measurement accuracy?

## Model

```text
3.3 V -> RREF -> RON of selected switch -> ADC_IN -> RX -> GND
```

The firmware initially assumes that the selected reference is exactly `RREF`:

```text
RX_CALCULATED = RREF × VADC / (3.3 - VADC)
```

The real upper resistance is `RREF + RON`. Consequently:

```text
RX_CALCULATED = RX_ACTUAL × RREF / (RREF + RON)
```

The switch therefore creates a systematic negative error if it is ignored.

## Verified LTspice results

Each range was tested at its mid-scale point, where `RX = RREF`.

| RREF | RX actual | Switch RON | VADC | Calculated RX | Error |
|---:|---:|---:|---:|---:|---:|
| 1 kΩ | 1 kΩ | 10 Ω | 1.641791 V | 990.10 Ω | −0.99% |
| 1 kΩ | 1 kΩ | 50 Ω | 1.609756 V | 952.38 Ω | −4.76% |
| 10 kΩ | 10 kΩ | 10 Ω | 1.649175 V | 9.99001 kΩ | −0.10% |
| 10 kΩ | 10 kΩ | 50 Ω | 1.645885 V | 9.95025 kΩ | −0.50% |
| 100 kΩ | 100 kΩ | 10 Ω | 1.649918 V | 99.9900 kΩ | −0.01% |
| 100 kΩ | 100 kΩ | 50 Ω | 1.649588 V | 99.9500 kΩ | −0.05% |

## Decision

A switch with approximately 50 Ω on-resistance is unsuitable for the 1 kΩ precision range if firmware ignores it. It consumes nearly the entire error budget by itself.

Preliminary selection requirements for the future analog switch are:

| Parameter | Requirement or design goal |
|---|---|
| Supply compatibility | Correct operation from the 3.3 V system rail |
| Signal range | Includes 0–3.3 V |
| On-resistance | Preferably 5 Ω or less for the 1 kΩ path |
| On-resistance variation | Low across signal voltage and temperature |
| Off leakage | Preferably a few nA at room temperature; must be budgeted across temperature |
| Channels | Enough to select three resistor references without unintended parallel paths |
| Break-before-make | Preferred to prevent temporarily paralleling reference resistors |

With 5 Ω in series with 1 kΩ, the uncorrected systematic error is approximately −0.50%. Static calibration can remove much of the nominal path resistance, but it cannot completely remove voltage and temperature variation.

## High-resistance leakage constraint

At `RX = 1 MΩ` with a 100 kΩ reference, the intended divider current is only approximately:

```text
3.3 V / 1.1 MΩ = 3.0 µA
```

An unwanted leakage current of 30 nA is already about 1% of this current. The high range therefore depends not only on low on-resistance but also on very low switch, PCB, socket, and ADC leakage. Clean PCB layout and calibration are required.

## Next action

Do not choose CD4053 or 74HC4051 merely because they are common. First compare candidate datasheets at 3.3 V for on-resistance, on-resistance flatness, off leakage, channel count, package availability, and cost. The selected part should then be inserted into a non-ideal LTspice model and rechecked before the shared front end is frozen.
