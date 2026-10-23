#!/usr/bin/env python3
"""Audit staged paths for obvious confidential, game-data and generated artifacts.
Use after git add -A. Exits nonzero on risk. Cannot replace manual license review.
"""
from pathlib import Path
import subprocess, re, sys
sub=subprocess.run(['git','diff','--cached','--name-only','-z','--diff-filter=ACMR'],capture_output=True)
if sub.returncode:
    sys.exit('ERROR: run within a git repository and stage files first.')
paths=[p.decode('utf-8','replace') for p in sub.stdout.split(b'\x00') if p]
# staged paths include third-party source; only exclude recognized binary payloads/credentials
reasons=[]
for name in paths:
    p=Path(name); lowered=name.lower(); ext=p.suffix.lower()
    why=[]
    if ext in {'.pk3','.pkz','.pak','.sprx','.prx','.self','.pkg','.pfs','.sfo','.ppm','.qvm','.key','.pem','.p12','.pfx','.dmp','.core'}:
        why.append('proprietary/game/firmware/debug/credential binary')
    if re.search(r'(^|/)(pak[0-9]+\.pk3|eboot\.bin|autoexec\.cfg|q3config\.cfg|id_rsa|id_ed25519)(/|$)',lowered):
        why.append('installed game/runtime/secret file')
    if any(t in lowered for t in ['xcsv_bq3hi-res','q3vr-fbo','q3vr-tiled','zzzzzz-q3psvr2-ui-menu-test']):
        why.append('external texture/debug/rejected-menu asset')
    if '/.env' in '/'+lowered or lowered.startswith('.env'):
        why.append('environment/secrets')
    if p.exists() and p.is_file() and p.stat().st_size > 50*1024*1024:
        why.append('oversize >50 MiB; check license and Git LFS')
    if why: reasons.append((name,', '.join(why)))
print('Staged paths:',len(paths))
if reasons:
    for n,r in reasons:print('BLOCK:',n,'=>',r)
    print('Refusing to approve staged set. Unstage files and review.')
    sys.exit(2)
print('PASS: no common binary/game-data/secret patterns. Manually inspect: git diff --cached --name-only')
