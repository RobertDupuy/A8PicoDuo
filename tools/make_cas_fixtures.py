#!/usr/bin/env python3
"""Build original synthetic cassette diagnostics and parser rejection cases."""
from pathlib import Path
import os, struct, subprocess
R=Path(__file__).resolve().parents[1]
G=R/'generated'; D=R/'tests/cas-fixtures'
G.mkdir(exist_ok=True); D.mkdir(exist_ok=True)
subprocess.run([os.environ.get('CA65','ca65'),str(R/'tests/cas_fixture.s'),'-o',str(G/'fixture.o')],check=True)
subprocess.run([os.environ.get('LD65','ld65'),'-C',str(R/'tests/cas_fixture.cfg'),str(G/'fixture.o'),'-o',str(G/'fixture.bin'),'-Ln',str(G/'fixture.lbl')],check=True)
boot=(G/'fixture.bin').read_bytes()
assert len(boot)==128*boot[1]
def chunk(tag,data=b'',aux=0):return tag+struct.pack('<HH',len(data),aux)+data
def checksum(data):
    s=0
    for b in data:s+=b;s=(s&255)+(s>>8)
    return s
def record(data=b'',tag=0xfc,short=False):
    if tag==0xfa:
        assert len(data)<=127
        payload=data+(b'' if short else bytes(127-len(data)))+bytes([len(data)])
    else:payload=data.ljust(128,b'\0');assert len(payload)==128
    raw=b'UU'+bytes([tag])+payload
    return chunk(b'data',raw+bytes([checksum(raw)]),250)
header=chunk(b'FUJI',b'A8Duo original CAS regression fixture')+chunk(b'baud',aux=600)
eof=record(tag=0xfe)
def body(b):return b''.join(record(b[i:i+128]) for i in range(0,len(b),128))
good=header+body(boot)+eof+chunk(b'FUJI',b'Next file')+record(b'HELLO',0xfa,True)+eof+record(b'RAW SIO RECORD')+record(b'Z')+eof
files={'BOOTTEST.CAS':good,'bad-checksum.cas':good[:-1]+bytes([good[-1]^1]),'turbo.cas':header+chunk(b'pwmc'), 'fsk.cas':header+chunk(b'fsk '), 'truncated.cas':good[:-2], 'missing-header.cas':record(boot[:128]),'bad-baud.cas':chunk(b'FUJI')+chunk(b'baud'), 'unknown.cas':header+chunk(b'xxxx'), 'empty.cas':b'', 'too-many.cas':good+record(b'X')*4096, 'empty-partial.cas':header+record(b'',0xfa,True)}
for addr in [0x047f,0xbf80,0xff80]:
    b=bytearray(boot);b[2:4]=struct.pack('<H',addr)
    files[f'bad-address-{addr:04x}.cas']=header+body(b)+eof
b=bytearray(boot);b[1]=0;b[2:4]=struct.pack('<H',0x4000)
files['count-zero.cas']=header+body(b)+record()*(256-len(b)//128)+eof
for name,data in files.items():(D/name).write_bytes(data)
# Exact inherited ATR functions, compiled into host tests without the GPIO code.
s=(R/'firmware/engine/atari_cart.c').read_text()
s=s[s.index('// ATR format'):s.index('int load_file(')]
(G/'atr_actual.c').write_text('#include <stdint.h>\n#include <string.h>\n#include "ff.h"\n'+s)
print(f'Generated {len(files)} CAS fixtures, including visible PASS/FAIL diagnostic.')
