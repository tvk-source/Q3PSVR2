#!/usr/bin/env python3
"""Opt-in Quake 3 UI main banner framing A/B in the isolated original QVM source."""
import argparse
import hashlib
import re
import shutil
import sys
from pathlib import Path

MARK = 'Q3PSVR2_MENU_BANNER_V3'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()[:12]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=Path, required=True, help='Existing ps5vr_vm_aim_test workspace')
    op = ap.add_mutually_exclusive_group()
    op.add_argument('--check', action='store_true', help='Verify patch eligibility, no modifications')
    op.add_argument('--restore', action='store_true', help='Restore the one automatic backup')
    a = ap.parse_args()
    p = a.work.resolve()/'source/code/q3_ui/ui_menu.c'
    backup = p.with_suffix('.c.before-q3psvr2-menu-v3')
    if not p.is_file():
        ap.error('Missing '+str(p))
    if a.restore:
        if not backup.is_file(): ap.error('No source backup exists: '+str(backup))
        if MARK not in p.read_text(): ap.error('Current source lacks patch marker, refusing to overwrite it')
        shutil.copy2(backup,p)
        print('Restored',p,'sha256/12',sha(p))
        return
    s=p.read_text()
    if MARK in s: ap.error('Already patched; use --restore to undo')
    # Use strict anchors from id Software q3_ui. Fail closed on different code.
    pat_view = r'(\bx\s*=\s*0\s*;\s*y\s*=\s*0\s*;\s*w\s*=\s*640\s*;\s*h\s*=\s*120\s*;)'
    pat_z = r'(\borigin\[2\]\s*=\s*-32\s*;)'
    if len(re.findall(pat_view,s)) != 1 or len(re.findall(pat_z,s)) != 1:
        ap.error('Original menu layout changed; refusing unverified patch. Inspect code/q3_ui/ui_menu.c')
    add_view='''\n\t/* Q3PSVR2_MENU_BANNER_V3: VR logo viewport diagnostic; defaults to stock. */
\t{
\t\tchar q3psvr2_vstr[32];
\t\tfloat q3psvr2_viewY;
\t\ttrap_Cvar_VariableStringBuffer("ps5vr_menuBannerViewportY", q3psvr2_vstr, sizeof(q3psvr2_vstr));
\t\tq3psvr2_viewY = atof(q3psvr2_vstr);
\t\tif (q3psvr2_viewY < -60.0f) q3psvr2_viewY = -60.0f;
\t\tif (q3psvr2_viewY > 120.0f) q3psvr2_viewY = 120.0f;
\t\ty += q3psvr2_viewY;
\t}
'''
    add_z='''\n\t/* Q3PSVR2_MENU_BANNER_V3: model position relative to original -32. */
\t{
\t\tchar q3psvr2_mstr[32];
\t\tfloat q3psvr2_drop;
\t\ttrap_Cvar_VariableStringBuffer("ps5vr_menuBannerModelDrop", q3psvr2_mstr, sizeof(q3psvr2_mstr));
\t\tq3psvr2_drop = atof(q3psvr2_mstr);
\t\tif (q3psvr2_drop < -128.0f) q3psvr2_drop = -128.0f;
\t\tif (q3psvr2_drop > 160.0f) q3psvr2_drop = 160.0f;
\t\torigin[2] -= q3psvr2_drop;
\t}
'''
    s=re.sub(pat_view,lambda m:m.group(1)+add_view,s,count=1)
    s=re.sub(pat_z,lambda m:m.group(1)+add_z,s,count=1)
    if a.check:
        print('Eligible:',p,'sha256/12',sha(p))
        print('No modifications made. New cvars default to 0 (stock menu).')
        return
    if backup.exists(): ap.error('Existing backup requires manual inspection: '+str(backup))
    shutil.copy2(p,backup)
    p.write_text(s)
    print('Patched:',p,'backup:',backup)
    print('NOTE: This changes UI source; compile/test a separate UI QVM. No EBOOT modification.')

if __name__ == '__main__': main()
