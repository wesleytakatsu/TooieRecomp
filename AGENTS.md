# AGENTS.md — TooieRecomp

This file applies to the entire repository unless a deeper `AGENTS.md` overrides it for a subtree.

The detailed project workflow lives in [`docs/AGENTS.md`](docs/AGENTS.md). Read and obey that file before making technical changes.

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
