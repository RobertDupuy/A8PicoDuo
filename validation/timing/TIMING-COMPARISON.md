# Timing-critical release comparison

GCC 13.2.Rel1; Pico SDK 1.5.1; Release (-O3). Baseline is the unchanged A8Duo HardGame v021 source rebuilt with the same tools (C linker selected for both builds).

| Function | Bytes | Object bytes identical | Linked bytes identical | Relocations normalized | SRAM address |
|---|---:|---|---|---|---|
| `emulate_boot_rom` | 196 | True | True | True | `0x200000c0` |
| `emulate_standard_8k` | 72 | True | True | True | `0x20000184` |
| `emulate_standard_16k` | 132 | True | True | True | `0x200001cc` |
| `emulate_XEGS_32k` | 256 | True | True | True | `0x20000250` |
| `emulate_XEGS_64k` | 248 | True | True | True | `0x20000350` |
| `emulate_XEGS_128k` | 256 | True | True | True | `0x20000448` |
| `emulate_bounty_bob` | 328 | True | True | True | `0x20000548` |
| `emulate_atarimax_128k` | 164 | True | True | True | `0x20000690` |
| `emulate_williams` | 164 | True | True | True | `0x20000734` |
| `emulate_OSS_B` | 232 | True | True | True | `0x200007d8` |
| `emulate_OSS_A` | 232 | True | True | True | `0x200008c0` |
| `emulate_megacart` | 288 | True | True | True | `0x200009a8` |
| `emulate_SIC` | 296 | True | True | True | `0x20000ac8` |
| `emulate_SDX` | 240 | True | True | True | `0x20000bf0` |
| `emulate_diamond_express` | 168 | True | True | True | `0x20000ce0` |
| `emulate_blizzard` | 160 | True | True | True | `0x20000d88` |
| `emulate_turbosoft` | 188 | True | True | True | `0x20000e28` |
| `emulate_atrax` | 164 | True | True | True | `0x20000ee4` |
| `emulate_microcalc` | 148 | True | False | True | `0x20000f88` |
| `emulate_phoenix_8k` | 112 | True | True | True | `0x2000101c` |
| `feed_XEX_loader` | 180 | True | True | True | `0x2000108c` |
| `atari_cart_main` | 938 | False | False | False | `0x20001140` |

The comparison includes literal pools. Relocation normalization zeros only locations declared by the compiler relocation table; it does not delete instructions or normalize arbitrary differences. Full disassemblies and SHA-256 values accompany this report. `atari_cart_main` changes at source and instruction level only to recognize CAS and call the outlined file-selection/OS-copy helpers. It is the menu dispatcher, not a cartridge polling routine; no equivalence is claimed for it. `emulate_microcalc` differs only in the relocated BL to the unchanged SDK division veneer; this bank-switch path needs scope validation, including XIP cache-cold behavior. The other 20 bus/loader routines are byte identical, including their literal pools.

This establishes equivalence to the controlled A8Duo v021 rebuild, **not** physical timing qualification. This CAS update changes no PCB or GPIO mapping. The original A8PicoCart published UF2 is a different baseline; no binary identity with it is claimed.
