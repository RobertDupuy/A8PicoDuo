# Build and reproduce checks

Build from the extracted package root. Use **Pico SDK 1.5.1**, commit `6a7db34ff63345a7badec79ebea3aaef1712f374`, with its TinyUSB submodule. Release artifacts used Arm GNU Toolchain **13.2.1 20231009**, binutils 2.42, CMake 3.28.3, newlib 4.4.0, and cc65 **2.19** (`ca65`/`ld65`). Python 3 and a host C compiler are needed for tests; `pyelftools` 0.30 is needed for ELF comparison. No Python dependencies are required by the CAS generators themselves.

## Firmware

Install `arm-none-eabi-gcc`, its newlib and C++ headers, CMake, make, and cc65; put their executables on PATH. Obtain the SDK:

```sh
git clone --branch 1.5.1 https://github.com/raspberrypi/pico-sdk.git
git -C pico-sdk submodule update --init lib/tinyusb
export PICO_SDK_PATH="$PWD/pico-sdk"
sh tools/build_release.sh
```

The script assembles the new Atari runtime, verifies that it only occupies unused bundled-OS filler, builds U1 in Release mode, and creates the diagnostic CAS. Existing Atari menu/OS/game headers are included and do not need regeneration. `CA65` and `LD65` may point to alternate absolute assembler paths.

Do not use a Debug build on the cartridge. Do not change the GPIO map, clock, board header, pinned linker layout, or SDK/toolchain while claiming the included timing comparison still applies. A different toolchain must be compared again and tested physically.

U2 is not rebuilt or updated for CAS. Its complete unchanged source is included for provenance.

## Controlled baseline and binary comparison

```sh
cmake -S baseline/firmware/engine -B build/baseline -DCMAKE_BUILD_TYPE=Release
cmake --build build/baseline -j4
python3 tools/compare_engine.py baseline build/baseline build/engine validation/timing
python3 tools/verify_release.py
```

The first comparator argument is retained for compatibility with the inherited tool interface; the actual comparison reads the two builds. Both must use the same compiler/SDK. Results include literal pools and compiler-declared relocations, plus per-function disassemblies. Absolute build paths/debug metadata may differ across machines; compare function bytes and UF2 address/content checks rather than assuming ELF files will hash identically.

## Host and Atari tests

```sh
python3 tools/make_cas_fixtures.py
sh tests/run_tests.sh
```

The existing menu/protocol tests use AddressSanitizer and UndefinedBehaviorSanitizer. The flash test uses a simulated flash address range and UBSan. On Linux this requires a host compiler with those sanitizers.

Obtain an isolated Atari800 **7.2.1** source tree from https://github.com/atari800/atari800 and its normal configure/make build prerequisites. Do not point the following at your personal emulator checkout: the preparation script adds test hooks to `cartridge.c` and `memory.h`.

```sh
export ATARI800_SRC=/absolute/path/to/atari800-7.2.1
python3 tools/prepare_atari800.py
sh tests/run_cas_tests.sh
python3 tests/test_published_loader.py
```

The optional last test downloads the independently published BL/C 0.2 raw boot body and wraps it in standard CAS records; that third-party payload is not shipped here. Network access is required on the first run. It checks PAL/NTSC cassette boot, C: binary loading, and RUNAD execution.

The unchanged game core can be checked with:

```sh
cc -std=c11 -O2 -shared -fPIC -DHG_TEST game/game_engine.c -o build/game.so
python3 game/tests/test_original.py
```

The inherited `game/tests/run_atari800_test.py` is preserved as historical v021 test code; it does not link the new CAS adapter and is not this update's test runner. Use the CAS runner above for the new wrapper integration. Logs under `validation/` describe the tests actually run for this release.
