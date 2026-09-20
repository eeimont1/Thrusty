#!/usr/bin/env python3
"""Generate build metadata from the single VERSION file."""
from pathlib import Path
import re
root = Path(__file__).resolve().parent.parent
version = (root / 'VERSION').read_text().strip()
if not re.fullmatch(r'\d+\.\d+\.\d+', version):
    raise SystemExit('VERSION must be MAJOR.MINOR.PATCH')
(root / 'src/version.h').write_text(
    '#pragma once\n#define THRUSTY_VERSION "' + version + '"\n'
    '#define THRUSTY_VERSION_W L"' + version + '"\n'
    '#define THRUSTY_VERSION_TUPLE ' + version.replace('.', ',') + ',0\n')
(root / 'version.nsh').write_text('!define THRUSTY_VERSION "' + version + '"\n')
print(version)
