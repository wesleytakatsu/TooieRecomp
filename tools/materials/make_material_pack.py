#!/usr/bin/env python3
"""Build a material-only texture pack (normal + ORM maps) for an existing color pack.

The pack follows the TK64 material contract (rt64.json "materialVersion": 1, entries with a "material" object and no
"path"), so it only adds maps to the colors another pack already replaces. The project ships no maps: this runs on the
author's machine, from the author's PNG maps.

Input: a PBR folder with manifesto.json (keys: "texturas" -> name -> {"hashes_rt64", "normal", "orm", "material",
"dimensoes"}), Normal/ and ORM/ PNG maps (normal in tangent space, +Y up; ORM = R occlusion, G roughness, B metal),
and the color pack (.rtz or folder) whose rt64.json maps the same hashes.

For each texture the maps are converted with Microsoft texconv to the size and mip count of the color texture of the
same hash: normal -> BC5_UNORM, ORM -> BC7_UNORM, linear (sRGB metadata ignored). They are placed next to the color
path (<color dir>/Normal/<name>.dds, <color dir>/ORM/<name>.dds). Then RT64's texture_packer builds the low mip cache
and the .rtz (zstd). Every DDS written is checked (format, size, mip count).

Usage (example):
  python make_material_pack.py --pbr "F:\\...\\PBR 64x" --color-pack "F:\\...\\Completa-64x.rtz" \\
      --texconv E:\\xu10\\tools\\texconv-may2026\\texconv.exe --texture-packer <build>\\texture_packer.exe \\
      --out F:\\tk64-work\\Tooie-Spiral-Mountain-HD-PBR --mod-id tooie_spiral_mountain_hd_pbr \\
      --display-name "Spiral Mountain HD PBR" --version 1.0.0
"""

import argparse
import concurrent.futures
import json
import os
import struct
import subprocess
import sys
import time
import zipfile

DXGI_BC5_UNORM = 83
DXGI_BC7_UNORM = 98
DXGI_NAMES = {83: 'BC5_UNORM', 98: 'BC7_UNORM'}
FOURCC_FORMATS = {b'DXT1': 71, b'DXT3': 74, b'DXT5': 77, b'ATI2': 83, b'BC5U': 83, b'ATI1': 80, b'BC4U': 80}


def dds_info(header):
    """Returns (width, height, mip count, DXGI format or 0) from the first 148 bytes of a DDS file."""
    if len(header) < 128 or header[:4] != b'DDS ':
        raise ValueError('not a DDS file')
    height, width = struct.unpack_from('<II', header, 12)
    mips = struct.unpack_from('<I', header, 28)[0] or 1
    fourcc = header[84:88]
    if fourcc == b'DX10':
        if len(header) < 148:
            raise ValueError('truncated DX10 header')
        fmt = struct.unpack_from('<I', header, 128)[0]
    else:
        fmt = FOURCC_FORMATS.get(fourcc, 0)
    return width, height, mips, fmt


class ColorPack:
    """Reads rt64.json and DDS headers from a color pack (.rtz/.zip or folder)."""

    def __init__(self, path):
        self.path = path
        self.zip = zipfile.ZipFile(path) if os.path.isfile(path) else None
        if self.zip is not None:
            names = {n.lower(): n for n in self.zip.namelist()}
            db_name = next((names[n] for n in names if n.endswith('rt64.json')), None)
            if db_name is None:
                raise SystemExit(f'{path}: no rt64.json')
            self.root = db_name[:-len('rt64.json')]
            self.names = names
            db = json.loads(self.zip.read(db_name).decode('utf-8'))
        else:
            self.root = ''
            with open(os.path.join(path, 'rt64.json'), encoding='utf-8') as f:
                db = json.load(f)
        self.hash_version = db.get('configuration', {}).get('hashVersion', 1)
        self.paths = {}
        for entry in db.get('textures', []):
            h = entry.get('hashes', {}).get('rt64', '').lower()
            if h and entry.get('path'):
                self.paths[h] = entry['path'].replace('\\', '/')

    def header(self, rel):
        base = rel[:-4] if rel.lower().endswith(('.dds', '.png')) else rel
        candidate = base + '.dds'
        if self.zip is not None:
            name = self.names.get((self.root + candidate).lower())
            if name is None:
                return None
            with self.zip.open(name) as f:
                return f.read(148)
        full = os.path.join(self.path, candidate)
        if not os.path.exists(full):
            return None
        with open(full, 'rb') as f:
            return f.read(148)


