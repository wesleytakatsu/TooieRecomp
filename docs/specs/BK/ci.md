---
title: "BK — CI/CD & Release Automation"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - ci
  - release
---

# BK — CI/CD & Release

The four GitHub Actions workflows under
`BanjoRecomp/.github/workflows/`. The repo's builds are fully automated, so
a portable release of the recomp can be regenerated from CI alone.

## `validate` (the shared matrix workflow)

Called by both internal and external PR flows with `workflow_call`
(inputs + secrets). Parameters:

- `SDL2_VERSION` — default `2.30.3`
- `N64RECOMP_COMMIT` — pinned upstream `N64Recomp` commit
  (default `2b6f05688de2abc7d86da5b4a89b84c2c6acbabe`)
- Secrets `SECRET_NAME` + `SECRET_TOKEN` — private repo holding the ROM

Jobs (matrix type `Debug` / `Release`):

1. `build-linux` — `ubuntu-22.04`, native x64, ccache, `clang`.
2. `build-linux-arm64` — `ubuntu-22.04-arm`, native ARM64.
3. `build-linux-flatpak` — Flatpak packaging (`Build/Linux/flatpak`).
4. `build-windows` — `windows-latest`, `Debug`/`RelWithDebInfo`.

Each job: `actions/checkout@v4` with submodules, ccache, install deps
(ninja, lld, clang, `libgtk-3-dev`, SDL2), fetch the ROM from the secret
repo into `extra/`, build `N64Recomp` + `RSPRecomp` at the pinned commit,
recompile the game (`./N64Recomp banjo.us.rev0.toml`, etc.), then CMake
build `BanjoRecompiled` and archive it as
`BanjoRecompiled-<os>-<arch>-<config>`.

## `validate-internal.yml`

- Triggers: push to `main`, or PRs from the same repo.
- Calls `validate.yml` with `secrets: inherit`.

## `validate-external.yml`

- Triggers: `pull_request_target` for PRs from **forks**.
- An `authorize` gate step runs first; only approved PRs build.

## `update-pr-artifacts.yml`

`workflow_run` on `validate-external`/`validate-internal` success. Finds the
PR by head SHA, then rewrites the PR body with **Build Artifacts** links
(powered by nightly.link) so contributors can download the fresh builds.

## Takeaway for TooieRecomp

The whole release pipeline is generic: swap ROM paths, TOML, and the `rsp`
config and the same four workflows can build and publish a Tooie build.

See [CI (BT)](../BT/ci.md) for the decomp repo's different automation.