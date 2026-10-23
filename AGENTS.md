# AGENTS.md — TooieRecomp

This file applies to the entire repository unless a deeper `AGENTS.md` overrides it for a subtree.

The detailed project workflow lives in [`docs/AGENTS.md`](docs/AGENTS.md). Read and obey that file before making technical changes.

## Workspace root

Start CLI and IDE agents in **this directory** (`TooieRecomp/`). Project MCP
configs (`.mcp.json`, `.vscode/mcp.json`, `opencode.json`, `.codex/config.toml`,
`.agents/mcp_config.json`) bind markdown-vault to `docs/`. Do not open
`N64-Recomp/` as the workspace. Shared sources are vendored under `lib/`, `decomp/` and `assets/`; normal
builds require no sibling checkout, no BanjoRecomp checkout, and no submodules.

The canonical ROM is strictly an explicit private input (`Banjo-Tooie (USA).z64`, size `33554432` bytes, MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`), git-ignored and never versioned. The repository contains zero game data; game assets are read from the verified decompressed ROM image (`35098224` bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`), which the runtime launcher strictly requires. `assets/` contains all restored native UI menu resources, animations, character vector art, typography fonts, and styling (managed via `TooieUIAssets`). The normal build is completely kit-free; the legacy IDO/gcc compiler kit is strictly an optional maintainer-only prerequisite for decomp/metadata regeneration (`TOOIE_TOOLCHAIN_KIT`, 34 files verified against `tools/toolchain.sha256`). The optional Angrylion oracle retains its separately documented setup; it is not required to build the game.

## Required read order

Before changing code, build logic, patches or docs:

1. Read this file.
2. Read `docs/AGENTS.md`.
3. Read `docs/HANDOFF.md` (current state, open items, next action).
4. Read only the specs (`docs/specs/Tooie/**`) and reverse-engineering pages (`docs/reverse-engineering/**`) for the area of the task.
5. Select one bounded objective before editing.

Do not reopen conclusions explicitly marked `REFUTED` or `CORRECTED/SUPERSEDED` unless new primary evidence contradicts them.

## Current guardrails (render)

- Widescreen: production culls with native-style 16:9 planes (volume aspect `+0xE8` widened only while
  `800CA1B0`/`800CA218`/`800CA2A8` build the lateral vectors; projection `800CB124` stays 4:3 for RT64). Attract demos
  (`0x80127632 == 5`) keep the original planes. Kill switch `TOOIE_TEST_WS_NATIVE_PLANES=0`. Draw-only forcing in
  `800DE498` is `REFUTED` as a fix (frozen edge actors, draws through walls). Do not combine the
  `TOOIE_TEST_DISABLE_*_CULL` diagnostic flags. Do not return to GPU-side investigation for culling.
- Interpolation: RT64 pairs untagged WORLD transforms by draw order and applies the P09 guards (align, camera cut, segment
  and joint coherence, spike guard); the elementwise closure guard is default OFF. See
  `docs/specs/Tooie/graphics-and-presentation.md`.
- Intro walls in 16:9 (`T_INTRO`) and ultrawide (> 16:9) are a separate open residual.

## Source-of-truth hierarchy

Use the repository, not chat history, as canonical state.

- `docs/HANDOFF.md`: concise global current state, open items and the next action.
- `docs/specs/**`: stable contracts of the current system only.
- `docs/reverse-engineering/**`: current reverse-engineering knowledge (what the game is).
- Git commits/patch artifacts: implementation history, investigation details and reproducibility (there is no evidence
  folder; plans and evidence were removed on 2026-10-23 and remain in git history).

Classify conclusions explicitly when documenting them:

- `PROVEN`
- `STRONG_INFERENCE`
- `HYPOTHESIS`
- `INFERENCE`
- `UNKNOWN`
- `REFUTED`
- `CORRECTED` / `CORRECTED/SUPERSEDED`
- `DISPUTED`

Never promote a disputed hypothesis into a spec. Canonical RE pages must never silently promote a hypothesis.

Whenever an investigation proves or materially corrects knowledge about a function role or arguments, struct field/offset, resource ID, asset type, model/animation identity, overlay, bone/tag/part, GeoCmd type, matrix/render path, state machine, audio system, map structure, or reusable engine behavior, evaluate whether it belongs in `docs/reverse-engineering/`. Canonical pages hold the current best model; provenance goes in the commit message.

## Generated code and ownership of fixes

Do **not** manually edit generated recompilation output, including `RecompiledFuncs/**`.

Fix the owning layer instead:

- N64Recomp generator / metadata for recompilation-generation defects;
- N64ModernRuntime / ultramodern for runtime semantics;
- TooieRecomp integration/source for project-specific integration;
- versioned patch artifacts for external dependency modifications.

Generated output may be regenerated only through the repository's documented tooling.

## External dependency / dirty-worktree safety

The runtime and N64Recomp checkouts used by TooieRecomp may intentionally contain documented local changes. Treat existing dirty state as protected evidence.

Before touching an external dependency:

```sh
git rev-parse HEAD
git status --short
git diff --name-status
git diff
```

For submodules, also record `git submodule status` and inspect the submodule worktree separately.

Never use destructive cleanup on a worktree with pre-existing changes unless the task explicitly authorizes it. In particular, do not use these as a shortcut:

```text
git reset --hard
git checkout .
git restore .
git clean
git stash
```

Do not silently fold unrelated dirty files into a new patch or commit.

