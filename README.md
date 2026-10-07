# A8PicoDuo

**An Atari 8-bit cartridge built around two RP2040 modules, with 32 MiB of installed flash and support for RP2040-assisted software.**

A8PicoDuo extends [Robin Edwards' A8PicoCart](https://github.com/robinhedwards/A8PicoCart) with a second purple RP2040 module on a cartridge carrier PCB. Each module has its own 16 MiB flash. The first module connects to the Atari cartridge bus; the second communicates with it over a serial link.

The project provides more space for games and files while retaining the original Atari cartridge interface. The second processor can also perform computation for games and other Atari software. A working prototype has demonstrated both libraries and an RP2040-assisted game on an Atari 800XL.

## Hardware

| Component | Role |
|---|---|
| U1 — Atari-facing RP2040 | Services the cartridge bus and accesses its local flash. |
| U2 — companion RP2040 | Provides additional storage and can run application or game logic. |
| Carrier PCB | Routes the Atari edge connector, interprocessor link, power and reset connections. |
| Shared reset button | Pulls both modules' RUN inputs low. |

Both modules are the purple RP2040 boards with 16 MiB onboard flash. Each RP2040 has its own 264 KiB SRAM. Their memories are separate: the board does not create a shared RAM pool or a single memory-mapped 32 MiB flash device.

The carrier retains the original cartridge PCB outline. Enclosure clearance also depends on module placement, header height and the modules' USB-C connectors.

## Connections for firmware developers

U1 retains the original A8PicoCart Atari GPIO assignments:

| U1 GPIO | Atari signal |
|---|---|
| GP0–GP12 | A0–A12, respectively |
| GP13–GP20 | D0–D7, respectively |
| GP21 | CCTL |
| GP22 | PHI2 |
| GP23 | R/W |
| GP24 | S4 |
| GP25 | S5 |
| GP26 | RD4 |
| GP27 | RD5 |

The interprocessor connections are:

| Connection | Direction when configured as UART |
|---|---|
| U1 GP28 → U2 GP1 | U1 UART0 TX → U2 UART0 RX |
| U2 GP0 → U1 GP29 | U2 UART0 TX → U1 UART0 RX |
| Shared ground | Common signal reference |
| U1 RUN and U2 RUN → reset button → ground | Resets both modules |

U2 has no direct connection to the Atari address or data bus. Information exchanged with U2 passes through U1. Any real-time Atari bus response must therefore be handled by U1, with data already available when needed.

**The wiring is a hardware constraint; the communication protocol, baud rate, flash partitioning and division of work are firmware choices.** Developers writing replacement firmware can define their own use of both processors.

The supplied reference firmware uses UART0 at 3 Mbaud, 8N1, with no hardware flow control. This gives a theoretical serial byte rate of 300 kB/s before packet overhead and processing time. It uses synchronous requests; these transactions do not run in the ordinary cartridge-emulation polling loops.

## Reference firmware

The included implementation presents two folders in the Atari menu: **Library A** on U1 and **Library B** on U2. Each USB connection provides file access to that module's library.

With this firmware, each library exposes 30,464 sectors of 512 bytes: **14.875 MiB per library, 29.75 MiB combined before FAT overhead**. The rest of the installed flash is reserved for firmware and storage management. Firmware occupies the first 1 MiB of each module's flash allocation.

Supported file handling includes CAR, ROM, XEX and ATR, subject to the inherited format and memory limits. The current source also includes standard boot CAS loading and a World's Hardest Game adaptation demonstrating computation on U2 with Atari-side input and display.

Ordinary cartridge images use the original A8PicoCart cartridge-emulation routines. File management, companion communication and application features are implemented around that engine. Timing-critical changes require release-build comparison and physical Atari testing.

Feature availability and computer compatibility depend on the installed firmware and the selected software. Prototype operation on an 800XL does not qualify every format or every Atari model. See [validation notes](docs/VALIDATION.md) for tested behavior and remaining hardware checks.

## Building and programming

See [BUILD.md](docs/BUILD.md) for the supplied firmware's toolchain and build instructions. The reference build uses Pico SDK 1.5.1, GCC 13.2.1 and Release optimization; U1 is configured for 250 MHz. These are reference-firmware settings, not requirements imposed by the carrier PCB.

Each module is programmed through its own USB port: hold that module's BOOT button while connecting USB, then copy the appropriate UF2 to `RPI-RP2`. Use firmware intended for the correct module. The currently packaged U1 update assumes an existing compatible U2 installation; U2 source is in `firmware/companion/`.

Remove the cartridge from the Atari before connecting USB, and use one USB connection at a time. Back up libraries before changing firmware or storage formats. Routine updates to the supplied firmware do not require a library erase.

## Repository layout

| Directory | Contents |
|---|---|
| `firmware/engine/` | U1 cartridge engine, storage integration and menu code |
| `firmware/companion/` | U2 storage server and application integration |
| `firmware/shared/` | Reference interprocessor protocol and transport |
| `game/` | RP2040-assisted game implementation and Atari frontend |
| `cassette/`, `generated/` | Atari cassette adapter and generated assets |
| `docs/` | Build instructions, change reports, technical references and validation scope |
| `tests/`, `tools/` | Test harnesses and development tools |
| `releases/` | Packaged firmware and diagnostics |
| `baseline/` | Preserved earlier source, upstream archive and rollback firmware |
| `validation/`, `evidence/` | Build/test results and binary-comparison evidence |

This repository contains firmware and development evidence. The current package does not include the A8PicoDuo carrier's editable KiCad design or manufacturing files; the original upstream hardware archive is not the dual-module carrier design.

## Credits

A8PicoDuo builds on **A8PicoCart by Robin Edwards**. The preserved upstream revision is `fb0b36137a969a71b9258841c46676701125699d`.

See [SOURCES.md](docs/SOURCES.md) for third-party attribution, inherited notices and licensing information.
