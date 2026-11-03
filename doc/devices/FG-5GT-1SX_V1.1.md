# FG-5GT-1SX_V1.1

Following is documentation for unmanaged switch marked as `FG-5GT-1SX_V1.1`.

### Brands

| Brand   | Type       | Managed | PCB              | Flash       | Chip RTL      |
|---------|------------|---------|------------------|-------------|---------------|
| Ruiying | RY-5GT-1SX | No      | FG-5GT-1SX_V1.1  | not verified | 8372 + 8221B |

### What works

- Five 2.5GBASE-T RJ45 ports at 10/100/1000/2500 Mbps
- One SFP+ cage with 1G/2.5G/10G modules
- Per-port dual colour LEDs (green / amber)
- SFP module detect on GPIO38 and module EEPROM on I2C

### Not verified

- Flash chip model and size
- Serial console header location and pinout
- 10G operation on the SFP+ cage (module EEPROM reading is wired up, link
  behaviour at 10G has not been confirmed)

### PCB overview

**Board markings**

- Top silkscreen: FG-5GT-1SX_V1.1

**Key components**

- Main switch: RTL8372 (non-N), selected via `MACHINE_CHIPSET RTL8372`
- Port 5 is driven by a separate RTL8221B PHY, the four remaining copper ports
  are internal PHYs

### Port layout

| Physical port | Logical port | Type   | Notes          |
|---------------|--------------|--------|----------------|
| 1             | 4            | Copper | 2.5G           |
| 2             | 5            | Copper | 2.5G           |
| 3             | 6            | Copper | 2.5G           |
| 4             | 7            | Copper | 2.5G           |
| 5             | 3            | Copper | 2.5G, RTL8221B |
| 6             | 8            | SFP+   | SDS1           |

Logical port 3 is the RTL8221B port. It is the port the stock firmware uses for
management access, see the LED note below.

There is no reset button: the footprint is not populated, so the definition
uses `reset_pin = GPIO_NA`.

### SFP+

| Cage | Logical port | pin\_detect | pin\_los | pin\_tx\_disable | SerDes | I2C SDA                | I2C SCL                 |
|------|--------------|-------------|----------|------------------|--------|------------------------|-------------------------|
| 1    | 8            | GPIO38      | GPIO\_NA | GPIO\_NA         | SDS1   | GPIO39\_I2C\_SDA4      | GPIO40\_I2C\_SCL3\_MDC1 |

`pin_detect` is MOD-DEF0, the I2C bus is shared with the MDC1 pin group.

### LED configuration

The front panel has six ports with two LED pins each, twelve pads in total:
`0,1`, `3,4`, `6,8`, `10,12`, `18,21` and `25,26`. The pin to port mapping does
not follow the stock LED mux, so the board uses `led_mux_custom` with its own 28
entry `led_mux` table.

| Port      | Set | Green                | Amber            |
|-----------|-----|----------------------|------------------|
| RJ45 1-5  | 0   | 2.5G link/activity   | 10/100/1000 link |
| SFP+      | 1   | 100M-2.5G link       | 10G link         |

`machine_custom_init()` sets the twelve `LED_GLB_IO_EN` bits one by one instead
of writing the register in one go: a full write clears the reset defaults and
takes down logical port 3 (RTL8221B) together with the management access through
it. The same hook enables `LED_MODE` bit 17 and clears bits 7 and 9 to match the
OEM LED profile.

### Machine target

```
make MACHINE=RUIYING_RY_5GT_1SX
```

The board definition lives in `machine/pcb/FG_5GT_1SX_V1_1.c` with the init hook
in `machine/pcb/FG_5GT_1SX_V1_1_init.c`, both linked from `machine/` as
`RUIYING_RY_5GT_1SX.c` and `RUIYING_RY_5GT_1SX_init.c`. The model string shown in
the UI and CLI is `RY-5GT-1SX`.
