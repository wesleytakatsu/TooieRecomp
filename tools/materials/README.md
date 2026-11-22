# Material packs (author tool)

`make_material_pack.py` builds a **material-only** texture pack (normal and occlusion/roughness/metal maps) for an
existing HD color pack. The project distributes no maps: the author runs it on their own PNG maps.

Status: the TK64 renderer reads and loads material packs, relights with the normal maps and gives depth with parallax
(TK64 `docs/specs/materials.md`); the game turns it on with the Materials option. The game's vendored `lib/rt64` does
not have that support yet, so the packs only serve testing with a TK64 build.

## Inputs

- One or more PBR folders (repeat `--pbr`; the first one listed that has a hash gives its maps, so put the folder of
  the pack that is active for shared textures first) with `manifesto.json` (`texturas` -> name -> `hashes_rt64`,
  `normal`, `orm`, optional `altura` and `profundidade`, `material`, `tratamento`, `dimensoes`), `Normal/` (tangent
  space, +Y up) and `ORM/` (R occlusion, G roughness, B metal) PNG maps, and optionally `Height/` (16-bit gray, white =
  top) for parallax.
- The color packs (`.rtz`, `.zip` or folder) that replace the same hashes with DDS colors: repeat `--color-pack` when
  the colors are split across packs (for example an area pack and a common-textures pack); the first pack listed that
  replaces a hash gives the size. All must use the same `hashVersion`. Textures no color pack replaces are skipped
  (listed on stderr).
- Python with NumPy and Pillow when there are heights (they are merged into the ORM PNG before `texconv`).
- Microsoft `texconv` (DirectXTex) and RT64/TK64 `texture_packer` built from TK64 (branch `feat/material-pack-tools` or
  later; older ones do not pack maps).

## What it does

1. Reads the size and mip count of each color DDS.
2. Converts each map with `texconv` with that mip count: normal -> `BC5_UNORM`, ORM -> `BC7_UNORM`, linear
   (`--ignore-srgb`). Maps are not resized: a map smaller or larger than its color by a power of two (same aspect,
   `dimensoes` in the manifest) keeps its size and gets as many mips fewer or more; other sizes are refused. Maps go next to the color: `<color dir>/Normal/<name>.dds`, `<color dir>/ORM/<name>.dds`.
3. Checks every DDS written (format, size, mips).
   With a height and a `profundidade` above 0, the height goes into the ORM map's alpha (8 bits) and the material
   gets `depth` (parallax); water (`ondas_estaticas`) gets none unless `--parallax-water`, and `--no-parallax` skips all.
4. Writes `rt64.json` (`materialVersion` 1, one material-only entry per hash, `mod.json` in `extraFiles`) and `mod.json`.
5. Runs `texture_packer --create-low-mip-cache` and `--create-pack` (zstd): `<out>/<out folder name>.rtz`.

`--skip-existing` keeps maps already converted and correct, so an interrupted run resumes. `--limit N` or `--only NAME`
converts only some textures for a quick test. Mips come from texconv's box filter; BC5 keeps only X and Y, Z is
rebuilt in the shader.

## Example

```powershell
python -X utf8 tools\materials\make_material_pack.py `
  --pbr "F:\...\Texturas Comuns HD\PBR 64x" --pbr "F:\...\Spiral Mountain HD\PBR 64x" `
  --color-pack "F:\...\Spiral Mountain HD\Tooie-Spiral-Mountain-HD-Completa-64x.rtz" `
  --color-pack "F:\...\Texturas Comuns HD\Tooie-Texturas-Comuns-HD-64x.rtz" `
  --texconv "<path>\texconv.exe" --texture-packer "<build>\rt64\src\tools\texture_packer\texture_packer.exe" `
  --out "F:\tk64-work\Tooie-HD-PBR" --mod-id tooie_hd_pbr `
  --display-name "Tooie HD PBR" --version 1.1.0 --jobs 4 --skip-existing
```

Enable the resulting pack together with the color pack. Order in the mod list matters only if two packs define a
material for the same hash (the one higher in the list wins).
