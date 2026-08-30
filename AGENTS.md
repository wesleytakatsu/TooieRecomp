# AGENTS.md — TooieRecomp

This file applies to the entire repository unless a deeper `AGENTS.md` overrides it for a subtree.

The detailed project workflow lives in [`docs/AGENTS.md`](docs/AGENTS.md). Read and obey that file before making technical changes.

## Workspace root

Start CLI and IDE agents in **this directory** (`TooieRecomp/`). Project MCP
configs (`.mcp.json`, `.vscode/mcp.json`, `opencode.json`, `.codex/config.toml`,
`.agents/mcp_config.json`) bind markdown-vault to `docs/`. Do not open
`N64-Recomp/` as the workspace. Sibling checkouts (`BanjoRecomp`, ROM,
`banjo-tooie-Recomp`) stay next to this folder for CMake and the Angrylion
oracle; they are not this git repository.

## Required read order

Before changing code, build logic, patches, plans, or specs:

1. Read this file.
2. Read `docs/AGENTS.md`.
3. Read `docs/plan/index.md`.
4. Locate the active BT phase in `docs/plans/BT/index.md` and read that phase.
5. Read only the BK/BT specs referenced by the active phase/task.
6. Read `docs/HANDOFF.md` and the current session handoff/evidence referenced there.
7. Select one open task / one bounded objective before editing.

Do not reopen hypotheses explicitly marked `REFUTED`, `CORRECTED/SUPERSEDED`, or resolved unless new primary evidence contradicts them.

Active P08 residual: `WIDESCREEN_EDGE_GEOMETRY`. Canonical state is
`docs/HANDOFF.md` and `docs/plan/evidence/p08-session-handoff.md`.
`gspropsDll_entrypoint_1` is a one-shot type-6 load of all gccubes, not
per-frame visibility. Type-6 per-frame consumer is `800FF62C` over
`D_80136EE0`; vis is Euclidean `800EFFB4` (NOT aspect/FOV); TYPE6 vis CF
`NO_CHANGE`. Sprite types 7/9/C (`800CBB18` is type 0xC, not 0xA) dispatch
at map load via `gccube_ep6`; `800F0064` is world XZ membership, not
camera/FOV/aspect; GSPROPS vis CF is `PARTIAL_HIT` on inserts and
`REFUTED` as Expand pop. Bound-sprite `800B3CDC` products live in
`D_8012AE80+0xE0`; per-frame consumers are spatial queries (`800CB8F0`
LIVE 5777) with `draw_fn=NONE`; `BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT`
is `REFUTED`. Do not wrap `80108DC0` as vis. Do not treat `800F0064` as
a FOV gate. Do not reopen bound-sprite draw. Do not combine
`TOOIE_TEST_DISABLE_MAP_VIS_CULL`, `TOOIE_TEST_DISABLE_MODEL_GEO_CULL`,
`TOOIE_TEST_DISABLE_PROP_CELL_VIS_CULL`, `TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL`,
or `TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL`.
`8011458C` is occupancy/near-fog via `801143A4`, not actor draw.
Actor chain `80114184`→`800BD708`→`800CD23C`→`800EA628`→`80103508`→`8010108C`.
`TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB` is `PARTIAL_HIT` visitation /
`NO_CHANGE` draw-DL (`03508=3` `0108c=0`).
`ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT` is `REFUTED`.
`80108C14` is a rare `D_80136EE0` submit (`LIVE` 2 vs `800FF62C=1337`),
same type-6 objects, GETTER jalr not DRAW, RESOURCE gates only.
`80108C14_AS_REMAINING_DRAW_ROOT` is `REFUTED`.
`REMAINING_STATIC_DECORATIVE_VIEW_CULL_AS_EXPAND_ROOT` is `REFUTED`.
Prior “intra-model GeoCmd analog untested” is `CORRECTED/SUPERSEDED`:
it was `800E3DC0`/`800CAB70` (BK GeoCmdD homolog; `E3DC0_DIRECT_ROOT`
already `REFUTED`). No distinct extra GeoCmd layer. `800CAA24` aspect-aware
H-plane Expand is **production default ON** (user visual
`VISUALLY_CONFIRMED_FOR_TESTED_GAMEPLAY_MAP_GEOMETRY`; `TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0`
forces original). `ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED`
(do not say globally actor/pickup view cull REFUTED).
`TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED`;
`TYPE6_GAMEPLAY_OBJECT_POP_ROOT: REOPENED_TARGETED`.
Screenshot A/B/C object pop is `PROVEN` (object-specific, shared with demo).
Corrected object-pop diagnostic captured gameplay `chsilo`/`chnests`.
A/B transitions are `OBJECT_MODEL_FRUSTUM_SIDE_PLANE` (`800CACEC` source
reject / target accept). User visual confirmed aspect-aware CACEC;
**production ON** unless `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`.
DRAW-only expand (`800E3E8C`/`8011447C`); original v0 for `8011458C`
and `chlightfader`. Intro walls `STILL_PRESENT` and independent
(`SOURCE_GEOMETRY`/`CAMERA_FRAMING` `STRONG_HYPOTHESIS`; map
`MAP_18B_CS_STARTUP_INTRO`). Do not return to GPU. Do not combine vis
flags. Do not implement an intro production fix until MAP_18B extents
are dumped. Keep original `800CACEC` v0 for demo/callback consumers.
`CLEAN_PINNED_BUILD_VALIDATION: PASS` (`23c070a9` + clean RT64 `6f1c2d99`;
artifact `/tmp/p08-clean-milestone-validation-20260830T035059Z/`).

