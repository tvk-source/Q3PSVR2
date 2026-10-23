#!/usr/bin/env python3
"""Audit and safely migrate Q3PSVR2 homebrew source identity (stdlib only).

Default is DRY RUN. Modifies only PS5 build metadata and specifically PS5 tools
build scripts containing the old title ID. Does not edit EBOOT or game data.
"""
from __future__ import annotations
import argparse, json, re, pathlib, hashlib, datetime, shutil
TITLE_PATTERN=re.compile(r'^PPSA\d{5}$')
OLD='PPSA99303'
NEW='PPSA99431'  # Candidate; verify against other installed apps.
APP_NAME='Quake III PSVR2'

def is_src(p,root):
    q=p.relative_to(root)
    parts=set(q.parts)
    if parts&{'.git','build','dist','output','.deps','node_modules','backups','ps5vr_vm_aim_test'}:return False
    if p.name=='param.json' and ('sce_sys' in parts or 'ps5' in parts):return True
    if p.name in {'build-vr.sh','build-ui.sh','Makefile','CMakeLists.txt','package.sh'} and ('ps5' in parts):return True
    return False

def update_param(src,new,name):
    data=json.loads(src)
    if data.get('titleId') not in (OLD,new):
        raise RuntimeError(f'Found different titleId in param.json: {data.get("titleId")}')
    data['titleId']=new
    cid=data.get('contentId','')
    if cid and OLD not in cid and new not in cid:raise RuntimeError('Unknown contentId; do not replace blindly')
    data['contentId']=(cid.replace(OLD,new) if cid else f'UP9000-{new}_00-Q3PSVR2HOMEBREW1')
    if not re.fullmatch(r'[A-Z]{2}\d{4}-PPSA\d{5}_00-[A-Z0-9]{16}',data['contentId']):
        raise RuntimeError('Invalid PS5 homebrew content ID: '+data['contentId'])
    loc=data.setdefault('localizedParameters',{})
    for k,v in loc.items():
        if isinstance(v,dict) and ('titleName' in v or k in ('en-US','en-GB')):
            v['titleName']=name
    if 'en-US' not in loc:loc['en-US']={'titleName':name}
    return json.dumps(data,indent=2,ensure_ascii=False)+'\n'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root',type=pathlib.Path,required=True)
    ap.add_argument('--title-id',default=NEW)
    ap.add_argument('--name',default=APP_NAME)
    ap.add_argument('--apply',action='store_true',help='Apply after showing dry-run plan')
    args=ap.parse_args()
    root=args.root.expanduser().resolve()
    if not root.is_dir():ap.error('Repository directory not found')
    if not TITLE_PATTERN.fullmatch(args.title_id):ap.error('Expected PPSA + five digits')
    files=[]
    for p in root.rglob('*'):
        if not p.is_file() or not is_src(p,root):continue
        if p.name!='param.json' and p.stat().st_size>2_000_000:continue
        old=p.read_text(encoding='utf-8')
        if p.name=='param.json':new=update_param(old,args.title_id,args.name)
        else:new=old.replace(OLD,args.title_id)
        if old!=new:files.append((p,old,new))
    print(f'Source: {root}\nIdentity: {args.title_id} / {args.name}')
    print('Files to change:',len(files))
    for p,_,__ in files:print('  ',p.relative_to(root))
    if not files:
        print('No applicable metadata/build-script identity found. Check build-vr.sh and external PS5_HUI_DIR manually.')
        return
    if not args.apply:
        print('DRY RUN. Add --apply to write files with SHA256 backups.')
        return
    back=root/'.q3psvr2-release-backups'/datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    back.mkdir(parents=True)
    hashes={}
    for p,old,new in files:
        rel=p.relative_to(root)
        dest=back/rel;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(p,dest)
        hashes[str(rel)]=hashlib.sha256(dest.read_bytes()).hexdigest()
    (back/'manifest.json').write_text(json.dumps({'root':str(root),'identity':args.title_id,'previous_hashes':hashes},indent=2)+'\n')
    for p,old,new in files:p.write_text(new,encoding='utf-8')
    print('Wrote changes. Exact source backups:',back)
    print('Important: verify externally supplied PS5_HUI_DIR packaging manifest; rebuild into NEW TITLE_ID folder.')

if __name__=='__main__':main()
