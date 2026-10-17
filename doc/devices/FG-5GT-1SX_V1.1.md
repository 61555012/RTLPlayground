# FG-5GT-1SX_V1.1

Unmanaged 5 x 2.5GBASE-T + 1 x 10G SFP+ switch, PCB silkscreen `FG-5GT-1SX_V1.1`.

## Brands

- Ruiying RY-5GT-1SX (label on the case reads `TY-5GT-1SX`, model RY-5GT-1SX)

## Hardware

| Item         | Value                                              |
| ------------ | -------------------------------------------------- |
| Switch IC    | RTL8372N (4 x internal 2.5G PHY + 2 SerDes)        |
| 5th RJ45 PHY | RTL8221B, attached to SDS0                         |
| Flash        | PUYA P25Q32SH, 4 MByte                             |
| Ports        | 5 x RJ45 10/100/1000/2500 + 1 x SFP+ (1G/2.5G/10G) |
| Power        | 12 V / 1 A barrel jack, ~1.8 W idle                |
| Size         | 140 x 80 x 25 mm                                   |

Serial console of the OEM firmware is provided by the Realtek QC/diagnostic  
shell (the same SDK image as `FG-4GT-2SX_V2.0`), 9600 baud. It offers  
`rd <reg>` / `wd <reg> <val>` to read and write switch registers.

## Port mapping

Identical to the other RTL8372N 5+1 boards:

| Logical port | Physical | Function                   |
| ------------ | -------- | -------------------------- |
| 3            | 5        | 5th RJ45, RTL8221B on SDS0 |
| 4-7          | 1-4      | internal 2.5G PHYs         |
| 8            | 6        | SFP+ on SDS1               |

The OEM firmware's own diagnostics print `sds0:port3, sds1:port8`, which  
confirms SDS0 = RTL8221B and SDS1 = SFP+.

## What works

- All five 2.5GBASE-T RJ45 ports at 10/100/1000/2500 Mbps
- SFP+ port: 1G / 2.5G / 10G modules (needs the correct MOD-DEF0 GPIO, see below)
- LEDs (see below)

## LEDs

`leds_setup()` alone is not enough on this PCB: it leaves `LED_MODE`  
(0x6520) with bits 7 and 9 set, while the OEM firmware ends up with  
`LED_MODE` fields `[5..8] = 1` and `[9..12] = 2` and bit 17 ("LED solid on")  
set. `machine_custom_init()` for `MACHINE_FG_5GT_1SX_V1_1` applies that  
(set bit 17, clear bits 7 and 9) and enables the 12 LED pads below.

The PCB's pad-to-faceplate routing is **not** the SWTG024AS layout. The real  
mapping was recovered by disassembling the 4 MByte OEM flash and reading the  
custom LED-mux profile it writes (`0x65e0..0x65f4`, code `00c44e..00c725`):

| Faceplate port          | LED pads (green, amber) | mux values  |
| ----------------------- | ----------------------- | ----------- |
| Port 5 (RTL8221B, SDS0) | pad 0, 1                | `0x0c 0x0d` |
| Port 1 (internal PHY)   | pad 3, 4                | `0x10 0x11` |
| Port 2 (internal PHY)   | pad 6, 8                | `0x14 0x15` |
| Port 3 (internal PHY)   | pad 10, 12              | `0x18 0x19` |
| Port 4 (internal PHY)   | pad 18, 21              | `0x1c 0x1d` |
| SFP+                    | pad 26, 25              | `0x20 0x21` |

That profile's `LED_GLB_IO_EN` value is `0x3624155b`, which enables exactly  
pads `0,1,3,4,6,8,10,12,18,21,25,26` (plus SYS LEDs on bits 28/29). The  
remaining pads (`2,5,7,9,11,13..17,19,20,22,23,24,27`) are the per-port third  
/ redundant lamp and are unused on this board.

The stock firmware also writes `PIN_MUX_0 = 0x20db68bf` and  
`LED_GLB_IO_EN = 0x0f249740` / `0x0f24977f` in other profiles; the  
`0x3624155b` profile is the one that matches this board's wiring.

## SDS0 / the RTL8221B port

Port 5 (the RTL8221B) hangs off SDS0. `sds_init()` already applies the
N-chip **RX** PN swap for SDS0 whenever `isN && !n_10g && !isRTL8373`, which
is the case here:

```
sds_write_v(0, 6, 2, pval | 0x2000)   // RX PN swap, 64B/66B
sds_write_v(0, 0, 0, pval | 0x200)    // RX PN swap, 8B/10B
```

Do **not** add the SWTG024AS boards' extra SDS0 bits `0x100` / `0x4000` (TX
polarity) in `machine_custom_init()`. Those boards need them because their
PCB crosses the SDS0 pairs; this one does not. With them set, port 5 still
links up and lights its LEDs but transmits garbage, so the web UI is
unreachable through that port while ports 1-4 work fine.

## SFP

- SerDes: SDS1 (`sfp_port[0].sds = 1`)
- I2C: SDA = GPIO39 (I2C bus 4), SCL = GPIO40 (I2C bus 3)
- MOD-DEF0 (module present): `GPIO38` is what most RTL8372N 5+1 boards use and  
  is the default here. If a module is never detected (no `<MODULE INSERTED>`  
  on the console, `port stats` keeps showing `NO SFP`), switch  
  `SFP_DETECT_PIN` in the machine file to `GPIO30_ACL_BIT3_EN`, or use the  
  `gpio` console command while inserting and removing a module: the pin that  
  toggles is the detect pin.
- TX_DISABLE and LOS are not wired on this board.

## Flashing

4 MByte image, build with:

```
make MACHINE=FG_5GT_1SX_V1_1
```