def run_texconv(texconv, src, out_dir, fmt, mips):
    cmd = [texconv, '-nologo', '-y', '-f', fmt, '-m', str(mips), '--ignore-srgb', '-o', out_dir, src]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f'texconv failed ({proc.returncode}) for {src}:\n{proc.stdout}\n{proc.stderr}')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--pbr', required=True, help='folder with manifesto.json, Normal/ and ORM/')
    ap.add_argument('--color-pack', required=True, help='color pack (.rtz/.zip or folder) with the same hashes')
    ap.add_argument('--texconv', required=True)
    ap.add_argument('--texture-packer', help='RT64/TK64 texture_packer; without it, no low mip cache and no .rtz')
    ap.add_argument('--out', required=True, help='output folder (the pack is <out>/<folder name>.rtz)')
    ap.add_argument('--mod-id', required=True)
    ap.add_argument('--display-name', required=True)
    ap.add_argument('--version', default='1.0.0')
    ap.add_argument('--author', default='')
    ap.add_argument('--game-id', default='bt')
    ap.add_argument('--minimum-recomp-version', default='0.1.6')
    ap.add_argument('--strength', type=float, default=1.0)
    ap.add_argument('--specular', type=float, default=0.5)
    ap.add_argument('--jobs', type=int, default=4, help='parallel texconv processes')
    ap.add_argument('--limit', type=int, default=0, help='convert only the first N textures (testing)')
    ap.add_argument('--skip-existing', action='store_true', help='keep converted DDS files that already pass the check')
    args = ap.parse_args()

    manifest_path = os.path.join(args.pbr, 'manifesto.json')
    with open(manifest_path, encoding='utf-8') as f:
        manifest = json.load(f)
    textures = manifest['texturas']
    color = ColorPack(args.color_pack)
    if color.hash_version != 5:
        print(f'warning: color pack hashVersion {color.hash_version}; the material pack uses the same', file=sys.stderr)

    # Plan every texture: color path, size and mips from the color DDS.
    jobs = []
    problems = []
    skipped = []
    for name in sorted(textures):
        entry = textures[name]
        hashes = [h.lower() for h in entry.get('hashes_rt64', [])]
        color_path = next((color.paths[h] for h in hashes if h in color.paths), None)
        if color_path is None:
            # Nothing to attach the maps to: the color pack does not replace this texture.
            skipped.append(f'{name}: no color entry for hashes {hashes}')
            continue
        header = color.header(color_path)
        if header is None:
            problems.append(f'{name}: color DDS {color_path} not found (PNG colors are not supported)')
            continue
        width, height, mips, _ = dds_info(header)
        dims = entry.get('dimensoes')
        if dims and (dims[0] != width or dims[1] != height):
            problems.append(f'{name}: maps are {dims[0]}x{dims[1]}, color is {width}x{height}')
            continue
        color_dir = os.path.dirname(color_path)
        prefix = (color_dir + '/') if color_dir else ''
        jobs.append({
            'name': name,
            'hashes': hashes,
            'kind': entry.get('material', ''),
            'width': width,
            'height': height,
            'mips': mips,
            'normal_src': os.path.join(args.pbr, entry['normal']),
            'orm_src': os.path.join(args.pbr, entry['orm']),
            'normal_rel': f'{prefix}Normal/{name}.dds',
            'orm_rel': f'{prefix}ORM/{name}.dds',
        })

    if skipped:
        print(f'Skipped {len(skipped)} textures the color pack does not replace:', file=sys.stderr)
        for p in skipped:
            print('  ' + p, file=sys.stderr)

    if problems:
        print('Problems:', file=sys.stderr)
        for p in problems:
            print('  ' + p, file=sys.stderr)
        return 1

    if args.limit > 0:
        jobs = jobs[:args.limit]

    os.makedirs(args.out, exist_ok=True)

    def expected_ok(path, fmt, job):
        if not os.path.exists(path):
            return False
        with open(path, 'rb') as f:
            w, h, m, real = dds_info(f.read(148))
        return (w, h, m, real) == (job['width'], job['height'], job['mips'], fmt)

    def convert(job):
        out = []
        for kind, fmt_name, fmt in (('normal', 'BC5_UNORM', DXGI_BC5_UNORM), ('orm', 'BC7_UNORM', DXGI_BC7_UNORM)):
            dst = os.path.join(args.out, job[kind + '_rel'])
            if args.skip_existing and expected_ok(dst, fmt, job):
                continue
            out_dir = os.path.dirname(dst)
            os.makedirs(out_dir, exist_ok=True)
            src = job[kind + '_src']
            run_texconv(args.texconv, src, out_dir, fmt_name, job['mips'])
            produced = os.path.join(out_dir, os.path.splitext(os.path.basename(src))[0] + '.dds')
            if os.path.normcase(produced) != os.path.normcase(dst):
                os.replace(produced, dst)
            if not expected_ok(dst, fmt, job):
                with open(dst, 'rb') as f:
                    got = dds_info(f.read(148))
                raise RuntimeError(f'{dst}: got {got}, expected {job["width"]}x{job["height"]} {job["mips"]} mips {DXGI_NAMES[fmt]}')
            out.append(dst)
        return job['name']

    start = time.time()
    done = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = [pool.submit(convert, j) for j in jobs]
        for fut in concurrent.futures.as_completed(futures):
            fut.result()
            done += 1
            if done % 10 == 0 or done == len(jobs):
                print(f'converted {done}/{len(jobs)} ({time.time() - start:.0f} s)', flush=True)

    # Database and mod manifest.
    entries = []
    for job in jobs:
        for h in job['hashes']:
            entries.append({
                'path': '',
                'hashes': {'rt64': h},
                'material': {
                    'normal': job['normal_rel'],
                    'normalSpace': 'tangent',
                    'normalY': 'up',
                    'orm': job['orm_rel'],
                    **({'strength': args.strength} if args.strength != 1.0 else {}),
                    **({'specular': args.specular} if args.specular != 0.5 else {}),
                    **({'kind': job['kind']} if job['kind'] else {}),
                },
            })
    db = {
        'configuration': {
            'configurationVersion': 3,
            'autoPath': 'rt64',
            'defaultOperation': 'stream',
            'defaultShift': 'half',
            'hashVersion': color.hash_version,
            'materialVersion': 1,
        },
        'textures': entries,
        'operationFilters': [],
        'shiftFilters': [],
        'extraFiles': ['mod.json'],
    }
    with open(os.path.join(args.out, 'rt64.json'), 'w', encoding='utf-8') as f:
        json.dump(db, f, indent=2)
    mod = {
        'id': args.mod_id,
        'game_id': args.game_id,
        'version': args.version,
        'minimum_recomp_version': args.minimum_recomp_version,
        'display_name': args.display_name,
        'short_description': 'Material maps (normal, ORM) for an HD texture pack.',
        'description': 'Material-only pack: enable it together with the color pack it was made for. Needs TK64 materials (D3D12 or Vulkan).',
        'authors': [args.author] if args.author else [],
        'enabled_by_default': True,
    }
    with open(os.path.join(args.out, 'mod.json'), 'w', encoding='utf-8') as f:
        json.dump(mod, f, indent=2)
    print(f'rt64.json: {len(entries)} material entries for {len(jobs)} textures')

    if args.texture_packer:
        for mode in ('--create-low-mip-cache', '--create-pack'):
            proc = subprocess.run([args.texture_packer, args.out, mode])
            if proc.returncode != 0:
                print(f'texture_packer {mode} failed ({proc.returncode})', file=sys.stderr)
                return 1
        pack = os.path.join(args.out, os.path.basename(os.path.normpath(args.out)) + '.rtz')
        print(f'pack: {pack} ({os.path.getsize(pack) / (1 << 30):.2f} GiB)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
