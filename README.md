# Smart Electronic Component Diagnostic Board (SECDB)

SECDB is an STM32F401CCU6-based tester for disconnected, unpowered electronic
components. Version 1 supports resistors, capacitors, diodes, and known-pinout
NPN/PNP BJTs through a shared protected analog front end.

## Repository contents

- `01_LTspice_Verified/` - independent measurement-principle simulations
- `02_Documentation/` - engineering reports
- `03_Range_Design/` - range selection and error budgets
- `04_Virtual_Breadboard/` - integrated virtual validation
- `05_Shared_Analog_Front_End/` - consolidated AFE design
- `06_Firmware/` - STM32 source-level firmware
- `07_KiCad/` - schematics, PCB, audits, and verified routing checkpoints
- `Mini project/` - earlier project material retained for reference

Simulation outputs, firmware acceptance vectors, KiCad DRC/ERC reports, and
intentional PCB checkpoint backups are committed as engineering evidence.

## Current hardware status

The PCB is preserved at the audited Phase 8D2B two-layer checkpoint with zero
KiCad DRC violations and 52 remaining unconnected items. Phase 8D3 identified a
routing-density decision between a coordinated two-layer MCU re-fan and a
four-layer conversion. See
[`07_KiCad/PHASE_8D3_ROUTABILITY_REVIEW.md`](07_KiCad/PHASE_8D3_ROUTABILITY_REVIEW.md).

The board is not fabrication-ready. Physical calibration and validation remain
required before relying on measurement accuracy or diagnostic classifications.

## Planning documents

`PHASE_PLAN.md` and `PROJECT_STATUS_AND_FILE_INDEX.md` are retained because they
document the project roadmap and earlier decisions. They are historical planning
records; the latest checkpoint reports under `07_KiCad/` are authoritative for
the current PCB state.

