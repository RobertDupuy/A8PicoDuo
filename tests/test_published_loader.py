#!/usr/bin/env python3
"""Optional network test: independent 1983 loader from AtariWiki, not distributed.
The wiki attachment is a raw boot body despite its .cas name. We wrap its first
six declared records into a standard FUJI container and append an original XEX.
"""
from pathlib import Path
import hashlib,struct,subprocess,shutil,urllib.request
R=Path(__file__).resolve().parents[1];D=R/'build/external-fixtures'
u='https://atariwiki.org/wiki/attach/Boot%20from%20Cassette/Binary_Loader_BL-C_0.2.cas'
cache=R/'research/Binary_Loader_BL-C_0.2.cas';cache.parent.mkdir(exist_ok=True)
if not cache.exists():cache.write_bytes(urllib.request.urlopen(u,timeout=30).read())
raw=cache.read_bytes();assert raw[:6]==bytes.fromhex('00 06 00 07 0b 07') and len(raw)==769
assert hashlib.sha256(raw).hexdigest()=='3ccf315981e667ed3398dffd9790654e4ec96ebaaa291173ca43d00ac621673f'
shutil.copytree(R/'tests/cas-fixtures',D,dirs_exist_ok=True)
def chunk(tag,data=b'',aux=0):return tag+struct.pack('<HH',len(data),aux)+data
def record(data=b'',tag=0xfc):
    if tag==0xfa:payload=data.ljust(127,b'\0')+bytes([len(data)])
    else:payload=data.ljust(128,b'\0')
    b=b'UU'+bytes([tag])+payload;s=0
    for v in b:s+=v;s=(s&255)+(s>>8)
    return chunk(b'data',b+bytes([s]),250)
boot=raw[:768];xex=bytes.fromhex('ff ff 00 20 07 20 a9 5c 8d 0d 06 4c 05 20 e0 02 e1 02 00 20')
cas=chunk(b'FUJI',b'BL/C 0.2 independent integration test')+chunk(b'baud',aux=600)
cas+=b''.join(record(boot[i:i+128]) for i in range(0,768,128))+record(tag=0xfe)+record(xex,0xfa)+record(tag=0xfe)
(D/'BOOTTEST.CAS').write_bytes(cas)
for tv in ['ntsc','pal']:
    proc=subprocess.run([str(R/'build/tests/cas-atari800'),str(D),str(R/'generated/cas-rom.bin'),'-'+tv,'1','external'],capture_output=True)
    log=f'Source: {u}\nRaw SHA256: {hashlib.sha256(raw).hexdigest()}\nContainer conversion: six 128-byte raw boot records, standard checksum/EOF, appended synthetic XEX.\n'.encode()+proc.stdout+proc.stderr
    (R/f'validation/published-loader-{tv}.log').write_bytes(log)
    print(log.decode());proc.check_returncode()
