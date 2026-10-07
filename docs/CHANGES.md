# File-by-file CAS change report

Baseline: the complete A8Duo HardGame v021 package previously supplied for this cartridge. Its underlying A8PicoCart revision is `fb0b36137a969a71b9258841c46676701125699d`. The baseline source and exact v021 U1 rollback UF2 are included unchanged.

| Firmware file | Change and reason |
|---|---|
| `engine/atari_cart.c` | Adds CAS to the filename filter; moves existing file-selection and OS-copy logic into helpers that can choose the CAS path. The `atari_cart_main` menu dispatcher changes. No polling routine, cartridge type implementation, GPIO definition or ATR sector function changes. |
| `engine/dual_menu.c` | Recognizes the selected CAS file as a virtual read-only ATR stream and clears stale virtual-file identity when a FIL object is reused. |
| `engine/game_virtual.c` | Existing FatFs wrappers route CAS reads/seeks/closes to the new adapter; CAS writes return write-protected. Ordinary-file and embedded-game handling stay on their prior paths. |
| `engine/CMakeLists.txt` | Compiles the two new C modules. The release flags, SDK setup, board and linker remain unchanged. |
| `engine/cas_file.c` — new | Validates FUJI chunks/records/checksums/boot bounds; indexes up to 4,096 records; streams and normalizes them through a virtual ATR; installs CAS-only OS additions. |
| `engine/cas_file.h` — new | Adapter and menu interfaces. |
| `engine/cas_menu.c` — new | CAS-aware file selection and OS-copy dispatch. These menu-only helpers run from flash to preserve pinned bus-code/buffer locations. |

Four existing firmware files change; three are added. **45 existing firmware files and all 31 game files are byte-identical to v021.** `validation/file-changes.json` records the list and hashes.

Specifically unchanged: `main.c`, `atari_cart.h`, every GPIO mask/group, `myboard.h`, both linker files, `rom.h`, `osrom.h`, original Atari boot ROM assembly, flash filesystem, FatFs, USB mass storage, UART block client/protocol, and the entire U2 source. The original cartridge ROM and OS files are not regenerated. For CAS only, a copy of the bundled OS gains the new cassette reader in unused filler; ordinary ATR uses the original OS copy procedure, including UNO_OS.ROM override.

## New non-firmware-tree files

`cassette/runtime.s` and `.cfg` implement a 627-byte Atari-side standard cassette adapter at `$C6A0–$C912`. `tools/build_cas_runtime.py` verifies the target region was filler and checks the original vector before generating `generated/cas_runtime.h` and the test ROM. At boot the adapter redirects cassette SIO and C: open, reuses the existing OS byte/block reader, loads the cassette header's requested address, runs its continuation and initialization, then enters DOSVEC. Writes are rejected. Disk SIO is forwarded to the original handler.

The remaining new scripts/fixtures are build, validation and packaging tools; they are not installed onto either RP2040. No MeanHamster or a8-pico-sio source was imported. `docs/SOURCES.md` records the public references and retained notices.

## Timing and storage

CAS follows the same synchronous command gap as an ATR sector read. While U1 reads a record from local flash or U2, the Atari runs its RAM OS and polls the existing mailbox. U1 returns to the unchanged cartridge service routine before acknowledging completion. CAS adds no background worker, ISR, core-1 task, flash write, or asynchronous storage work. It never services storage concurrently with ordinary cartridge polling. As with the inherited ATR path, a program's custom interrupt routine that tries to execute cartridge ROM during a command gap is outside the supported loader model.

The compiled comparison uses the same GCC 13.2.1, SDK 1.5.1, Release configuration for old and new code. **21 bus/loader functions retain identical object-section bytes and SRAM addresses; 20 also retain identical linked bytes.** `emulate_microcalc` differs only by a relocated branch to the SDK division veneer, with all nonrelocation bytes identical. `atari_cart_main` is intentionally different and is excluded from that equivalence claim. This does not establish electrical timing on hardware.

Pinned addresses remain `A8PicoCart_rom=0x20002340`, `cart_d5xx=0x20005A60`, `cart_ram=0x20005B60`. New image: 98,088 binary bytes; UF2 writes 98,304 padded bytes. Static RAM ends at `0x2003D850`, with 10,160 bytes to the end of the main 256 KiB RAM area, including the existing 2 KiB heap reserve. The existing 2 KiB core-0 stack remains in scratch Y (`0x20041800–0x20042000`). CAS indexes account for 20 KiB; selected images are streamed, not fully resident in RP2040 RAM.

See `validation/timing/TIMING-COMPARISON.md` and paired disassemblies for exact evidence. Equivalence is to the controlled v021 rebuild, not to the separately compiled original upstream UF2.
