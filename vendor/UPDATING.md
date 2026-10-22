# Changing or refreshing vendored code

Vendored code is ordinary repository content: edit it in a normal commit. Then keep the provenance honest:

1. **Local fix inside `lib/<x>`**: also add `vendor/patches-local/<name>.patch` (paths relative to that vendored dir, `a/` `b/`), append a row to
   `vendor/patches-applied/ORDER.tsv` (group, dir, patch, sha256 via `sha256sum`, base) and, for a file under `lib/rt64`, a note in `UPSTREAMS.tsv` (`patches_integrated`).
2. `tools/vendor_manifest.sh` (rewrites `MANIFEST.sha256`, `MODES.tsv`), then `tools/vendor_verify.sh --reverse-check` must print `VENDOR VERIFY OK`.
3. **Upstream bump** (one-time online step, done by a maintainer, never by the build): fetch the official upstream at the new pin, check the commit and licence
   file bytes, replace the vendored tree (same deny-list, `vendor/EXCLUDES.txt`), re-apply `patches-local` + `patches-applied` for that group, update the pin,
   file counts and licence sha256 in `UPSTREAMS.tsv`, regenerate the manifest, re-run the clean offline build + tests, record the evidence under `docs/plan/evidence/`.
4. New prebuilt binaries are refused unless added (with sha256 + origin + licence status) to `vendor/BINARIES.tsv`.
5. Never vendor a ROM (`Banjo-Tooie (USA).z64`), decompressed ROM image, extracted game assets, saves, generated `RecompiledFuncs/`, or IDO/gcc kit bytes. The repository contains zero game assets (`assets/` is strictly reserved for native UI typography, styling, and icons).
