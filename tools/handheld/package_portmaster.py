#!/usr/bin/env python3
"""Builds the PortMaster zip from build-handheld (TK64_MOBILE build). Refuses a build without TK64_MOBILE and any game data.

    python tools/handheld/package_portmaster.py [--build-dir build-handheld] [--release-version 0.1.4]

--release-version, or TOOIE_RELEASE_VERSION, names a release; without it the name carries git describe.

llvm-strip comes from HANDHELD_LLVM, then PATH, then Visual Studio's LLVM on Windows.

Writes dist/BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip (dev-<describe> without a version) and its .sha256.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
def find_strip():
    exe = 'llvm-strip.exe' if os.name == 'nt' else 'llvm-strip'
    llvm = os.environ.get('HANDHELD_LLVM') or os.environ.get('XU10_LLVM')
    if llvm:
        return str(Path(llvm) / exe)
    found = shutil.which(exe)
    if found:
        return found
    if os.name == 'nt':
        vswhere = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        if vswhere.exists():
            vs = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Llvm.Clang', '-property', 'installationPath'], capture_output=True, text=True).stdout.strip()
            candidate = Path(vs) / 'VC/Tools/Llvm/x64/bin' / exe
            if vs and candidate.exists():
                return str(candidate)
    sys.exit('llvm-strip not found: set HANDHELD_LLVM or put it on PATH')


LICENSE_DIRS = ['vendor/licenses', 'lib/rt64', 'lib/N64ModernRuntime', 'lib/RecompFrontend', 'lib/SlotMap', 'lib/dr_libs']
# Game data never goes in: ROM images by name, and whatever a run leaves in a port folder.
FORBIDDEN_NAME = re.compile(r'\.(z64|n64|v64|rom|eep|sra|fla)$|^(saves?|shadercache|log\.txt|stats\.txt)$', re.IGNORECASE)
MAX_FILE_BYTES = 64 * 1024 * 1024


def git(*args):
    return subprocess.run(['git', '-C', str(ROOT), *args], capture_output=True, check=True, text=True).stdout.strip()


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def elf_needed(data):
    """DT_NEEDED of a little-endian ELF64."""
    if data[:6] != b'\x7fELF\x02\x01':
        sys.exit('not a little-endian ELF64 binary')
    phoff, phentsize, phnum = struct.unpack_from('<Q', data, 32)[0], *struct.unpack_from('<HH', data, 54)
    loads, dynamic = [], None
    for i in range(phnum):
        ptype, _, offset, vaddr, _, filesz = struct.unpack_from('<IIQQQQ', data, phoff + i * phentsize)
        if ptype == 1:
            loads.append((vaddr, offset, filesz))
        elif ptype == 2:
            dynamic = (offset, filesz)
    def file_offset(address):
        for vaddr, offset, filesz in loads:
            if vaddr <= address < vaddr + filesz:
                return address - vaddr + offset
        sys.exit('dynamic address outside the load segments')
    needed, strtab = [], None
    for i in range(dynamic[1] // 16):
        tag, value = struct.unpack_from('<qQ', data, dynamic[0] + i * 16)
        if tag == 1:
            needed.append(value)
        elif tag == 5:
            strtab = file_offset(value)
    return [data[strtab + n:data.index(b'\0', strtab + n)].decode() for n in needed]


def version_key(version):
    return tuple(int(part) for part in version.split('.'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', default='build-handheld')
    parser.add_argument('--release-version', default=os.environ.get('TOOIE_RELEASE_VERSION') or None)
    args = parser.parse_args()

    build = ROOT / args.build_dir
    cache = (build / 'CMakeCache.txt').read_text(encoding='utf-8', errors='replace')
    if 'TK64_MOBILE:BOOL=ON' not in cache:
        sys.exit(f'{build} is not a TK64_MOBILE build')

    describe = git('describe', '--tags', '--always', '--dirty')
    version = f'v{args.release_version}' if args.release_version else f'dev-{describe}'
    name = f'BanjoTooieRecompiled-{version}-Linux-arm64-PortMaster'
    stage_root = ROOT / 'dist' / f'{name}.stage'
    shutil.rmtree(stage_root, ignore_errors=True)
    stage = stage_root / 'tooie'
    stage.mkdir(parents=True)

    binary = stage / 'TooieRecompiled'
    shutil.copyfile(build / 'TooieRecompiled', binary)
    subprocess.run([find_strip(), '--strip-all', str(binary)], check=True)

    data = binary.read_bytes()
    if data[18:20] != struct.pack('<H', 183):
        sys.exit('the binary is not aarch64')
    needed = elf_needed(data)
    glibc = sorted(set(re.findall(rb'GLIBC_(\d+\.\d+(?:\.\d+)?)\0', data)), key=lambda v: version_key(v.decode()))
    min_glibc = glibc[-1].decode()

    shutil.copytree(build / 'assets', stage / 'assets')
    (stage / 'ui_assets').mkdir()
    shutil.copyfile(ROOT / 'ui_assets/TooieLogo.svg', stage / 'ui_assets/TooieLogo.svg')
    (stage / 'icons').mkdir()
    shutil.copyfile(ROOT / 'icons/app.png', stage / 'icons/app.png')
    shutil.copyfile(ROOT / 'recompcontrollerdb.txt', stage / 'recompcontrollerdb.txt')
    shutil.copyfile(ROOT / 'LICENSE', stage / 'LICENSE.txt')
    shutil.copyfile(HERE / 'graphics.json', stage / 'graphics.default.json')
    shutil.copyfile(HERE / 'gameinfo.xml', stage / 'gameinfo.xml')
    shutil.copyfile(HERE / 'README-PORTMASTER.txt', stage / 'README.txt')
    (stage / 'portable.txt').write_bytes(b'')
    for directory in LICENSE_DIRS:
        for path in (ROOT / directory).rglob('*'):
            if path.is_file() and re.match(r'^(LICENSE|LICENCE|COPYING)', path.name):
                target = stage / 'licenses' / path.relative_to(ROOT)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)

    (stage / 'DEPENDENCIES.txt').write_text(
        '# Libraries TooieRecompiled loads from the firmware; none is bundled.\n'
        + ''.join(f'{lib}\tSYSTEM\n' for lib in needed)
        + f'# Newest glibc symbol version: {min_glibc}. EGL, OpenGL ES and libwayland-egl are loaded at run time.\n', encoding='utf-8', newline='\n')

    head = git('rev-parse', 'HEAD')
    dirty = git('status', '--short').splitlines()
    (stage / 'BUILD-INFO.txt').write_text(
        f'commit={head}\ndescribe={describe}\nrelease_version={args.release_version or ""}\nbuild=TK64_MOBILE\narch=aarch64\n'
        f'dirty_files={len(dirty)}\n' + ''.join(f'dirty {line}\n' for line in dirty), encoding='utf-8', newline='\n')

    port = {
        'version': 4,
        'name': 'tooie.zip',
        'items': ['Banjo-Tooie.sh', 'tooie/'],
        'items_opt': None,
        'attr': {
            'title': 'Banjo-Tooie Recompiled',
            'porter': ['TooieRecomp'],
            'desc': 'Native recompilation of Banjo-Tooie (N64) with the TK64 renderer. Requires your own copy of the game.',
            'desc_md': None,
            'inst': 'Copy your Banjo-Tooie (USA) ROM to ports/tooie: 33554432 bytes, .z64 SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3. Needs a firmware with Wayland or X11. See README.txt.',
            'inst_md': None,
            'genres': ['platformer'],
            'image': {'screenshot': None, 'covers': [], 'thumbnail': None, 'video': None},
            'rtr': False,
            'exp': True,
            'runtime': [],
            'store': [],
            'availability': 'paid',
            'reqs': [],
            'arch': ['aarch64'],
            'min_glibc': min_glibc,
        },
    }
    (stage / 'port.json').write_text(json.dumps(port, indent=4) + '\n', encoding='utf-8', newline='\n')

    launcher = (HERE / 'Banjo-Tooie.sh').read_bytes().replace(b'\r\n', b'\n')
    (stage_root / 'Banjo-Tooie.sh').write_bytes(launcher)

    # MANIFEST last, over everything else in the folder.
    files = sorted(p for p in stage.rglob('*') if p.is_file())
    (stage / 'MANIFEST.sha256').write_text(''.join(f'{sha256(p)}  {p.relative_to(stage).as_posix()}\n' for p in files), encoding='utf-8', newline='\n')

    entries = sorted(p for p in stage_root.rglob('*') if p.is_file())
    for path in entries:
        rel = path.relative_to(stage_root)
        for part in rel.parts:
            if FORBIDDEN_NAME.search(part):
                sys.exit(f'refused, looks like game or run data: {rel}')
        if path.stat().st_size > MAX_FILE_BYTES:
            sys.exit(f'refused, larger than any file this package should hold: {rel}')

    zip_path = ROOT / 'dist' / f'{name}.zip'
    executable = {'Banjo-Tooie.sh', 'tooie/TooieRecompiled'}
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in entries:
            arcname = path.relative_to(stage_root).as_posix()
            info = zipfile.ZipInfo(arcname, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            mode = 0o755 if arcname in executable else 0o644
            info.external_attr = (stat.S_IFREG | mode) << 16
            archive.writestr(info, path.read_bytes())

    digest = sha256(zip_path)
    (zip_path.parent / f'{zip_path.name}.sha256').write_text(f'{digest}  {zip_path.name}\n', encoding='utf-8', newline='\n')
    shutil.rmtree(stage_root)
    print(f'{zip_path}\n{zip_path.stat().st_size} bytes, {len(entries)} files, sha256 {digest}')
    print(f'needs: {" ".join(needed)}; glibc {min_glibc}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