## Source-of-truth hierarchy

Use the repository, not chat history, as canonical state.

- `docs/HANDOFF.md`: concise global current state.
- `docs/plan/evidence/*session-handoff*.md`: operational state sufficient for a fresh agent/chat.
- `docs/plans/**`: phase/task status, gates, residual work, next action.
- `docs/specs/**`: stable contracts only.
- `docs/plan/evidence/**`: experiments, dumps, hypotheses, arbitration, and proof.
- Git commits/patch artifacts: exact implementation history and reproducibility.

Classify conclusions explicitly when documenting them:

- `PROVEN`
- `REFUTED`
- `CORRECTED/SUPERSEDED`
- `INFERENCE`
- `DISPUTED`
- `UNKNOWN`

Never promote a disputed hypothesis into a spec.

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

When dependency changes are part of a TooieRecomp fix, follow the repository's existing patch/versioning mechanism (for example `patches/runtime/` plus `tools/apply_runtime_patches.sh`) unless the active task explicitly defines another policy.

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
- record the exact diff/command/result in evidence;
- distinguish timing-perturbed runs from clean ground truth;
- revert temporary diagnostic code before a docs-only checkpoint unless the active task explicitly promotes it into the implementation.

Never report a test as `PASS` unless it was actually run. Mark unimplemented future tests as `SKIPPED` / `NOT_IMPLEMENTED`, not passing.

## Documentation is part of the task

A technical task is not complete when only the code or experiment is done.

When the state changes, review and update as applicable:

- task-specific evidence;
- current session handoff;
- `docs/HANDOFF.md`;
- active phase plan;
- related specs;
- indexes/vault navigation.

Specs describe stable contracts; evidence records investigation and uncertainty; handoffs describe current operational state.

When models/agents disagree, preserve both claims and record the arbitration/proof instead of silently choosing one.

Run the documented documentation checks before closing a task, including `markdown_vault_audit` when available and `git diff --check`.

## Build and validation

Use commands already documented by the active phase/specs/evidence. Do not invent a new build or dependency workflow if the repository already has one.

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
- docs/phase/specs were reviewed and updated where appropriate;
- `git diff --check` passes for intended changes;
- worktree status is explicitly reported;
- commit/push state is recorded;
- `NEXT_SINGLE_ACTION` is concrete and singular.
