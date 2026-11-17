# Phase 7B4 - ST7735 Display Interface Capture Report

Date: 2026-09-13  
Project: SECDB Component Tester  
Capture method: KiCad 10 CLI-compatible connectivity-first schematic generation

## Outcome

Phase 7B4 completes the fourth hierarchical schematic sheet. The frozen board-side ST7735 interface, its local supply decoupling, and its backlight supply link are now included in the whole-project netlist.

This completes connectivity-first capture of all four planned schematic sheets:

1. USB-C input and power
2. STM32F401 core and user interface
3. Shared analog front end and component sockets
4. ST7735 display connector

## Captured circuit

| Reference | Frozen reference | Function | Value / footprint |
|---|---|---|---|
| J6 | J_TFT | Board-side display socket | 1x8, 2.54 mm female, vertical |
| R42 | R_TFT_BL | Display backlight supply link | 0 ohm fitted, 0603 |
| C21 | C_TFT_BULK | Local display bulk decoupling | 10 uF X5R, 0805 |
| C22 | C_TFT_HF | Local display high-frequency decoupling | 100 nF X7R, 0603 |

J6 uses `Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical`.

## Frozen board-side pin order

| J6 pin | Net | MCU connection / purpose |
|---:|---|---|
| 1 | GND | Ground |
| 2 | 3V3_D | Digital 3.3 V display supply |
| 3 | TFT_SCK | U1 PA5, pin 15 |
| 4 | TFT_MOSI | U1 PA7, pin 17 |
| 5 | TFT_CS | U1 PA4, pin 14 |
| 6 | TFT_DC | U1 PB1, pin 19 |
| 7 | TFT_RST | U1 PB0, pin 18 |
| 8 | TFT_BL | Backlight node from R42 pin 2 |

The physical PCB header order is frozen. If the purchased ST7735 module uses a different order, a correctly wired keyed cable or adapter must be used instead of changing the PCB net order.

## Automated validation

- KiCad parsed the complete five-page hierarchical project successfully.
- Whole-project ERC: **0 errors, 122 warnings**.
- Warning breakdown: 90 expected custom embedded-symbol library notices and 32 pre-existing off-grid endpoint warnings.
- Whole-project BOM and netlist exports completed successfully.
- Eight display-net assertions passed:
  - TFT_SCK = J6.3 and U1.15
  - TFT_MOSI = J6.4 and U1.17
  - TFT_CS = J6.5 and U1.14
  - TFT_DC = J6.6 and U1.19
  - TFT_RST = J6.7 and U1.18
  - TFT_BL = J6.8 and R42.2
  - 3V3_D includes J6.2, R42.1, C21.1 and C22.1
  - GND includes J6.1, C21.2 and C22.2

## Backlight decision

R42 is fitted as a 0 ohm link because the baseline assumes an ST7735 module with an onboard LED current limiter or backlight driver. Before connecting a purchased module, inspect its schematic or PCB. If its LED pin is bare and has no current-limiting element, replace R42 with a resistor calculated from the LED forward voltage and required current.

## Visual verification

The Phase 7B4 sheet and the complete five-page PDF were rendered and inspected for readable titles, references, pin labels, notes, borders, and absence of clipped or overlapping content.

## Remaining limitations

This is not yet fabrication-ready. The sheets intentionally use embedded connectivity symbols created for CLI capture. Phase 7C must replace or validate them against manufacturer/library symbols, confirm every package and pad number, align endpoints to the KiCad grid, and resolve or formally waive every ERC warning. The chosen physical ST7735 module must also be checked for supply voltage, connector orientation, pin order, and backlight circuitry.

## Next phase

Phase 7C - full schematic audit and cleanup:

1. Manufacturer symbol and datasheet pin-number audit
2. Footprint and package verification
3. Grid cleanup and ERC warning closure
4. Final schematic review gate before PCB placement and routing
