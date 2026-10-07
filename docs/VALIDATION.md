# Validation and physical test procedure

## Completed on 2026-10-07

| Check | Result and scope |
|---|---|
| RP2040 Release build | Pass; GCC 13.2.1 20231009, Pico SDK 1.5.1, `-O3 -DNDEBUG`, inherited board/linker, 250 MHz setting retained. |
| UF2 validation | Every block's magic, sequence, family, size, address and payload checked; flash offset range `[0,0x18000)`; no library addresses. Boot2 bytes equal baseline. |
| Timing-critical comparison | 21 unchanged object sections/same addresses; 20 identical linked functions, one relocation-only difference. Menu dispatcher intentionally changes. |
| Parser and FatFs integration | ASan/UBSan pass; both real FAT volumes, full/partial/EOF records, count 0 interpreted as 256, malformed/truncated/unsupported/oversize/unsafe-address inputs rejected. Virtual-file reuse, menu dispatch, ordinary file fallback and custom-OS fallback checked. No storage writes during CAS operations. |
| Atari800 integration | Four passes: PAL/NTSC × Library A/B. Atari CPU/ANTIC executes the actual CAS runtime and bundled OS. Host calls the actual C parser, FatFs wrappers and inherited ATR functions. Boots at `$0700`; validates continuation, CASINI, DOSVEC, subsequent C: reads, partial records, EOF, raw cassette SIO, RBLOKV, rejected writes, and timeout beyond EOF. Diagnostic prints PASS. |
| Independent loader | BL/C 0.2 (1983, ML), published by AtariWiki, boots on PAL/NTSC. Test sends A, loader reads an appended synthetic XEX through C: and executes RUNAD. The wiki's raw boot body was explicitly wrapped in a standard FUJI container for this test; unconverted raw dumps are not supported. |
| Existing behavior regression | FatFs/menu and UART tests pass. Flash test: 91,392 sector writes, three complete fill/read/reboot cycles, address bounds and invalid-map checks pass. These writes occur in the host flash simulation, not on your cartridge. |
| Preserved game | All 31 source/data files unchanged. Host game-core tests pass for 30 levels, 20,178 enemy-position comparisons, state transitions and PAL/NTSC rate. CAS checks also confirm embedded-game virtual-file routing still works. No new physical gameplay result is claimed. |

The Atari800 bridge begins at the inherited OS-in-RAM/ATR launch state, and deliberately delays each command by 12,000 Atari cycles. It emulates the Atari CPU/ANTIC and real firmware's higher-level file logic; **it does not execute RP2040 machine code, model electrical GPIO timing, test USB hardware, reproduce the entire cartridge menu boot, or measure the physical UART latency**. Real SIO hardware and turbo waveform playback were not tested. No commercial-game compatibility list is claimed from these diagnostic results.

## First physical test

1. Use the known-working 64 KiB 800XL configuration, with USB disconnected. Record PAL/NTSC, OS revision and any RAM/OS upgrades. Keep existing library backups and the included v021 U1 rollback.
2. Install the U1 UF2 as described in README. Verify the menu and both libraries still appear. Launch one previously working CAR, XEX and ATR. Confirm the built-in game still starts with your existing matching v021 U2.
3. Select `CAS/CASTEST.CAS` in Library A. Expect the normal text display and `A8DUO CAS TEST: PASS`, with no cassette beep/START sequence. Repeat from Library B. Photograph any error or unexpected display.
4. Repeat each diagnostic from a cold power-on ten times; return using the cartridge's reset/menu button. Test the Atari's own RESET separately, since a program may install its own warm-start behavior.
5. Try a known standard boot CAS game and a standard multistage loader. Record exact filenames and SHA-256 hashes, whether the title appears, whether later levels load, and whether gameplay starts. A title screen alone does not prove a multiload game works.
6. Try one FSK/PWM/turbo fixture from `tests/cas-fixtures/`. Expect a menu error, then successful loading of a normal file without power cycling. Do not erase a library to fix a rejected format.
7. Reconnect each library's USB drive separately with the cartridge removed from the Atari. Compare stored file hashes with the pretest copies. The CAS reads should not alter the source images. Eject correctly before removing USB.

Repeat on a PAL and NTSC XL/XE where available. A stock 400/800 without usable RAM under the OS is not within this CAS path's current qualification. Existing CAR/XEX behavior on those systems is unchanged at source; verify their normal boot separately.

## Timing qualification for production

Use a high-impedance logic analyzer/oscilloscope on PHI2, S4/S5/CCTL, R/W, D0 and one bank-select address bit. Compare the v021 rollback and CAS build using identical hardware, clocks and cartridge images. Check data-valid margin before the Atari read sample, release to high impedance after each selected read, and absence of U1/U2 UART traffic during normal CAR emulation. Exercise every previously supported bank-switch family, especially Microcalc after a cold launch (its SDK call relocates), then repeated reset/cold-start cycles. During CAS/ATR transfers confirm the mailbox acknowledgment appears only after returned sector bytes are ready. No numeric physical timing margin or throughput is asserted by this software-only release.

## Performance measurements to record on hardware

Measure time from selecting CASTEST.CAS to PASS for Library A and B; repeat with a larger standard CAS near the supported record limit. Record size/record count and cold-versus-repeat timing, with the same PAL/NTSC machine. Selection scans and checksums the whole image before launching, so large files on Library B incur UART scanning time. Directory scans, USB transfer rate and flash endurance policy are unchanged. This update does not turn either library into a new USB device or change storage capacity.
