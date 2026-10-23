#!/usr/bin/env python3
"""Create an upload-ready ShadowMount homebrew ZIP from a verified build.

Refuses to package retail Quake content; requires the caller's native build.
"""
from __future__ import annotations
import argparse,json,hashlib,zipfile,re,pathlib

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app',required=True,type=pathlib.Path,help='Built PPSAxxxxx directory containing eboot.bin and sce_sys/param.json')
    p.add_argument('--out',required=True,type=pathlib.Path)
    p.add_argument('--reticle',type=pathlib.Path,help='User-built/custom reticle artwork PK3, optional')
    p.add_argument('--qvm',type=pathlib.Path,help='User-built custom cgame QVM PK3, optional')
    a=p.parse_args()
    app=a.app.expanduser().resolve();out=a.out.expanduser().resolve()
    if not app.is_dir() or not (app/'eboot.bin').is_file():p.error('Built EBOOT folder required')
    manifest=app/'sce_sys/param.json'
    if not manifest.is_file():p.error('Missing sce_sys/param.json')
    data=json.loads(manifest.read_text())
    tid=data.get('titleId','')
    if not re.fullmatch(r'PPSA\d{5}',tid) or tid!=app.name:p.error('Title ID mismatch between manifest and folder')
    if tid=='PPSA99303':p.error('Old development title ID. Migrate identity first.')
    if not data.get('contentId','').startswith(f'UP9000-{tid}_00-'):p.error('Content ID mismatch')
    if out.exists():p.error('Output already exists. Refusing to overwrite.')
    files=[]
    for f in app.rglob('*'):
        if not f.is_file():continue
        if f.is_symlink():p.error('Symlink in release source: '+str(f))
        if f.suffix.lower() in {'.sprx','.self','.pkg'} and 'sce_module' not in f.parts:
            print('Warning: inspect redistributability of '+str(f))
        if f.suffix.lower()=='.pk3' and f.name not in ('zzzz-ps5vr-reticle.pk3','zzzzz-ps5vr-cgame-aim-test.pk3'):
            p.error('Refusing to package possible commercial game archive '+str(f))
        files.append((f,f.relative_to(app).as_posix()))
    if a.reticle:
        f=a.reticle.expanduser().resolve()
        if not f.is_file() or f.suffix!='.pk3':p.error('Invalid reticle path')
        files.append((f,'baseq3/zzzz-ps5vr-reticle.pk3'))
    if a.qvm:
        f=a.qvm.expanduser().resolve()
        if not f.is_file() or f.suffix!='.pk3':p.error('Invalid QVM path')
        with zipfile.ZipFile(f) as z:
            if 'vm/cgame.qvm' not in z.namelist():p.error('QVM PK3 lacks vm/cgame.qvm')
        files.append((f,'baseq3/zzzzz-ps5vr-cgame-aim-test.pk3'))
    dests=[x for _,x in files]
    if len(dests)!=len(set(dests)):p.error('Duplicate destination files; avoid passing assets already present')
    out.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=5) as z:
        for f,rel in files:z.write(f,f'{tid}/{rel}')
    digest=hashlib.sha256(out.read_bytes()).hexdigest()
    checksum=out.with_suffix(out.suffix+'.sha256')
    checksum.write_text(f'{digest}  {out.name}\n')
    print('Created',out,'\nSHA256',digest)
    print('No commercial baseq3 content included. Users supply their own legally obtained assets.')

if __name__=='__main__':main()
