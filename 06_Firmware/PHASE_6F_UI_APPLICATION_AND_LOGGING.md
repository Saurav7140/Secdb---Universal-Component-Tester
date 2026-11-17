# Phase 6F — UI, Application State Machine, Calibration Storage, and Logging

## Completion status

Phase 6F is complete at source level. This finishes the planned Phase 6 firmware modules and connects the measurement and diagnostic layers to the three buttons, a 128×160 ST7735 display, versioned calibration storage, and a raw serial logging port.

The result is not target-compiled or hardware-validated. The workspace has no generated STM32CubeIDE project, ARM compiler, or physical board. Display offsets, flash-sector ownership, USB CDC operation, button behavior, and electrical timing must be validated on the real target.

## Delivered firmware

### Application and controls

- `Core/Inc/secdb_app.h`
- `Core/Src/secdb_app.c`
- `Core/Inc/secdb_buttons.h`
- `Core/Src/secdb_buttons.c`
- `Core/Src/phase6f_integration_example.c`

### Display

- `Core/Inc/secdb_st7735.h`
- `Core/Src/secdb_st7735.c`
- `Core/Inc/secdb_ui.h`
- `Core/Src/secdb_ui.c`

### Persistence and logging

- `Core/Inc/secdb_calibration_store.h`
- `Core/Src/secdb_calibration_store.c`
- `Core/Inc/secdb_log.h`
- `Core/Src/secdb_log.c`

The acceptance cases are recorded in `PHASE_6F_APPLICATION_TEST_MATRIX.md`.

## User workflow

The three active-low buttons are MODE on PC13, UP on PC14, and DOWN on PC15. External 10 kΩ pull-ups remain mandatory, and no LSE crystal may be fitted while PC14/PC15 are buttons.

The controls are consistent across screens:

- UP/DOWN changes a menu selection or editable value.
- MODE accepts the current choice and advances.
- MODE on the ready screen starts the test.
- MODE on a result returns to the component menu.
- UP on a result returns to the ready screen for an explicit retest.
- DOWN on a result returns to the menu.

Button inputs use a 25 ms debounce. UP and DOWN begin repeating after 600 ms and repeat at 150 ms intervals. MODE never repeats, which prevents a held confirmation button from accidentally advancing through multiple safety screens.

## Application states

| State | Purpose |
|---|---|
| ADC calibration | Requires empty sockets and validates the ADC before DUT access |
| Menu | Selects resistor, capacitor, diode, BJT NPN, or BJT PNP |
| Value reference | Selects an optional resistor/capacitor nominal value |
| Tolerance | Selects 1%, 2%, 5%, 10%, or 20% |
| BJT gain minimum | Selects no reference or a part-specific minimum |
| BJT gain maximum | Selects a maximum not below the chosen minimum |
| Diode open calibration | Requires an empty diode socket and records both blocking directions |
| Ready | Shows insertion, E/B/C, discharge, and unpowered-part instructions |
| Measuring | Prevents button handling while a blocking measurement runs |
| Result | Displays the normalized finding and action |
| Safety lock | Blocks all buttons after an unsafe capacitor discharge result |
| Display error | Forces the AFE safe and stops normal application control |

`SECDB_App_ProcessButtonEvents()` is public so host tests can inject button events without GPIO hardware. `SECDB_App_Tick()` is the production polling entry point.

## Safety behavior

The display is updated before analog acquisition starts. No TFT transfers occur during a measurement. Each measurement function returns the AFE to the safe state before result rendering or USB/serial logging begins.

An ADC error inside the capacitor charge loop was hardened during this phase. The code now removes the charge path and attempts final discharge rather than returning directly.

Initial-discharge failure or unconfirmed final discharge produces safety severity. The application then enters `SAFETY_LOCK`:

- all ordinary button input is ignored;
- the AFE remains disabled;
- the screen instructs the user to remove power and discharge the component;
- only `SECDB_App_ClearSafetyLock(..., true)` can clear it after an external layer has obtained explicit confirmation.

## Reference editing

The local UI uses bounded presets that stay inside Version 1 ranges:

- resistance: no reference or E12-like values from 100 Ω to 1 MΩ;
- capacitance: no reference or values from 100 nF to 100 µF;
- tolerance: 1%, 2%, 5%, 10%, or 20%;
- BJT gain: no reference or an expected range inside hFE 20–500.

Selecting no reference preserves the measured value and produces no health claim. The comparisons still use the exact Phase 6E integer rules.

## ST7735 driver

The display driver targets a 128×160 ST7735 on SPI1 with:

- PA4 chip select;
- PA5 SPI1 clock;
- PA7 SPI1 MOSI;
- PB0 reset;
- PB1 data/command.

