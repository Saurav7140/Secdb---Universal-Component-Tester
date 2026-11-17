# Phase 6F Application Test Matrix

These checks define the host-side and first-target acceptance behavior for the Phase 6F application layer. Hardware-dependent rows remain pending until the generated CubeIDE project and physical board exist.

## Startup and calibration

| ID | Stimulus | Expected result |
|---|---|---|
| S01 | App pointer is null | Initialization returns invalid argument |
| S02 | ST7735 is not initialized | App enters display-error state and refuses startup |
| S03 | Storage port is not bound | Safe default path calibration is loaded |
| S04 | Stored magic or version is wrong | Image is rejected and safe defaults remain active |
| S05 | Stored size is wrong | Image is rejected and safe defaults remain active |
| S06 | One stored byte changes | CRC fails and safe defaults remain active |
| S07 | CRC passes but a path value is implausible | Payload is rejected and defaults remain active |
| S08 | Valid stored image | Versioned values and sequence are retained |
| S09 | Startup completes | AFE is disabled and the screen asks for ADC calibration |
| S10 | MODE starts a passing ADC calibration | DUT interlock unlocks and app enters menu |
| S11 | ADC calibration fails | App stays on calibration screen and displays error code |

## Buttons and navigation

| ID | Stimulus | Expected result |
|---|---|---|
| N01 | Raw edge lasts less than 25 ms | No button event |
| N02 | Active-low edge remains for 25 ms | One press event |
| N03 | MODE remains held | No repeat events |
| N04 | UP/DOWN held less than 600 ms | No repeat event |
| N05 | UP/DOWN held beyond 600 ms | Repeat events occur no faster than every 150 ms |
| N06 | UP in menu | Selection advances and wraps after BJT PNP |
| N07 | DOWN in menu | Selection moves backward and wraps before resistor |
| N08 | MODE in resistor/capacitor menu | Nominal-reference editor opens |
| N09 | Select no reference | Tolerance editor is skipped |
| N10 | Select a nominal reference | Tolerance editor opens with 5.00% default |
| N11 | MODE after tolerance | Ready screen opens |
| N12 | MODE in diode menu without open calibration | Empty-socket confirmation screen opens |
| N13 | MODE in NPN/PNP menu | Part-specific gain editor opens |
| N14 | Select no BJT gain reference | Maximum editor is skipped |
| N15 | Select BJT gain minimum | Maximum choices are constrained to be at least the minimum |

## Measurement integration

| ID | Stimulus | Expected result |
|---|---|---|
| M01 | MODE on resistor ready screen | Resistor measure, diagnostic, log, and result render run in order |
| M02 | MODE on capacitor ready screen | Capacitor measure, diagnostic, log, and result render run in order |
| M03 | MODE on diode ready screen | Two-polarity diode result is normalized and rendered |
| M04 | MODE on NPN ready screen | Known-E/B/C NPN path is used |
| M05 | MODE on PNP ready screen | Known-E/B/C PNP path is used |
| M06 | Measurement begins | Display update completes before analog acquisition and status LED turns on |
| M07 | Measurement ends | AFE is safe before display or serial activity and status LED turns off |
| M08 | Valid measured value | Value and unit are displayed |
| M09 | Result has no valid value | No fabricated zero value is displayed |
| M10 | MODE on result screen | App returns to menu |
| M11 | UP on result screen | App returns to ready for an explicit retest |
| M12 | DOWN on result screen | App returns to menu |
| M13 | Logger is unbound or busy | Measurement result remains valid and log failure is recorded separately |

## Capacitor safety behavior

| ID | Stimulus | Expected result |
|---|---|---|
| C01 | ADC fails during charging | Charge path is removed and final discharge is attempted |
| C02 | Initial discharge or external-voltage check fails | Safety-severity diagnostic is shown |
| C03 | Final discharge cannot be confirmed | App enters safety lock |
| C04 | Any button is pressed in safety lock | Input is ignored |
| C05 | Clear is called without explicit user confirmation | Safety lock remains and confirmation-required is returned |
| C06 | Service layer confirms removal and discharge | AFE is forced safe and app returns to menu |

## Display and logging

| ID | Stimulus | Expected result |
|---|---|---|
| D01 | Bind a non-SPI1 handle | ST7735 initialization rejects it |
| D02 | SPI transfer fails | Display driver returns SPI failure |
| D03 | Text contains lowercase letters | Driver renders their uppercase glyphs |
| D04 | Text exceeds the line width | Rendering clips at the 128-pixel boundary |
| D05 | Fill rectangle exceeds display bounds | Invalid argument is returned |
| D06 | Render pass/caution/fault/safety result | Header color follows normalized severity |
| D07 | Write a resistor record | Raw trial/final codes and selected range are present |
| D08 | Write a capacitor record | Time, final raw code, discharge flag, and measurement status are present |
| D09 | Write a diode record | Both direction differences and all four terminal codes are present |
| D10 | Write a BJT record | E/B/C codes, junction voltages, currents, and base-off fields are present |

## Persistent calibration port

| ID | Stimulus | Expected result |
|---|---|---|
| P01 | Create default image | Phase 5C path constants populate and CRC is finalized |
| P02 | Save valid edited calibration | Sequence increments before write and CRC covers the new image |
| P03 | Payload validation fails before save | Storage write is not called |
| P04 | Storage write fails | Sequence and CRC return to the pre-save image |
| P05 | Load callback fails | Defaults remain active and I/O failure is reported |
| P06 | Use CRC implementation with ASCII `123456789` | Result equals standard CRC-32 value `0xCBF43926` |

## Source-level result

The C parser accepts all seven new Phase 6F translation units with project headers and HAL type stubs. Static integration checks confirm every component measurement, diagnostic, and logger path is reachable from the application source. Physical display offsets, SPI timing, USB CDC behavior, flash erase/write behavior, and button electrical behavior remain first-board tests.
