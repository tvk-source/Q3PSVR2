#!/usr/bin/env python3
"""Build an isolated experimental Quake III menu UI QVM and PK3.

Fix v4:
* Ignore the upstream ui.q3asm Windows-style '-o "\\quake3\\baseq3\\vm\\ui"'
  output directive, replacing it with '-o ui' in a generated config.
* Supply the prewritten code/ui/ui_syscalls.asm; it MUST NOT be compiled
  from code/ui/ui_syscalls.c because that explicitly forbids Q3_VM.
* Never overwrite a successful existing test PK3 or modify working gameplay
  QVMs, native PS5 EBOOT, original assets or installed application.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zipfile

VM_MAGICS = {0x12721444, 0x12721445}
GAME_MODULES = {'bg_misc', 'bg_lib', 'q_math', 'q_shared'}


def parse_q3asm(conf: Path) -> list[str]:
    modules: list[str] = []
    output_count = 0
    for number, raw in enumerate(conf.read_text(encoding='utf-8').splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith(('//', '#', ';')):
            continue
        if line.startswith('-'):
            # Upstream has: -o "\\quake3\\baseq3\\vm\\ui"
            # It is a Windows destination, not a module to compile.
            if re.match(r'^-o\s+\S', line):
                output_count += 1
                continue
            raise ValueError(f'{conf}:{number}: unexpected assembler directive {line!r}')
        name = line.replace('\\', '/').split('/')[-1]
        if name.endswith('.asm'):
            name = name[:-4]
        if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name):
            raise ValueError(f'{conf}:{number}: invalid module name {line!r}')
        if name in modules:
            raise ValueError(f'{conf}:{number}: duplicate UI module {name!r}')
        modules.append(name)
    if output_count != 1:
        raise ValueError(f'{conf}: expected one -o output line, found {output_count}')
    if not modules or modules[0] != 'ui_main' or 'ui_menu' not in modules or 'ui_syscalls' not in modules:
        raise ValueError(f'{conf}: missing critical ui_main/ui_menu/ui_syscalls modules')
    return modules


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True,
                        help='Existing ps5vr_vm_aim_test workspace with patched full Quake III source')
    parser.add_argument('--check', action='store_true',
                        help='Validate source, modules and tools, without building or writing files')
    args = parser.parse_args()
    work = args.work.expanduser().resolve()
    src = work / 'source'
    compiler = work / 'toolbin/q3lcc'
    assembler = src / 'q3asm/q3asm'
    if not assembler.is_file():
        assembler = work / 'toolbin/q3asm'
    config = src / 'code/q3_ui/ui.q3asm'
    menu = src / 'code/q3_ui/ui_menu.c'
    syscalls = src / 'code/ui/ui_syscalls.asm'
    # The original PS5VR cgame sandbox copier excluded *.asm except cg_syscalls.asm.
    # Therefore the UI syscall assembly often lives ONLY in the pristine
    # upstream source checkout listed in the workspace's manifest.json.
    if not syscalls.is_file():
        manifest = work / 'manifest.json'
        if manifest.is_file():
            try:
                upstream = Path(json.loads(manifest.read_text(encoding='utf-8'))['upstream_checkout'])
                candidate = upstream / 'code/ui/ui_syscalls.asm'
                if candidate.is_file():
                    syscalls = candidate
            except (ValueError, KeyError, TypeError):
                pass

    for item in (compiler, assembler, config, menu, syscalls):
        if not item.is_file():
            parser.error(f'Missing {item} (UI VM needs code/ui/ui_syscalls.asm and built q3lcc/q3asm). '
                         'The isolated cgame workspace may have omitted *.asm; '
                         'copy code/ui/ui_syscalls.asm from your original Quake III source checkout into work/source/code/ui/.')
    if 'Q3PSVR2_MENU_BANNER_V3' not in menu.read_text(encoding='utf-8'):
        parser.error(f'Menu patch not found in {menu}; apply patch_menu_banner.py once first')
    try:
        modules = parse_q3asm(config)
    except ValueError as e:
        parser.error(str(e))

    for name in modules:
        if name == 'ui_syscalls':
            continue  # Copy the official QVM syscall ASM, never compile ui_syscalls.c.
        folder = 'game' if name in GAME_MODULES else 'q3_ui'
        source_file = src / 'code' / folder / (name + '.c')
        if not source_file.is_file():
            parser.error(f'Missing C source for {name}: {source_file}')
    out_dir = work / 'q3psvr2-ui-v3'
    pk3 = out_dir / 'zzzzzz-q3psvr2-ui-menu-TEST.pk3'
    if pk3.exists():
        parser.error(f'Refusing to overwrite existing UI test package: {pk3}. Back it up/remove it before rebuilding.')
    if args.check:
        print(f'OK: {len(modules)} modules, including UI VM syscalls ASM: {syscalls}')
        print(f'OK: patched menu source: {menu}')
        print(f'OK: compiler: {compiler}; assembler: {assembler}')
        print(f'Will produce: {pk3}. No files changed.')
        return

    out_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(syscalls, out_dir / 'ui_syscalls.asm')
    env = dict(os.environ)
    env['PATH'] = str(work / 'toolbin') + os.pathsep + env.get('PATH', '')
    flags = (['-DQ3_VM', '-S', '-Wf-target=bytecode', '-Wf-g']
             + ['-I' + str(src / 'code' / folder) for folder in ('cgame', 'game', 'q3_ui')])
    for name in modules:
        if name == 'ui_syscalls':
            continue
        folder = 'game' if name in GAME_MODULES else 'q3_ui'
        source_file = src / 'code' / folder / (name + '.c')
        output_asm = out_dir / (name + '.asm')
        # Remove stale generated assembler modules so a silent compiler failure
        # cannot masquerade as a successful rebuild.
        if output_asm.exists():
            output_asm.unlink()
        print('Compiling', name, flush=True)
        subprocess.run([str(compiler), *flags, str(source_file)], cwd=out_dir,
                       env=env, check=True)
        if not output_asm.is_file():
            parser.error(f'q3lcc did not create {output_asm}')
    generated = out_dir / 'q3psvr2_ui.q3asm'
    generated.write_text('-o ui\n' + '\n'.join(modules) + '\n', encoding='utf-8')
    qvm = out_dir / 'ui.qvm'
    if qvm.exists():
        qvm.unlink()
    print('Assembling menu UI VM', flush=True)
    subprocess.run([str(assembler), '-f', 'q3psvr2_ui'], cwd=out_dir,
                   env=env, check=True)
    if not qvm.is_file() or qvm.stat().st_size < 1000:
        parser.error(f'UI assembler did not produce a plausible ui.qvm in {out_dir}')
    magic = struct.unpack('<I', qvm.read_bytes()[:4])[0]
    if magic not in VM_MAGICS:
        parser.error(f'Unexpected QVM magic: {magic:#x}')
    with zipfile.ZipFile(pk3, mode='x', compression=zipfile.ZIP_DEFLATED) as zf:
        zf.write(qvm, 'vm/ui.qvm')
    print(f'BUILT experimental UI overlay: {pk3} ({pk3.stat().st_size} bytes)')
    print('Copy only the overlay to PPSA99431/baseq3 after saving the existing app files.')
    print('ROLLBACK: remove the test UI PK3, restart. It changes no native or gameplay QVM code.')


if __name__ == '__main__':
    main()
