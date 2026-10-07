#!/usr/bin/env python3
"""Prepare an isolated Atari800 7.2.1 checkout for the command-bridge tests.
This is a host test hook, not RP2040 firmware or a cycle-accurate RP2040 model.
"""
from pathlib import Path
import os,re,subprocess
S=Path(os.environ['ATARI800_SRC']).resolve()
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
for args in [['./configure','--target=libatari800','--disable-maintainer-mode','--disable-netsio','--disable-rdevice','--disable-ide','--disable-pokeyrec'],['make','-C','src','clean'],['make','-C','src','-j4','libatari800.a','CFLAGS=-O2 -g -DNEW_CYCLE_EXACT']]:
    subprocess.run(args,cwd=S,check=True)
