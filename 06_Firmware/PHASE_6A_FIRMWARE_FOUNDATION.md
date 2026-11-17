# Phase 6A — STM32 Firmware Foundation and Safe AFE Driver

## Completion status

Phase 6A is complete as a source-level STM32 HAL foundation. It converts the frozen Phase 5D pin map and truth table into reusable firmware, but it does not claim on-target execution before a real board or STM32 development target is available.

The implementation is under `Phase_6_Firmware/Core` and is intended to be copied into a CubeMX-generated STM32CubeIDE project for the STM32F401CCU6.

## Phase 6 subdivision

| Step | Purpose | Status after this work |
|---|---|---|
| 6A | Board pin definitions, microsecond timebase, reset-safe AFE state driver | Complete at source level |
| 6B | ADC1 acquisition, calibration-channel validation, averaging, timeouts | Complete at source level |
| 6C | Resistor and capacitor measurement algorithms | Complete at source level |
| 6D | Diode and known-pinout BJT algorithms with validity gates | Complete at source level |
| 6E | Rule-based diagnostic engine | Complete at source level |
| 6F | ST7735 UI, buttons, complete application state machine and logging | Complete at source level |

## Delivered source files

```text
Phase_6_Firmware/
└── Core/
    ├── Inc/
    │   ├── secdb_board.h
    │   ├── secdb_afe.h
    │   └── secdb_timebase.h
    └── Src/
        ├── secdb_afe.c
        ├── secdb_timebase.c
        └── phase6a_integration_example.c
```

`secdb_board.h` is the single pin-definition authority. In particular, it freezes `SRC_A1` on PA8 and contains no PB11 definition.

`secdb_afe.c` owns every measurement-related GPIO. Measurement algorithms must call its mode API and must not write mux or DUT-drive GPIOs directly.

`secdb_timebase.c` uses the Cortex-M4 DWT cycle counter for the 2 µs break-before-make delay, 200 µs analog settling delay, and future capacitor timing.

## Safety rules enforced in code

Every ordinary mode change calls `SECDB_AFE_EnterSafeTransition()` first. That function:

1. disables U_SENSE;
2. disables U_SRC;
3. waits 2 µs;
4. removes the BJT base drive first;
5. places all five DUT drive pins in analog/no-pull mode;
6. returns both mux addresses to zero.

The new mode is then applied with all sensing disabled. U_SENSE is enabled only after the selected source or drive state is complete. The driver waits 200 µs before returning control to an ordinary measurement caller.

The BJT output order is explicitly emitter, collector, and base. This prevents the base path from being energized before the emitter and collector paths are bounded.

## Capacitor timing exception

Capacitance timing cannot use an ordinary mode transition because the 200 µs sensing delay must occur before charging begins.

The enforced sequence is:

```c
SECDB_AFE_ApplyMode(SECDB_AFE_CAP_DISCHARGE);
/* Phase 6B: confirm voltage below threshold with a timeout. */
SECDB_AFE_ApplyMode(SECDB_AFE_CAP_READY);
SECDB_AFE_StartCapCharge(&charge_start_us);
```

`SECDB_AFE_StartCapCharge()` rejects the request unless `CAP_READY` is the current state. It leaves RC sensing connected, selects the 10 kΩ source, enables U_SRC, and records the start time without inserting a post-start delay.

## CubeMX project settings for integration

Create a project for `STM32F401CCU6`, then use these minimum settings:

- system clock: HSE crystal, initial target 84 MHz;
- debug: Serial Wire enabled;
- ADC1: channel IN0 on PA0, 12-bit, right aligned, software triggered;
- SPI1: master transmit-only, PA5 SCK and PA7 MOSI; keep TFT CS on PA4 as software GPIO;
- USB OTG FS device: PA9 VBUS sense, PA11 DM, PA12 DP when USB firmware is introduced;
- PC13, PC14 and PC15: button inputs; no LSE crystal;
- do not generate a conflicting PB11 assignment;
- preserve PA13/PA14 for SWD.

After `HAL_Init()` and `SystemClock_Config()`, initialize the timebase and AFE before enabling ADC, display, USB, or component-test tasks:

```c
if (!SECDB_Timebase_Init()) {
    Error_Handler();
}
SECDB_AFE_Init();
SECDB_AFE_ApplyMode(SECDB_AFE_CAL_1V1);
```

## Source-level verification completed

The implementation was checked against all 23 rows in `PHASE_5D_MODE_TRUTH_TABLE.csv`:

- U_SRC binary assignments are S1/S2/S3/S4 = `00/01/10/11`;
- U_SENSE binary assignments are S1 through S8 = `000` through `111`;
- resistor trial/low/high map to 10 kΩ/1 kΩ/100 kΩ;
- both diode polarities and both terminal-read states are represented;
- NPN and PNP E/B/C states and base-off confirmation states are represented;
- capacitor charge is callable only after the isolated-ready state;
- reset and transition states disable both muxes and tri-state all DUT drives;
- no code-level pin assignment uses PB11.

A native firmware build is not yet claimed because this folder is a HAL module rather than a generated CubeIDE project, and no ARM GCC/CubeIDE compiler is available in the current command environment. Compilation and on-target GPIO observation are mandatory when the CubeMX project is generated.

## Phase 6A acceptance gate

Phase 6A passes at source level. Phase 6B may now add ADC acquisition and calibration, provided it obeys these constraints:

- run both calibration reads before allowing a component measurement;
- reject measurements if calibration is out of range or non-monotonic;
- wait 200 µs and discard the first ADC conversion after every ordinary sense change;
- average subsequent ADC readings and enforce conversion timeouts;
- never sample PA0 while U_SENSE is disabled;
- always return the AFE to a safe state after success, rejection, or timeout.
