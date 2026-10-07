#!/usr/bin/env python3
"""Reproduce full-Atari tests with an isolated Atari800 7.2.1 source checkout."""
from pathlib import Path
import os,re,subprocess
from PIL import Image
R=Path(__file__).resolve().parents[2];S=Path(os.environ['ATARI800_SRC']).resolve();B=R/'build/tests';V=R/'validation'
B.mkdir(parents=True,exist_ok=True);V.mkdir(exist_ok=True)
assert '7.2.1' in (S/'configure.ac').read_text()
p=S/'src/cartridge.c';s=p.read_text()
if 'extern UBYTE hg_cart_read' not in s:
 old='UBYTE CARTRIDGE_GetByte(UWORD addr, int no_side_effects)\n{';assert old in s
 s=s.replace(old,'extern UBYTE hg_cart_read(UWORD addr);\nextern void hg_cart_write(UWORD addr,UBYTE byte);\n'+old+'\n return hg_cart_read(addr);')
 old='void CARTRIDGE_PutByte(UWORD addr, UBYTE byte)\n{';assert old in s
 s=s.replace(old,old+'\n hg_cart_write(addr,byte);return;');p.write_text(s)
p=S/'src/memory.h';s=p.read_text()
if 'hg_monitor_write' not in s:
 s=s.replace('#include "atari.h"','#include "atari.h"\nvoid hg_monitor_write(UWORD addr, UBYTE byte);')
 s,n=re.subn(r'(#define MEMORY_PutByte\(addr, byte\)\s+do \{) ',r'\1 hg_monitor_write(addr,byte); ',s);assert n==1
 s,n=re.subn(r'(#define MEMORY_PutByte\(addr,byte\)\s+\()',r'\1hg_monitor_write(addr,byte), ',s);assert n==1
 p.write_text(s)
def run(args,cwd,log):
 result=subprocess.run(args,cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 (V/log).write_bytes(result.stdout)
 result.check_returncode()
if not os.environ.get('ATARI800_SKIP_BUILD'):
 run(['./configure','--target=libatari800','--disable-maintainer-mode','--disable-netsio','--disable-rdevice','--disable-ide','--disable-pokeyrec'],S,'atari800-config.log')
 run(['make','-C','src','clean'],S,'atari800-clean.log')
 run(['make','-C','src','-j4','libatari800.a','CFLAGS=-O2 -g -DNEW_CYCLE_EXACT'],S,'atari800-build.log')
# The library target selects cycle-map objects but this release's configure does
# not define NEW_CYCLE_EXACT for it; explicitly select that supported core mode.
# -Werror is not used: release headers redefine ULONG to equivalent uint32 types.
b=bytes(int(x,16) for x in re.findall(r'0x([\da-fA-F]{2})',(R/'firmware/engine/osrom.h').read_text()));assert len(b)==16384
(B/'osrom.bin').write_bytes(b)
labels={line.split()[2][1:]:int(line.split()[1],16) for line in (R/'game/generated/game.lbl').read_text().splitlines()}
(B/'atari_labels.h').write_text(''.join(f'#define L_{x} 0x{labels[x]:04x}\n' for x in ['fps','state','px','py','level','clock','activefont','backfont','pending']))
run(['cc','-std=c11','-O2','-DNEW_CYCLE_EXACT','-I'+str(S/'src'),'-I'+str(B),'-Ifirmware/engine','-Ifirmware/engine/fatfs','-Igame','game/tests/atari800_harness.c','game/tests/bridge.c','firmware/engine/game_virtual.c','game/game_engine.c',str(S/'src/libatari800.a'),'-lm','-o',str(B/'atari800-game-test')],R,'atari800-link.log')
for tv in ['ntsc','pal']:
 run([str(B/'atari800-game-test'),str(B/'osrom.bin'),'-'+tv,str(V/f'atari800-{tv}.ppm')],R,f'atari800-{tv}.log')
 Image.open(V/f'atari800-{tv}.ppm').resize((768,480),resample=Image.Resampling.NEAREST).save(V/f'atari800-{tv}.png')
 print((V/f'atari800-{tv}.log').read_text().strip())
run(['cc','-std=c11','-O2','-DNEW_CYCLE_EXACT','-I'+str(S/'src'),'-I'+str(B),'-Ifirmware/engine','-Ifirmware/engine/fatfs','-Igame','game/tests/display_stress.c','game/tests/bridge.c','firmware/engine/game_virtual.c','game/game_engine.c',str(S/'src/libatari800.a'),'-lm','-o',str(B/'display-stress')],R,'stress-link.log')
for tv in ['ntsc','pal']:
 run([str(B/'display-stress'),str(B/'osrom.bin'),'-'+tv,'expect-pass'],R,f'display-stress-{tv}.log')
 print((V/f'display-stress-{tv}.log').read_text().strip())
