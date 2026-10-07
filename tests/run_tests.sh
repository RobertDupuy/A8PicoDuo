#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests validation
cc -std=c11 -O1 -g -fsanitize=address,undefined -Itests/mocks -Ifirmware/engine -Ifirmware/engine/fatfs tests/menu_test.c firmware/engine/cas_file.c firmware/engine/dual_menu.c firmware/engine/game_virtual.c game/game_engine.c firmware/engine/fatfs/ff.c firmware/engine/fatfs/ffunicode.c -Wl,--wrap=f_mount,--wrap=f_open,--wrap=f_opendir,--wrap=f_readdir,--wrap=f_closedir,--wrap=f_read,--wrap=f_write,--wrap=f_lseek,--wrap=f_sync,--wrap=f_close -o build/tests/menu-test
cc -std=c11 -g -fsanitize=address,undefined -Itests/mocks -Ifirmware/shared tests/protocol_test.c -o build/tests/protocol-test
cc -std=c11 -D_GNU_SOURCE -DXIP_BASE=0x10000000u -O1 -g -fsanitize=undefined -Itests/mocks -Ifirmware/engine tests/flash_test.c firmware/engine/flash_fs.c -o build/tests/flash-test
ASAN_OPTIONS=detect_leaks=0 build/tests/menu-test > validation/menu-test.log
ASAN_OPTIONS=detect_leaks=0 build/tests/protocol-test > validation/protocol-test.log
build/tests/flash-test > validation/flash-test.log
cat validation/menu-test.log validation/protocol-test.log validation/flash-test.log
