#!/usr/bin/env python3
"""Export all user-tested QVM modifications as a reproducible GPL patch.

Diffs against original Quake III Arena GPL source. Does not modify either source
and never exports commercial Quake PK3 content.
"""
from __future__ import annotations
import argparse,json,difflib,hashlib,pathlib

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--work',type=pathlib.Path,required=True,help='Existing ps5vr_vm_aim_test sandbox')
    p.add_argument('--upstream',type=pathlib.Path,required=True,help='Untouched id-Software/Quake-III-Arena checkout')
    p.add_argument('--out',type=pathlib.Path,required=True,help='New, empty directory under q3psvr2/patches/')
    a=p.parse_args();work=a.work.expanduser().resolve();up=a.upstream.expanduser().resolve();out=a.out.expanduser().resolve()
    if not (work/'manifest.json').is_file():p.error('Not an existing QVM workspace')
    d=json.loads((work/'manifest.json').read_text())
    if d.get('marker')!='PS5VR_VM_AIM_V1_20261021' or pathlib.Path(d.get('work','')).resolve()!=work:
        p.error('QVM manifest mismatch')
    if out.exists():p.error('Destination already exists, refusing overwrite')
    patch=[];manifest=[]
    for rel in ('code/cgame/cg_draw.c','code/cgame/cg_weapons.c'):
        left=up/rel;right=work/'source'/rel
        if not left.is_file() or not right.is_file():p.error('Missing upstream or modified file: '+rel)
        old=left.read_text();new=right.read_text()
        if old==new:p.error('No QVM modifications detected for '+rel)
        patch.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),
                                         fromfile='a/'+rel,tofile='b/'+rel,n=3))
        manifest.append({'path':rel,'upstream_sha256':digest(left),'modified_sha256':digest(right)})
    if not patch:p.error('No differences')
    out.mkdir(parents=True)
    (out/'q3psvr2-cgame.patch').write_text(''.join(patch))
    (out/'SOURCE_SHA256.json').write_text(json.dumps({'files':manifest,'license':'Quake III Arena GPL engine source; preserve upstream COPYING','note':'Apply against matching unmodified upstream files'},indent=2)+'\n')
    print('Exported source changes:',out/'q3psvr2-cgame.patch')
    print('Track these two patch files in Git; run git apply --check first against verified upstream checkout.')

if __name__=='__main__':main()