It provides initialization, address-window writes, full-screen and rectangle fill, and fixed 5×7 text at scale 1 or 2. Lowercase input is rendered with uppercase glyphs, avoiding a large font table. Text is clipped at the display boundary and application messages are wrapped into 21-character lines.

The initial orientation uses MADCTL `0xC8`, RGB565 color, and zero X/Y offsets. ST7735 modules vary in controller revision, tab color, offsets, and BGR orientation. The first-board test must confirm these settings and adjust the driver constants if required.

Suggested CubeMX SPI1 settings are master mode, 8-bit data, MSB first, software NSS, clock idle low, first-edge sampling, and an initial clock no faster than 10 MHz. Increase speed only after signal integrity and display reliability are confirmed.

## Versioned calibration persistence

The calibration image contains:

- magic `0x53454344`;
- schema version 1;
- image size;
- monotonically incremented sequence;
- three calibrated resistor paths;
- capacitor timing denominator;
- BJT low/high drive, ADC full scale, base path, and collector path;
- diode ADC full-scale voltage;
- CRC-32 over the header and payload.

Invalid size, version, magic, CRC, or implausible values cause safe Phase 5C defaults to load. The CRC uses polynomial `0xEDB88320`; the standard `123456789` vector evaluates to `0xCBF43926`.

ADC two-point calibration and diode empty-socket calibration are deliberately repeated after startup rather than trusted from nonvolatile storage. They are sensitive to the current supply, ADC path, contact state, and temperature.

The storage module uses bound read/write callbacks. `phase6f_integration_example.c` intentionally returns false from its placeholder callbacks. Do not choose a flash address until the CubeIDE linker map, firmware size, erase-sector boundaries, write alignment, and power-loss strategy are known. A reserved internal-flash sector or external EEPROM can implement the same callback contract.

## Serial diagnostic logging

The logger uses one bound non-blocking write callback. It is suitable for a USB CDC wrapper such as `CDC_Transmit_FS` or for a UART transmit adapter. Logging failure is recorded separately and never changes a valid component result.

Each record includes sequence, component, measurement status, normalized diagnostic code, severity, confidence, value, unit, and device-specific raw fields:

- resistor: trial code, final code, selected range;
- capacitor: charge time, last raw code, discharge flag, original measurement status;
- diode: both direction differences, type, and all four terminal codes;
- BJT: polarity, E/B/C codes, junction voltages, currents, and base-off confirmation values.

No floating-point formatting is used. This keeps the logger compatible with a small embedded C library.

## CubeMX integration order

After CubeMX initializes the clock, GPIO, ADC1, SPI1, and optional USB device stack:

```c
SECDB_Timebase_Init();
SECDB_AFE_Init();
SECDB_ADC_Bind(&hadc1);
SECDB_Calibration_BindStorage(read_port, write_port);
SECDB_Log_Bind(cdc_or_uart_write_port);
SECDB_ST7735_Init(&hspi1);
SECDB_App_Init(&app);

for (;;) {
    SECDB_App_Tick(&app);
}
```

The application initially asks the user to remove all parts and press MODE for ADC calibration. DUT modes remain locked until that calibration passes.

## Verification completed

- Seven new C translation units pass syntax parsing with the project headers and HAL type stubs.
- All 12 application states are present in the source.
- Every resistor, capacitor, diode, and BJT measurement path reaches its Phase 6E diagnostic and device-specific logger.
- The safety-lock acknowledgement and safe-transition calls are present.
- Storage validation checks magic, version, size, CRC, and payload ranges.
- The standard CRC-32 reference vector passes.
- Button debounce/repeat constants and non-repeating MODE behavior are frozen.
- The 61-case application test matrix covers startup, navigation, measurement integration, safety, display, logging, and persistence.
- No stale PB11 pin assignment was introduced.

## Remaining hardware gates

Phase 6 is complete only at source level. Before the firmware can be called target-complete:

1. Generate the exact STM32F401CCU6 CubeIDE project and compile with warnings enabled.
2. Confirm stack and flash usage, especially the display and logging buffers.
3. Verify every GPIO transition and the capacitor charge timestamp with an oscilloscope or logic analyzer.
4. Validate ST7735 initialization, orientation, offsets, colors, and SPI clock.
5. Implement and power-fail-test the selected calibration storage port.
6. Verify USB CDC or UART logging under busy/disconnected conditions.
7. Execute the Phase 6B–6F vectors on the physical board.

## Next phase

Phase 7 is next: KiCad schematic capture and PCB engineering from the frozen Phase 5D netlist, with the firmware pin map used as a cross-check. Physical validation follows after manufacturing and assembly.
