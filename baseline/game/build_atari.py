#!/usr/bin/env python3
"""Build a bootable 128-byte-sector image and the U1 embedded image header."""
from pathlib import Path
import os, subprocess, hashlib, json
P=Path(__file__).resolve().parent
G=P/'generated'
subprocess.run(['python3',str(P/'generate_assets.py')],check=True)
subprocess.run([os.environ.get('CA65','ca65'),'-g','-o',str(G/'game.o'),str(P/'atari/game.s')],check=True)
subprocess.run([os.environ.get('LD65','ld65'),'-C',str(P/'atari/game.cfg'),'-o',str(G/'game.bin'),'-Ln',str(G/'game.lbl'),'-m',str(G/'game.map'),str(G/'game.o')],check=True)
b=bytearray((G/'game.bin').read_bytes());b+=bytes((-len(b))%128)
assert 6<len(b)<=8192
b[1]=len(b)//128
(G/'game.bin').write_bytes(b)
(G/'atari_image.h').write_text('/* Generated boot sectors; do not edit. */\nstatic const unsigned char hg_atari_image[]={\n'+''.join(' '+','.join(f'0x{x:02x}' for x in b[i:i+16])+',\n' for i in range(0,len(b),16))+'};\n')
header=bytes([0x96,2,0,0x20,0x80]+[0]*11)
(G/'HARDGAME.atr').write_bytes(header+b+bytes(1024*128-len(b)))
info={'boot_bytes':len(b),'boot_sectors':b[1],'load_address':'0x2000','entry':'0x2006','sha256':hashlib.sha256(b).hexdigest()}
(G/'atari-build.json').write_text(json.dumps(info,indent=2)+'\n')
print(info)
