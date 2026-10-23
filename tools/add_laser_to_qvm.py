#!/usr/bin/env python3
"""Reversible gameplay-aim laser in the existing PS5VR cgame VM sandbox.

Uses the already-computed reticle trace and Quake's standard RT_RAIL_CORE beam.
It does NOT enable tracked PSVR2 Sense aiming or alter shots/bullets.
Requires the existing PS5VR_VM_AIM_V1 and QVM CvarValue compatibility fix.
"""
from __future__ import annotations
import argparse, json, hashlib, shutil, datetime
from pathlib import Path

MARK = '/* Q3PSVR2_LASER_V1:'
BEGIN = '/* PS5VR_VM_AIM_V1_BEGIN */'
END = '/* PS5VR_VM_AIM_V1_END */'
WORK_MARK = 'PS5VR_VM_AIM_V1_20261021'
PK3 = 'zzzzz-ps5vr-cgame-aim-test.pk3'
INSERT = r'''    /* Q3PSVR2_LASER_V1: uses the same gameplay hit as our QVM reticle.
     * RT_RAIL_CORE exists in stock Quake III: draws a 3D beam between
     * origin and oldorigin; the rail shader may affect thickness/colour.
     * One beam per scene build. NO extra collision trace. */
    if (PS5VR_VM_CvarValue("ps5vr_laserSight") > 0.5f) {
        refEntity_t beam;
        if (cgs.media.railCoreShader) {
            memset(&beam, 0, sizeof(beam));
            beam.reType = RT_RAIL_CORE;
            beam.customShader = cgs.media.railCoreShader;
            VectorCopy(muzzle, beam.origin);
            if (trace.fraction < 1.0f) {
                VectorMA(trace.endpos, -1.5f, forward, beam.oldorigin);
            } else {
                VectorCopy(trace.endpos, beam.oldorigin);
            }
            beam.shaderRGBA[0] = 255;
            beam.shaderRGBA[1] = 40;
            beam.shaderRGBA[2] = 30;
            beam.shaderRGBA[3] = 120;
            trap_R_AddRefEntityToScene(&beam);
        }
    }
'''

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def prep(work):
    w=work.resolve()
    m=w/'manifest.json'
    if not m.is_file(): raise RuntimeError('QVM sandbox manifest missing')
    d=json.loads(m.read_text())
    if d.get('marker')!=WORK_MARK or Path(d.get('work','')).resolve()!=w:
        raise RuntimeError('Unexpected QVM sandbox. Refusing to patch.')
    s=w/'source/code/cgame/cg_draw.c'
    if not s.is_file():raise RuntimeError('Missing cg_draw.c')
    return w,s,w/PK3

def modify(src):
    if MARK in src: raise RuntimeError('Already patched')
    if src.count(BEGIN)!=1 or src.count(END)!=1: raise RuntimeError('Missing QVM reticle markers')
    start=src.index(BEGIN); end=src.index(END,start)
    part=src[start:end]
    if 'PS5VR_VM_CvarValue' not in part: raise RuntimeError('Apply QVM cvar compatibility fix first')
    pre='    if (PS5VR_VM_CvarValue("ps5vr_vmReticle") < 0.5f) return;'
    point='    VectorSubtract(target, cg.refdef.vieworg, delta);'
    if part.count(pre)!=1 or part.count(point)!=1:raise RuntimeError('Unexpected reticle code; refusing blind edit')
    part=part.replace(pre,'    if (PS5VR_VM_CvarValue("ps5vr_vmReticle") < 0.5f &&\n        PS5VR_VM_CvarValue("ps5vr_laserSight") < 0.5f) return;',1)
    part=part.replace(point,INSERT+'\n'+point,1)
    # Draw laser even with reticle disabled.
    part=part.replace(point,'    if (PS5VR_VM_CvarValue("ps5vr_vmReticle") < 0.5f) return;\n'+point,1)
    return src[:start]+part+src[end:]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--work',type=Path,required=True)
    p.add_argument('--check',action='store_true')
    p.add_argument('--restore',type=Path)
    a=p.parse_args(); work,draw,pk3=prep(a.work.expanduser())
    if a.restore:
        back=a.restore.expanduser().resolve(); meta=json.loads((back/'backup.json').read_text())
        if meta['marker']!='Q3PSVR2_LASER_V1' or meta['work']!=str(work):raise RuntimeError('Wrong backup')
        for name in ('cg_draw.c',PK3):
            b=back/name
            if not b.is_file() or sha(b)!=meta['sha256'][name]:raise RuntimeError('Damaged backup '+name)
        if MARK not in draw.read_text(): raise RuntimeError('Laser patch not installed')
        if pk3.exists():
            trial=back/'laser-experimental.pk3'
            if trial.exists(): raise RuntimeError('Trial backup already exists')
            shutil.move(pk3,trial)
        shutil.copy2(back/'cg_draw.c',draw);shutil.copy2(back/PK3,pk3)
        print('Restored original cg_draw.c and PK3 in local workspace.')
        return
    new=modify(draw.read_text())
    if not pk3.is_file() or pk3.stat().st_size<1024:raise RuntimeError('Preserve existing compiled PK3 before patching')
    if a.check:
        print('CHECK OK: supports reticle-only, laser-only, and both, sharing the collision trace')
        return
    backups=work/'q3psvr2_laser_backups'
    stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    folder=backups/('baseline-'+stamp);folder.mkdir(parents=True,exist_ok=False)
    shutil.copy2(draw,folder/'cg_draw.c');shutil.copy2(pk3,folder/PK3)
    (folder/'backup.json').write_text(json.dumps({'marker':'Q3PSVR2_LASER_V1','work':str(work),'sha256':{x:sha(folder/x) for x in ('cg_draw.c',PK3)}},indent=2)+'\n')
    try:
        draw.write_text(new)
        pk3.unlink()  # existing resume script intentionally rejects overwrites
    except BaseException:
        shutil.copy2(folder/'cg_draw.c',draw);shutil.copy2(folder/PK3,pk3)
        raise
    print('Patched laser into shared QVM trace. Backup:',folder)
    print('Run existing ps5vr_qvm_resume_compat.py --work',work)

if __name__=='__main__':main()
