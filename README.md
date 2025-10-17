# NMEA-2k

## Overview

NMEA-2k provides a simple way to link between a boat’s data networks and converts NMEA 0183 into NMEA 2000 and vice versa.

The firmware is based on the [Zephyr RTOS](https://www.zephyrproject.org), allows turning
your microcontroller development board into a NMEA 0183 to NMEA 2000 convertor.

## About NMEA

The National Marine Electronics Association (NMEA) is a US-based marine electronics trade organization setting standards of communication between marine electronics.

NMEA 0183: A point-to-point serial data protocol allowing a single talker to transmit to multiple listeners, defined under IEC 61162-1. It is slowly being phased out in favor of the newer NMEA 2000 standard.

NMEA 2000: A multi-drop, Controller Area Network (CAN Bus)-based standard (250 kbps) standardized under IEC 61162-3 and heavily influenced by SAE J1939. Communication runs at 250 kbps and allows any sensor to talk to any display unit or other device.


## Architecture
```shell
┌─────────────────┐                                               ┌─────────────────┐
│  NMEA 2000      │ ◄──► CAN ◄──► Parser/Converter ◄──► UART ◄──► |   NMEA 0183     |
│  (CAN bus)      │                                               │  (RS232/RS485)  │ 
└─────────────────┘                                               └─────────────────┘
```

---
## Key Protocol Mappings

| Data Type     | NMEA 2000 (PGN) |  NMEA 0183  |
|---------------|-----------------|-------------|
| Heading       |  127250         |  HDT, HDG   |
| GPS Position  |  129029         |  GGA, RMC   |
| Speed/Course  |  129026         |  VTG, RMC   |
| Depth         |  128267         |  DPT        |
| Wind          |  130306         |  MWV, MWD   |
| AIS Data      |  129038-129810  |  VDM        |

---

## Hardware Requirements

Since the NMEA-2k firmware is based on the Zephyr RTOS, it requires Zephyr support for the
board it is to run on. The board configuration must support both an UART driver and at least
one CAN controller.

Check the list of [supported boards](https://docs.zephyrproject.org/latest/boards/index.html) in the
Zephyr documentation to see if your board is already supported. If not, have a look at the
instructions in the [board porting
guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html).

## Building and Running

Building the walidbadar firmware requires a proper Zephyr development environment. Follow the
official [Zephyr Getting Started
Guide](https://docs.zephyrproject.org/latest/getting_started/index.html) to establish one.

Once a proper Zephyr development environment is established, inialize the workspace folder (here
`workspace`). This will clone the walidbadar firmware repository and download the necessary
Zephyr modules:

```shell
west init -m https://github.com/walidbadar/NMEA-2k --mr main workspace
cd workspace
west update
```

Next, build the walidbadar firmware in its default configuration for your board (here
`stm32f3_disco`) using `west`:

```shell
west build -b stm32f3_disco NMEA-2k/app
```

After building, the firmware can be flashed to the board by running the `west flash` command.

## Related Projects
- **[canboat](https://github.com/canboat/canboat)** - Linux based applications for decoding NMEA 2000