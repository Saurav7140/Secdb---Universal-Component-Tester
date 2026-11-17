# SECDB Phase 7A — KiCad Net-Naming Rules

Date: 2026-09-13  
Status: Phase 7A planning baseline; no detailed schematic capture has begun.

## Authority and invariants

The electrical naming authority is `PHASE_5D_MASTER_NETLIST.csv`, reconciled with `PHASE_5D_MASTER_SCHEMATIC_SPECIFICATION.md` and the Phase 6 `secdb_board.h` pin map.

- Names are uppercase ASCII with underscores.
- Existing frozen names are copied exactly; aliases and silent renames are prohibited.
- `SRC_A1` is connected to PA8, UFQFPN48 package pin 29.
- PB11 is unavailable and must not appear as a pin assignment or net alias.
- `ADC_BUFFERED` is the only measurement ADC net and terminates at PA0 / ADC1_IN0.
- `3V3_D` and `3V3_A` remain distinct nets separated by `FB_A`.
- GND is one continuous ground system; no `AGND` split is introduced.
- Connector-side, protection-side, and MCU-side USB nets remain distinct.
- Unconnected pins use KiCad no-connect markers; `NC`, `NC_SPARE`, and `NC_ESD_SPARE` are audit states, not copper net labels.

## Label scope

- Use global labels only for frozen cross-sheet rails and named cross-sheet signals.
- Use local labels for short intra-sheet nodes such as `RC_INTERNAL`, `SENSE_COM`, and `OPA_OUT` when they do not cross a sheet boundary.
- Do not create hierarchical-sheet pins during Phase 7A. During detailed capture, add a sheet pin only where an explicit wire-through hierarchy is clearer than a frozen global label.
- Never rely on unlabeled long wires to convey design intent across functional blocks.

## Frozen canonical net vocabulary

### Rails and power path

`GND`, `USB_VBUS_RAW`, `USB_VBUS_FUSED`, `5V_DIODE`, `LDO_IN`, `3V3_D`, `3V3_A`, `VCAP_1`

### USB

`USB_CC1`, `USB_CC2`, `USB_DP_CONN`, `USB_DP_ESD`, `USB_DP_MCU`, `USB_DM_CONN`, `USB_DM_ESD`, `USB_DM_MCU`, `USB_VBUS_SENSE`

### MCU core, clock, reset, debug, and UI

`BOOT0`, `NRST`, `HSE_IN`, `HSE_OUT`, `HSE_XTAL_OUT`, `SWDIO`, `SWCLK`, `STATUS_LED`, `BTN_MODE`, `BTN_UP`, `BTN_DOWN`

### Display

`TFT_SCK`, `TFT_MOSI`, `TFT_CS`, `TFT_DC`, `TFT_RST`, `TFT_BL`

### Source multiplexer and RC path

`SRC_EN`, `SRC_A0`, `SRC_A1`, `SRC_1K`, `SRC_10K`, `SRC_100K`, `SRC_DISCHARGE`, `RC_INTERNAL`, `RC_SOCKET`

### Sense multiplexer and ADC chain

`SNS_EN`, `SNS_A0`, `SNS_A1`, `SNS_A2`, `SNS_RC`, `SNS_DIODE_A`, `SNS_DIODE_B`, `SNS_BJT_E`, `SNS_BJT_B`, `SNS_BJT_C`, `SNS_CAL_1V1`, `SNS_CAL_2V2`, `SENSE_COM`, `OPA_OUT`, `ADC_BUFFERED`

### DUT drive and exposed terminals

`DIODE_A_GPIO`, `DIODE_A`, `DIODE_B_GPIO`, `DIODE_B`, `BJT_E_GPIO`, `BJT_E`, `BJT_B_GPIO`, `BJT_B`, `BJT_C_GPIO`, `BJT_C`

### Calibration

`CAL_1V1`, `CAL_2V2`

## Differential-pair convention

KiCad's automatic `_P` / `_N` pairing convention is not applied because the frozen names are `USB_DP_*` and `USB_DM_*`. The PCB phase must assign the paired segments explicitly and verify physical matching across `*_CONN`, `*_ESD`, and `*_MCU` sections.

## Reserved naming policy

- New test-point-only labels, if later required, use `TP_<FROZEN_NET>` without renaming the underlying net.
- New intentional tuning links use `LINK_<FUNCTION>` as the reference-purpose field, not as a replacement net name.
- Any new electrical net requires a documented Phase 5D deviation review before capture.

