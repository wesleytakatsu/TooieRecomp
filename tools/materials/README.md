# Material packs (author tool)

`make_material_pack.py` builds a **material-only** texture pack (normal and occlusion/roughness/metal maps) for an
existing HD color pack. The project distributes no maps: the author runs it on their own PNG maps.

Status: the TK64 renderer reads and loads material packs (TK64 `docs/specs/materials.md`), but the game's `lib/rt64`
does not have that support yet and nothing is shaded with the maps yet. Until then the packs only serve testing.

## Inputs

- A PBR folder with `manifesto.json` (`texturas` -> name -> `hashes_rt64`, `normal`, `orm`, `material`, `dimensoes`),
  `Normal/` (tangent space, +Y up) and `ORM/` (R occlusion, G roughness, B metal) PNG maps.
- The color pack (`.rtz`, `.zip` or folder) that replaces the same hashes with DDS colors. Textures the color pack does
  not replace are skipped (listed on stderr).
- Microsoft `texconv` (DirectXTex) and RT64/TK64 `texture_packer` built from TK64 (branch `feat/material-pack-tools` or
  later; older ones do not pack maps).

## What it does

1. Reads the size and mip count of each color DDS.
2. Converts each map with `texconv` to that size and mip count: normal -> `BC5_UNORM`, ORM -> `BC7_UNORM`, linear
   (`--ignore-srgb`). Maps go next to the color: `<color dir>/Normal/<name>.dds`, `<color dir>/ORM/<name>.dds`.
3. Checks every DDS written (format, size, mips).
4. Writes `rt64.json` (`materialVersion` 1, one material-only entry per hash, `mod.json` in `extraFiles`) and `mod.json`.
5. Runs `texture_packer --create-low-mip-cache` and `--create-pack` (zstd): `<out>/<out folder name>.rtz`.

`--skip-existing` keeps maps already converted and correct, so an interrupted run resumes. `--limit N` converts only N
textures for a quick test. Mips come from texconv's box filter; BC5 keeps only X and Y, Z is rebuilt in the shader.

## Example

```powershell
python -I -X utf8 tools\materials\make_material_pack.py `
  --pbr "F:\...\Spiral Mountain HD\PBR 64x" `
  --color-pack "F:\...\Spiral Mountain HD\Tooie-Spiral-Mountain-HD-Completa-64x.rtz" `
  --texconv "<path>\texconv.exe" --texture-packer "<build>\rt64\src\tools\texture_packer\texture_packer.exe" `
  --out "F:\tk64-work\Tooie-Spiral-Mountain-HD-PBR" --mod-id tooie_spiral_mountain_hd_pbr `
  --display-name "Spiral Mountain HD PBR" --version 1.0.0 --jobs 4 --skip-existing
```

Enable the resulting pack together with the color pack. Order in the mod list matters only if two packs define a
material for the same hash (the one higher in the list wins).
