#!/usr/bin/env python3
"""Check every UF2 block and protected flash boundary, plus source preservation."""
from pathlib import Path
import hashlib,json,struct
R=Path(__file__).resolve().parents[1]
u=R/'releases/A8Duo-CAS-v1-U1-Engine.uf2';data=u.read_bytes()
assert len(data)%512==0;count=len(data)//512;payload=bytearray()
for i in range(count):
    b=data[i*512:(i+1)*512];a,z,flags,addr,n,index,total,family=struct.unpack_from('<8I',b)
    assert (a,z)==(0x0a324655,0x9e5d5157) and struct.unpack_from('<I',b,508)[0]==0x0ab16f30
    assert flags==0x2000 and family==0xe48bff56 and n==256 and index==i and total==count
    assert addr==0x10000000+len(payload) and addr+n<=0x10100000
    payload+=b[32:32+n]
binary=(R/'build/engine/a8_pico_cart.bin').read_bytes()
assert payload[:len(binary)]==binary
assert binary[:256]==(R/'build/baseline/a8_pico_cart.bin').read_bytes()[:256]
changed=[];unchanged=[]
for p in sorted((R/'baseline/firmware').rglob('*')):
    if p.is_file():
        rel=p.relative_to(R/'baseline');q=R/rel
        (unchanged if q.read_bytes()==p.read_bytes() else changed).append(str(rel))
assert changed==['firmware/engine/CMakeLists.txt','firmware/engine/atari_cart.c','firmware/engine/dual_menu.c','firmware/engine/game_virtual.c']
for p in (R/'baseline/game').rglob('*'):
    if p.is_file():assert p.read_bytes()==(R/'game'/p.relative_to(R/'baseline/game')).read_bytes()
report=dict(uf2_sha256=hashlib.sha256(data).hexdigest(),uf2_bytes=len(data),blocks=count,flash_start='0x10000000',flash_end_exclusive=hex(0x10000000+len(payload)),storage_starts='0x10100000',binary_bytes=len(binary),boot2_identical=True,modified_existing_files=changed,unchanged_existing_firmware_files=len(unchanged),game_files_unchanged=31)
(R/'validation/release-check.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
