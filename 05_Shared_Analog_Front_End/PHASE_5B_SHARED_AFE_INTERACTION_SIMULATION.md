# SECDB — Phase 5B Consolidated Analog-Front-End Interaction Simulation

## Status

Phase 5B is complete at the LTspice level. The Phase 5A architecture—one TMUX1104 R/C source mux, one TMUX1108 universal sense mux, one OPA391, and PA0—was tested across resistor, capacitor, diode, BJT, calibration, rail-fault, off-channel, and high-impedance settling cases.

Every acceptance gate passed. These are simulation results; physical validation remains pending.

## Final Phase 5B model assumptions

| Item | Model used |
|---|---|
| U_SRC | TMUX1104 selected path, 9.5 Ω conservative on-resistance |
| U_SENSE | TMUX1108 selected path, 9.5 Ω conservative on-resistance |
| Sense-node capacitance | 100 pF, including 65 pF typical TMUX1108 on-capacitance plus trace/socket/buffer allocation |
| Aggregate sense leakage | 2 nA conservative allocation where relevant |
| Buffer | TI OPAx391 PSpice macro-model Rev. B |
| Buffer-to-PA0 resistor | 100 Ω |
| ADC input capacitance | 7 pF |
| ADC/buffer static uncertainty allocation | ±3.973 mV per sampled node |
| Supply | 3.3 V |

The 2 nA leakage allocation is deliberately no lower than the earlier Phase 3/4 budget. The current TMUX1108 datasheet gives lower limits through the normal temperature range, so retaining 2 nA avoids making the range claims artificially better during consolidation.

## Simulation files

| File | Purpose |
|---|---|
| `Phase_5B_Shared_AFE_Resistor.cir` | Full resistor sweep with source/sense mux paths, inactive channels, OPA391, ADC loading, leakage, and error allocation |
| `Phase_5B_Shared_AFE_Capacitor.cir` | Shared 10 kΩ charge path, S4 discharge path, 100 pF sense capacitance, leakage, OPA391 threshold timing |
| `Phase_5B_Shared_AFE_Active_Modes.cir` | 1.1/2.2 V calibration, conducting diode, NPN, and PNP through TMUX1108/OPA391 |
| `Phase_5B_Shared_AFE_Isolation_Settling.cir` | Channel transition, 1 MΩ resistor settling, BJT-base charge sharing, reverse/open/short signatures, and off-state loading |

The corresponding `.log` and `.raw` files are retained as simulation evidence.

## Resistor-mode result

The 19-point 100 Ω–1 MΩ sweep retained the Phase 4 autorange boundaries and calibrated source paths. U_SENSE adds 9.5 Ω before the OPA391 input, but its DC voltage drop is negligible because the buffer input current is extremely small.

| Actual RX | Selected reference | Nominal estimate | Conservative error interval |
|---:|---:|---:|---:|
| 100 Ω | 1 kΩ | 99.9925 Ω | −1.512% to +1.501% |
| 1 kΩ | 1 kΩ | 999.990 Ω | −0.494% to +0.494% |
| 10 kΩ | 10 kΩ | 9999.903 Ω | −0.495% to +0.495% |
| 100 kΩ | 100 kΩ | 99999.013 Ω | −0.506% to +0.506% |
| 1 MΩ | 100 kΩ | 1,000,009 Ω | −1.536% to +1.583% |

Full-range result:

```text
minimum modeled conservative error = -1.5358%
maximum modeled conservative error = +1.5826%
acceptance requirement              = within +/-2%
result                              = PASS
```

The shared architecture therefore preserves the 100 Ω–1 MΩ resistor claim.

## Capacitor-mode result

U_SRC S2 reuses the 10 kΩ resistor reference as the capacitor charge path. U_SRC S4 provides the separate 1 kΩ discharge path. U_SENSE remains on `RC_SENSE` throughout discharge and charge, so the sense mux is not changed while the timing measurement is running.

| Actual CX | Code-2589 time | Calibrated estimate | Center error | Time to discharge below 1% |
|---:|---:|---:|---:|---:|
| 100 nF | 1.002782 ms | 100.0521 nF | +0.0521% | 0.46582 ms |
| 1 µF | 10.023305 ms | 1.000070 µF | +0.0070% | 4.65397 ms |
| 10 µF | 100.228501 ms | 10.000248 µF | +0.0025% | 46.5354 ms |
| 100 µF | 1.002281 s | 100.0020 µF | +0.0020% | 0.465350 s |

The 100 pF shared-sense capacitance is most visible at 100 nF and adds approximately 0.05%. The code-2585/code-2593 timing window remains approximately ±0.266%, so the consolidated circuit remains compatible with the ±5% electronics target after calibration.

The vendor op-amp model used Gmin stepping near its ground-rail startup point and then completed every value and measurement. Explicit floating 1 TΩ inactive nodes were removed from this deck because they destabilized the vendor macro-model; their physical aggregate effect remains represented by 100 pF and 2 nA.

## Calibration-channel result

The always-present three-resistor ladder and TMUX1108 S7/S8 channels produced:

| Channel | ADC voltage | Equivalent ideal code | Nominal code |
|---|---:|---:|---:|
| `CAL_1V1` | 1.099967 V | 1364.959 | 1365 |
| `CAL_2V2` | 2.199977 V | 2729.971 | 2730 |

The small shifts include the deliberately conservative 2 nA sense leakage. Both results remain comfortably inside the Phase 4A validation windows.

## Diode-mode result

The conducting 1N4148 model, sensed sequentially through the universal TMUX1108 path, produced:

```text
forward voltage = 0.600193 V
test current     = 1.28440 mA
```

