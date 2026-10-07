# A8Duo CAS v1 — existing purple RP2040 cartridge

This U1 update adds read-only **standard boot cassette `.CAS` support** to the working A8Duo v021 firmware. Copy CAS files into either Library A or Library B and select them in the normal menu. No SIO cable, new PCB, or U2 update is needed. Existing CAR/ROM/XEX/ATR handling and the v021 built-in game remain present.

**Status:** release build, PAL/NTSC Atari800 integration tests, an independent published loader test, storage regressions, and critical-code comparison pass. This is a first hardware-test release; it has not yet been run on your physical Atari. CAS uses the inherited ATR RAM-OS mechanism, so start with your **64 KiB Atari 800XL/XL/XE**. CAS operation on a stock 16/48 KiB 400/800 is not claimed.

## Install

1. Keep a backup of Library A and Library B. Turn the Atari off and remove the cartridge before connecting USB.
2. Connect **U1 / Engine**, holding that module's BOOT button until `RPI-RP2` appears. Use the engine module, not U2 / Library B.
3. Copy `releases/A8Duo-CAS-v1-U1-Engine.uf2` onto `RPI-RP2`. Let the copy finish and the drive disconnect. **Do not run an erase program.**
4. Reconnect U1 normally, without holding BOOT, to access its existing `A8DUO` file drive. If needed, press the cartridge's reset button once. Make a `CAS` folder and copy `releases/CASTEST.CAS` into it. Eject the drive properly and disconnect USB.
5. Put the cartridge back into the powered-off Atari and switch on. Open **Library A → CAS → CASTEST.CAS**. It should display **A8DUO CAS TEST: PASS**. No START-key cassette boot procedure is needed; select the file normally.
6. Use the cartridge's reset/menu button to return to the menu. Copy your bootable CAS games into folders and select them the same way. You can also copy CASTEST.CAS to Library B using U2's normal USB file drive and test that library.

**Leave U2's installed firmware alone.** CAS only uses its existing sector-read protocol. The retained World's Hardest Game entry still requires the matching v021 game-capable U2 that was used previously.

This UF2 writes flash offsets `0x000000–0x017FFF` only. Each library starts at `0x100000`; its format, flash driver, USB behavior and allocation map are unchanged. This update is for the existing **A8Duo storage format**. It is not a drop-in migration image for an untouched original A8PicoCart filesystem, nor firmware for the Waveshare RP2350 board.

## Supported cassette files

- FUJI CAS containers with decoded standard `data` records, optional `baud` chunks, full/partial/EOF records, and correct checksums.
- Boot blocks followed by more standard cassette data, including loaders using the normal `C:` handler, `SIOV` cassette reads, or `RBLOKV`.
- Up to 4,096 records (at most 512 KiB of payload). Initial boot body: up to 256 × 128-byte records, loaded at `$0480` or above and ending by `$C000`.
- Accelerated loading: recorded pauses and baud timing are not reproduced. Files are read in place, with no converted files written into either library.

Not supported: FSK/PWM/turbo/protected tape signals, custom loaders that bypass the OS cassette interface, ordinary nonboot BASIC CSAVE files, raw headerless tape dumps merely named `.CAS`, cassette recording/saves, or custom `UNO_OS.ROM` during CAS loading. CAS always uses the bundled OS; normal ATR files retain their previous custom-OS behavior. Programs that replace the cassette vectors or overwrite the RAM OS need individual assessment.

An unsupported or malformed file produces a menu error. A failure after boot begins displays `CAS LOAD ERROR - RESET CART` when the loader returns an error; a program that hangs internally may require the cartridge reset button. An error does not call for erasing either library.

## Rollback and package

To undo this update, flash `baseline/ROLLBACK-v021-U1.uf2` onto U1 using the same BOOT procedure. That is the exact previous v021 U1 binary; keep your current U2 installed. No library erase is required.

- `firmware/`: complete modified fork; `baseline/firmware/` and `baseline/game/`: untouched v021 source.
- `cassette/`: new Atari assembly; `generated/`: assembled runtime and test transport.
- `docs/BUILD.md`, `docs/CHANGES.md`, `docs/VALIDATION.md`: build, change report and test limits.
- `tests/`, `validation/`, `evidence/`: reproducible checks, logs, ELF/map files, paired critical disassemblies.
- `baseline/A8PicoCart-*.tar.gz`: untouched original upstream tree at commit `fb0b36137a969a71b9258841c46676701125699d`.
- `CAS-vs-v021.patch`: this update's firmware diff. The older `firmware/engine.patch` is inherited historical material, not the CAS patch.

The supplied CASTEST.CAS is original diagnostic code, not a commercial game.
