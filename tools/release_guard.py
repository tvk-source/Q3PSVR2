#!/usr/bin/env python3
"""Audit PPSA99431 for rejected menu UI overlays; optionally quarantine the known test PK3.

The guard does not touch EBOOT, game data, native UI code, QVM gameplay overlays,
or autoexec.cfg. A custom PK3 with vm/ui.qvm requires explicit investigation.
"""
from __future__ import annotations

import argparse
import datetime
from pathlib import Path
import shutil
import sys
import zipfile

KNOWN = 'zzzzzz-q3psvr2-ui-menu-TEST.pk3'


def contains_ui_qvm(p: Path) -> bool:
    try:
        with zipfile.ZipFile(p, 'r') as z:
            return any(name.lower() == 'vm/ui.qvm' for name in z.namelist())
    except (zipfile.BadZipFile, OSError) as exc:
        raise ValueError(f'Unable to inspect PK3 {p}: {exc}') from exc


def audit(app: Path):
    base = app/'baseq3'
    bad = []
    for p in sorted(base.glob('*.pk3')):
        # Never quarantine any retail game data. These may legitimately contain UI QVMs.
        if p.name.lower().startswith('pak') and p.name[3:-4].isdigit():
            continue
        if p.name == KNOWN or contains_ui_qvm(p):
            bad.append(p)
    return bad


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True,
                        help='Path to local installation/build directory named PPSA99431')
    parser.add_argument('--apply', action='store_true',
                        help='Move only the known rejected menu test PK3 outside app (with timestamp)')
    args = parser.parse_args()
    app = args.app.expanduser().resolve()
    if app.name != 'PPSA99431' or not (app/'baseq3').is_dir():
        parser.error('--app must be a PPSA99431 application folder containing baseq3/')
    bad = audit(app)
    if not bad:
        print('PASS: no experimental/custom UI QVM overlay found in baseq3/')
        return 0
    print('BLOCK: custom or rejected UI overlay(s) detected:')
    for p in bad:
        print(' -', p.relative_to(app))
    if not args.apply:
        print(f'Preview only. Run with --apply to quarantine the known {KNOWN}.')
        return 1
    unknown = [p for p in bad if p.name != KNOWN]
    candidate = [p for p in bad if p.name == KNOWN]
    if candidate:
        if len(candidate) != 1:
            raise RuntimeError('More than one matching test overlay, refusing to move')
        qroot = app.parent / ('.q3psvr2-rejected-ui-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
        qroot.mkdir(mode=0o700)
        dst = qroot/candidate[0].name
        shutil.move(str(candidate[0]), str(dst))
        print('QUARANTINED:', dst)
    if unknown:
        print('BLOCK: other custom UI QVM remains; review manually. No other files were changed.')
        return 2
    print('PASS: known rejected UI test quarantined. Other runtime assets unchanged.')
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError) as exc:
        print('ERROR:', exc, file=sys.stderr)
        sys.exit(2)
