#!/usr/bin/env python3
"""Produce portable and source release archives, plus SHA-256 checksums."""
import hashlib
import zipfile
from pathlib import Path
root = Path(__file__).resolve().parent.parent
version = (root / 'VERSION').read_text().strip()
dist = root / 'dist'
dist.mkdir(exist_ok=True)
portable = dist / f'Thrusty-{version}-Windows-x64.zip'
source = dist / f'Thrusty-{version}-Source.zip'
installer = dist / f'Thrusty-{version}-Setup.exe'
if not installer.is_file():
    raise SystemExit('Compile the installer before packaging.')
files = {'Thrusty.exe': root/'bin/Thrusty.exe', 'ViGEmClient.dll': root/'ViGEmClient.dll'}
for name in ['README.html', 'LICENSE.txt', 'CREDITS.md', 'VALIDATION.txt']:
    files[name] = root/name
for directory in ['drivers', 'licenses']:
    for path in (root/directory).rglob('*'):
        if path.is_file(): files[path.relative_to(root).as_posix()] = path
with zipfile.ZipFile(portable, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for name,path in sorted(files.items()): z.write(path, 'Thrusty/'+name)
# Explicit exclusions keep compiled output, personal settings and git internals out.
excluded = {'.git','bin','build','dist','__pycache__'}
with zipfile.ZipFile(source, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for path in sorted(root.rglob('*')):
        rel = path.relative_to(root)
        if not path.is_file() or any(part in excluded for part in rel.parts): continue
        if path.name in ['version.h','version.nsh'] or path.suffix in ['.pyc','.log']: continue
        z.write(path, 'Thrusty-'+version+'/'+rel.as_posix())
for path in [portable,source]:
    with zipfile.ZipFile(path) as z:
        if z.testzip(): raise SystemExit('Archive validation failed: '+str(path))
artifacts = [installer,portable,source]
(dist/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in artifacts))
print('Release packages verified: '+', '.join(p.name for p in artifacts))
