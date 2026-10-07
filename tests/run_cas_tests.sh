#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests validation
SRC="tests/cas_test_storage.c generated/atr_actual.c firmware/engine/cas_file.c firmware/engine/dual_menu.c firmware/engine/game_virtual.c firmware/engine/fatfs/ff.c firmware/engine/fatfs/ffunicode.c"
INC="-Itests/mocks -Ifirmware/engine -Ifirmware/engine/fatfs"
WRAP="-Wl,--wrap=f_mount,--wrap=f_open,--wrap=f_opendir,--wrap=f_readdir,--wrap=f_closedir,--wrap=f_read,--wrap=f_write,--wrap=f_lseek,--wrap=f_sync,--wrap=f_close"
cc -std=c11 -O1 -g -fsanitize=address,undefined $INC tests/cas_parser_test.c firmware/engine/cas_menu.c $SRC $WRAP -o build/tests/cas-parser
ASAN_OPTIONS=detect_leaks=0 build/tests/cas-parser tests/cas-fixtures > validation/cas-parser.log
S=${ATARI800_SRC:-../game-tools/atari800-7.2.1}
cc -std=c11 -O2 -DNEW_CYCLE_EXACT -I"$S/src" $INC tests/cas_atari800.c $SRC "$S/src/libatari800.a" $WRAP -lm -o build/tests/cas-atari800
for tv in ntsc pal; do
 for bank in 0 1; do
  build/tests/cas-atari800 tests/cas-fixtures generated/cas-rom.bin "-$tv" "$bank" > "validation/cas-$tv-$bank.log" 2>&1
  cat "validation/cas-$tv-$bank.log"
 done
done