When dependency changes are part of a TooieRecomp fix, follow the repository's vendoring mechanism: dependencies are vendored regular files under `lib/` and `decomp/` (provenance in `vendor/`); record a local change as `vendor/patches-local/*.patch` and refresh `vendor/MANIFEST.sha256` with `tools/vendor_manifest.sh` (see `vendor/UPDATING.md`) unless the active task explicitly defines another policy. The retired `tools/apply_runtime_patches.sh`/`tools/bootstrap_deps.sh` no longer exist.

Do not push changes to upstream third-party repositories unless explicitly requested and consistent with project policy.

## Git discipline

Before editing, establish the current HEAD and worktree state.

Before every commit:

```sh
git status --short
git diff
git diff --check
```

If staging is required, stage intended paths explicitly. Avoid `git add -A`, `git add .`, and `git commit -a` when unrelated or pre-existing changes exist.

Review the staged diff before committing:

```sh
git diff --cached --name-status
git diff --cached
git diff --cached --check
```

Do not commit ROMs, proprietary assets, core dumps, large transient dumps, build outputs, or temporary diagnostic artifacts.

## Runtime/recompilation invariants

Preserve original Banjo-Tooie/N64 observable behavior before enhancements:

- timing and thread-priority semantics;
- interrupt/message ordering semantics;
- original 4:3 presentation unless a later phase intentionally changes it;
- correct overlay/function/data relocation behavior;
- fail-fast behavior for unknown symbols, relocations, syscalls, or RSP tasks where the project requires it.

Do not fix a synchronization/lifecycle defect by force-clearing guest state or skipping original work unless primary evidence proves that behavior is faithful.

## Experiments and diagnostics

Prefer the smallest discriminator that separates competing hypotheses.

For temporary diagnostic code:

- keep it at the real source layer, never generated C;
- make it reversible;
- record the exact diff/command/result in the commit message;
- distinguish timing-perturbed runs from clean ground truth;
- revert temporary diagnostic code before a docs-only checkpoint unless the active task explicitly promotes it into the implementation.

Never report a test as `PASS` unless it was actually run. Mark unimplemented future tests as `SKIPPED` / `NOT_IMPLEMENTED`, not passing.

## Documentation is part of the task

A technical task is not complete when only the code or experiment is done.

When the state changes, review and update as applicable:

- `docs/HANDOFF.md`;
- related specs;
- reverse-engineering pages when knowledge of the game changed;
- indexes/vault navigation (`docs/VAULT.md`).

Specs describe stable contracts; handoffs describe current operational state; commits record how it was shown.

When models/agents disagree, preserve both claims and record the arbitration/proof instead of silently choosing one.

Run the documented documentation checks before closing a task, including `markdown_vault_audit` when available and `git diff --check`.

## Build and validation

Use commands already documented by the specs and `tools/` docs. Do not invent a new build or dependency workflow if the repository already has one.

For a behavioral/runtime change, validate at the narrowest useful levels first (unit/harness), then integration/build, then the relevant clean game reproduction.

Record:

- exact command;
- exit code;
- relevant HEADs;
- patch/artifact hashes when applicable;
- observed result and acceptance criterion.

## Scope control

Work on one bounded objective at a time. If a fix reveals a new downstream blocker, record it and stop unless the task explicitly authorizes continuing into that blocker.

Do not mix unrelated cleanup, refactors, speculative fixes, or downstream debugging into an implementation checkpoint.

## Completion checklist

Before handing off:

- code/patch scope matches the requested task;
- generated code was not hand-edited;
- pre-existing dirty state was preserved;
- required tests/builds actually ran;
- docs/specs were reviewed and updated where appropriate;
- `git diff --check` passes for intended changes;
- worktree status is explicitly reported;
- commit/push state is recorded;
- `NEXT_SINGLE_ACTION` is concrete and singular.

<!-- graft:start -->
## Graft — repo context graph

This repo is indexed in `graft/`: small linked markdown nodes that explain each
system and carry exact file:line spans, kept in sync with the code through git.

For ANY task here — understanding how something works, finding where code lives,
or scoping a change — get context from the graph before grepping or opening
source files. Re-ask freely (it's cheap) and reuse literal identifiers you
already have (symbol, error string, file name) as the query. New to this repo?
Run `graft map` first — a token-budgeted orientation (dir clusters, hubs,
hotspots), no LLM, no key.

- Run `graft ask "<your question>" --source` → ranked nodes with the relevant
  code spans inlined (each hit's ≤8-line crux by default; `--full` for whole
  definitions when the crux isn't enough). Match the tool to the task shape:
  for understanding or editing, the top node IS the answer — cite its
  `covers:` file:line spans and edit straight from `--source`. For
  exhaustive tasks ("every occurrence / every caller of this pattern"), ranked
  results are top-N, not complete — run `graft grep "<literal>"` instead
  (exhaustive over indexed files, grouped by enclosing symbol), falling back
  to raw `grep -rn` only for unindexed files.
- `graft skeleton <file>` → every definition's signature + span, ~10× cheaper
  than reading the file; use it to skim an API surface.
- `graft callers <symbol>` gives precomputed, exact edges — who calls this.
  Add `--direction out` for what it calls, or `--depth N` to walk
  transitively for the full blast radius. For structural questions, skip
  ranking and use this directly.
- Or browse: `graft/INDEX.md` lists every node; follow the links.
- Monorepos and folders of multiple repos rank fairly across sub-projects —
  hits carry `[scope/]` labels naming which one they're from. Narrow with
  `graft ask "<task>" --in <scope>/` once you know where you're working.

If a returned span is truncated ("+N more lines"), open the file at that exact
range before finalizing. Only open source files when a node genuinely lacks a
needed detail, and then at the exact file:line the node points to — never
re-read whole files.

After big code changes, refresh the graph with `graft build` (deterministic,
no API key, $0).
<!-- graft:end -->
