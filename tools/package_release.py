#!/usr/bin/env python3
"""Build a checked ZIP without toolchains, caches, or downloaded third-party CAS."""
from pathlib import Path
import hashlib,json,shutil,zipfile
R=Path(__file__).resolve().parents[1];D=R.parent/'deliverables';D.mkdir(exist_ok=True)
paths=[]
for name in ['firmware','game','baseline','cassette','generated','tests','validation','evidence','docs','releases']:
    for p in sorted((R/name).rglob('*')):
        if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc':
            paths.append(p)
paths += [R/'README.md',R/'CAS-vs-v021.patch']
paths += sorted((R/'tools').glob('*.py'))
paths += [R/'tools/build_release.sh']
manifest=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(R))+'\n' for p in paths)
(R/'PACKAGE-SHA256SUMS.txt').write_text(manifest)
paths.append(R/'PACKAGE-SHA256SUMS.txt')
archive=D/'A8Duo-CAS-v1-Complete.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for p in paths:z.write(p,'A8Duo-CAS-v1/'+str(p.relative_to(R)))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert len(set(z.namelist()))==len(paths)
    for p in paths:assert z.read('A8Duo-CAS-v1/'+str(p.relative_to(R)))==p.read_bytes()
firmware=D/'A8Duo-CAS-v1-U1-Engine.uf2';shutil.copyfile(R/'releases'/firmware.name,firmware)
result={'archive':str(archive),'archive_bytes':archive.stat().st_size,'files':len(paths),'zip_crc_check':'PASS','archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'firmware_sha256':hashlib.sha256(firmware.read_bytes()).hexdigest()}
(D/'A8Duo-CAS-v1-package-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
