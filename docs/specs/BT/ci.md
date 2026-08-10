---
title: "BT — CI/CD"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - ci
---

# BT — CI/CD

The decomp repo's single workflow `.github/workflows/ci.yml`.

## `ci.yml`

Name: **Build**

- Triggers: `pull_request_target` to `master`, and `push` to `master`.
- Runs on `ubuntu-latest`.
- Matrix: single version `us`.

Steps:

1. **Checkout** — `actions/checkout@v4`, submodules recursive,
   head repo/ref (for PRs).
2. **Install packages** — `make`, `git`, `build-essential`,
   `binutils-mips-linux-gnu`, `gcc-mips-linux-gnu`, Python3, pip,
   `libfmt-dev`, `libtoml11-dev`.
3. **Pip deps** — `tools/requirements.txt`, `tools/splat/requirements.txt`.
4. **Baserom** — fetched from a private `secrets.SECRETREPO` repo using
   `secrets.SECRETTOKEN`, copied to `baserom.us.z64`.
5. **Setup** — `make setup -j $(nproc)` (ultralib, tools, IDO, decompress,
   splat split).
6. **Build** — `make -j $(nproc)`.
7. **Progress** — `python3 tools/progress.py --version us` prints matching
   stats per folder.
8. **Upload frogress** — only on `master` non-PR: pushes code progress to
   the public [Banjo-Tooie progress page](https://progress.deco.mp/tooie) via
   `tools/upload_frogress.py`.
9. **Map artifact** — uploads
   `build/us/banjotooie_decompressed.map`.

> Differs from BK: no compile-the-recomp step, no binaries. It proves the
> decomp matches byte-for-byte and keeps display.

See [Matching](matching.md) for the per-function workflow the CI validates.