#!/usr/bin/env python3
"""Read-only audit of release-sensitive automated PPM screenshot and UI text paths."""
from pathlib import Path
import argparse
import re

PATTERNS={
 'PPM capture': re.compile(r'\.ppm\b|q3vr-(?:fbo|tiled)|(?:save|write|dump|capture)[A-Za-z_]*(?:PPM|Ppm)|(?:PPM|Ppm)[A-Za-z_]*(?:save|write|dump|capture)',re.I),
 'Native console/chat drawing': re.compile(r'Con_DrawNotify|Con_DrawConsole|Con_Print|CG_DrawTeamInfo|CG_AddToTeamChat|CG_DrawStringExt|chatMsg|CHATLOC_|cg_teamChat',re.I),
 'VR UI projection': re.compile(r'ps5vr_uiScale|ps5vr_uiYOffset|RB_SetGL2D|Main_MenuDraw|vr_virtual_screen|ps5vr_uiPanel',re.I),
}
EXCLUDE={'build','dist','.git','__pycache__','node_modules','ps5vr_vm_aim_test'}
SUFFIXES={'.c','.cc','.cpp','.h','.hpp','.sh'}
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--root',type=Path,required=True)
 ap.add_argument('--only',choices=list(PATTERNS)+['all'],default='all')
 ap.add_argument('--max-hits',type=int,default=80)
 a=ap.parse_args()
 root=a.root.resolve()
 if not root.is_dir():ap.error('Not a directory '+str(root))
 total={k:0 for k in PATTERNS}
 for p in sorted(root.rglob('*')):
  if not p.is_file() or p.suffix not in SUFFIXES or any(seg in EXCLUDE for seg in p.relative_to(root).parts):continue
  try: lines=p.read_text(errors='replace').splitlines()
  except (OSError,UnicodeError):continue
  for i,line in enumerate(lines):
   for title,pat in PATTERNS.items():
    if a.only not in ('all',title) or not pat.search(line):continue
    total[title]+=1
    if total[title]<=a.max_hits:
     print(f'[{title}] {p.relative_to(root)}:{i+1}: {line.strip()[:200]}')
 print('\nMatches:',total)
 print('Read-only scan. Presence of PPM strings does NOT prove automatic screenshots.')
 print('Inspect trigger/call sites. Release policy: automatic captures OFF; explicit diagnostic opt-in only.')
if __name__=='__main__':main()
