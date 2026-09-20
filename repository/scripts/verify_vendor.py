#!/usr/bin/env python3
"""Fail before packaging if an embedded third-party binary has changed."""
import hashlib
from pathlib import Path
root = Path(__file__).resolve().parent.parent
expected = {
 'ViGEmClient.dll': '2bf0cb1d809039573c922737d298a1653d4dbc61408060ff45a9bcfde82e97d2',
 'drivers/ViGEmBus_1.22.0.exe': '89220a7865076b342892f98865f3499fb7c4cfd673159e89d352c360fd014c6a',
}
for name, digest in expected.items():
    if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
        raise SystemExit(f'Bundled dependency checksum mismatch: {name}')
print('Bundled dependency checksums verified.')