Rail-level cases in the isolation deck produced:

| Case | Differential voltage | Classification |
|---|---:|---|
| Healthy diode reverse-biased | 3.299996 V | Blocking |
| 1 GΩ open | 3.299993 V | Open-like |
| 10 mΩ short | 15.74 µV | Short |

The short current remained 1.56993 mA. The ratiometric 90%-of-open blocking threshold and both-polarities-below-0.10-V short rule remain valid.

## BJT-mode result

The real LTspice 2N3904 and 2N3906 models were biased using the Phase 5 GPIO/resistor paths and sensed through the universal TMUX1108 plus OPA391.

| Device | Junction voltage | Active-region voltages | Indicative hFE | Result |
|---|---:|---|---:|---|
| 2N3904 | VBE=0.651447 V | VCE=2.383584 V, VCB=1.732137 V | 301.35 | Active; valid |
| 2N3906 | VEB=0.641511 V | VEC=2.674590 V, VBC=2.033079 V | 201.92 | Active; valid |

The 2 nA conservative selected-channel leakage changes the high-impedance base node slightly but does not invalidate the Phase 4E range or active-region rules. Direct connection of the BJT base node to the STM32 ADC remains prohibited; the OPA391 is still mandatory.

## Shared-mux settling result

The worst transition cases used the 100 pF sense-node allocation and the TI OPA391 model.

| Transition | Verified behavior |
|---|---:|
| 1.1 V to 2.2 V calibration channel | Reached within 1 mV in 0.381 µs |
| Enable 100 kΩ source range with 1 MΩ DUT | Reached within 1 mV of 2.9997 V in 72.91 µs |
| Switch a 2.2 V-precharged sense node onto the 2N3904 base | Base initially disturbed by 1.349 V in the deliberately severe charge-sharing model |
| BJT base recovery | Within 1 mV of the undisturbed reference in 39.95 µs |
| BJT base residual at 50 µs | 0.302 mV |
| BJT base residual at 100 µs | 0.0753 mV |
| Buffered ADC residual at 100 µs | 0.0593 mV |

The finite 1 GΩ off-state switch in the settling deck also represents nanoampere-class off leakage and avoids an unrealistically infinite conductance ratio.

## Frozen firmware settling rule

Use the following conservative rule until physical measurements justify reducing it:

```text
after changing U_SRC or U_SENSE:
    wait 200 us
    discard the first ADC conversion
    then acquire the required averaged readings
```

Special capacitor sequence:

1. Select U_SENSE S1 and wait 200 µs before starting the capacitor operation.
2. Keep U_SENSE continuously on S1 during discharge and charge.
3. Change only U_SRC from S4 discharge to disabled, then to S2 charge.
4. Start the charge timer at the defined U_SRC-enable event and poll without inserting a new 200 µs delay.

This prevents the lower 100 nF range from losing 20% of its timing interval to an avoidable post-start delay.

## Inactive-network isolation conclusion

The architecture isolates the three socket groups as intended:

- U_SRC connects only to the R/C socket;
- diode and BJT drive GPIOs remain analog/no-pull outside their own modes;
- U_SENSE exposes only one of eight nodes to the buffer;
- off-channel loading is represented by high off resistance, 100 pF total selected-path capacitance, and the retained 2 nA leakage budget;
- calibration nodes remain available without directly loading DUT sockets;
- no inactive network changed a classification or pushed a supported range outside its error target.

Only one component should still be inserted at a time. Isolation is a safety and accuracy layer, not permission to test several installed components simultaneously.

## Phase 5B acceptance table

| Requirement | Result | Status |
|---|---:|---|
| Resistor 100 Ω–1 MΩ within ±2% modeled electronics target | −1.536% to +1.583% | PASS |
| Capacitor 100 nF–100 µF compatible with ±5% target | maximum center shift +0.052%; threshold window ≈±0.266% | PASS |
| Diode two-polarity separation preserved | 0.600 V forward / 3.300 V blocking | PASS |
| Diode short current below 2 mA | 1.570 mA | PASS |
| BJT active-region rules preserved | NPN and PNP both pass | PASS |
| BJT base remains buffered | TMUX1108 → OPA391 → PA0 | PASS |
| Calibration channels remain correct | codes 1364.96 and 2729.97 | PASS |
| Conservative common settling delay established | 200 µs | PASS |

## Limitations

- TMUX1104 and TMUX1108 are represented by conservative resistance, leakage, capacitance, and switch equivalents rather than complete vendor transient macro-models.
- The OPA391 macro-model represents typical modeled behavior, not every production corner.
- Protection components are not yet added; their leakage and capacitance are the subject of Phase 5C.
- A real PCB may have contamination leakage and coupling not represented here.
- Physical sockets, STM32 drive levels, VDDA noise, and actual component distributions remain unverified.

## Phase 5B conclusion

The two-multiplexer shared analog front end is accepted for continued design. It preserves all four component-measurement functions, reduces duplication, passes the numerical error/classification gates, and supports a simple 200 µs static-settling rule.

The next step is Phase 5C: select and validate socket/input protection, finalize decoupling and calibration components, audit the power/ground interface, and produce the preliminary consolidated BOM without adding leakage that breaks the verified ranges.

Official references:

- TMUX1104 datasheet: https://www.ti.com/lit/ds/symlink/tmux1104.pdf
- TMUX1108 datasheet: https://www.ti.com/lit/ds/symlink/tmux1108.pdf
- OPA391 product/model page: https://www.ti.com/product/OPA391
- STM32F401xB/xC datasheet: https://www.st.com/resource/en/datasheet/stm32f401cc.pdf
