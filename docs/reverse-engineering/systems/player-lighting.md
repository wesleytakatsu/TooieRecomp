---
title: "Player lighting and shared model vertices"
description: "Per-player light state, CPU vertex-color lighting and private output vertices for experimental story split screen."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-02
tags: [reverse-engineering, player, lighting, models, split-screen]
---

# Player lighting and shared model vertices

## Current model

| Claim | Status and primary evidence |
| --- | --- |
| Player lighting inputs are per player, rather than one shared RGB value | `PROVEN`: `func_800A1F0C` reads RGB floats from `PlayerState+0x108 -> +0x4/+0x8/+0xC`; `func_800A2060` updates them. `func_800A235C` builds the light list through `PlayerState+0xB4 -> +0x4`, using that player's position. The two-player runtime trace showed distinct state and light-list pointers. |
| Player model creation does not always allocate output vertices per instance | `PROVEN`: `func_80092EC8` and the LOD setter `func_800941C8` request private vertices from `func_80100D24` only when `func_800B26F0(model)` finds the model's `+0x28` data. Lighting alone does not request them. The secondary-model setter `func_8009359C` has the same condition. |
| The model renderer falls back to vertices inside the shared model asset | `PROVEN`: at `0x800DE7A8..0x800DE7BC`, `func_800DE498` fills an absent render-context `+0x1C` with `model_bin + u32(model_bin+0x10)`. The runtime trace observed the same output pointer for J1 and J2 despite distinct player model states/instance handles. |
| Lighting is applied by writing CPU vertex colors | `PROVEN`: the renderer selects the lighting callbacks through `0x80123030`; mode 1 uses `func_8010E2C4`, `func_8010E4C4` and `func_8010E7F4`. `func_8010E4C4` gets the output vertex list through `func_800DF8B4` (render-context `+0x1C`) and writes RGB at `Vtx+0xC/+0xD/+0xE`, advancing 16 bytes per vertex. |
| J2 can overwrite J1's already-computed colors | `PROVEN`: a read-only diagnostic hashed J1's output RGB immediately before and after J2's complete draw. With private vertices disabled, 7 of 40 checks changed; both players used the same output vertex lists. This was an instrumented Windows attract-map reproduction, not a manual story-save playtest. |
| Private output vertices resolve the reported movement-dependent brightness/color flicker | `PROVEN` for the reported Windows reproduction: the user confirmed the graphical lighting problem resolved on 2026-10-31, after the instrumented A/B had demonstrated the shared output writes and their removal. |

The global CPU lighting scratch context at `0x80137060` and cached packed lighting records are not by themselves proof
of an error: draws use them serially. The demonstrated conflict is in the output vertices referenced by the display
lists, not the identity of the player's light-input state.

## Integration fix

The experimental story mode now requests one private output vertex buffer for each lit player model that originally
requested none. It uses the native `func_80100D24` allocator and `func_800B2400` vertex-list allocation/copy; existing
instance retirement and defragmentation retain ownership. It does not copy the whole model or create host-owned guest
buffers.

Scope is established around the primary (`0x80092EC8`), LOD (`0x800941C8`) and secondary (`0x8009359C`) player-model
setters, then consumed at the model-instance allocator (`0x80100D24`). During initial creation, player objects are
not yet published in `D_80135490`, so the story-player creation scope also admits their model setters. Later setters
must belong to slot 0 or 1. The creation hook latches this scope only when an original one-player request becomes the
experimental two-player story request; an original multiplayer/minigame request does not enter it.

Only `a2 == 0 && a3 != 0` is changed to `a2 = 1`; existing private-buffer requests, unlit models and other allocation
callers retain their arguments. `TOOIE_TEST_2P_PRIVATE_VERTICES=0` disables the fix for A/B reproduction; default is on.
The source owner is `src/game/tooie_dev_warp_hook.cpp`; registrations are in `src/game/boot_hw_stubs.cpp`.

## Manual capture

`TOOIE_DIAG_2P_LIGHTING_LOG=<file>` enables a default-OFF, read-only allocation trace. It records map creation and
primary/LOD/secondary model allocations, with player owner/slot, requested and actual private-buffer counts, model and
output pointers, and light/tint-state pointers. Slot `-1` during initial creation means the player has not yet been
published in the slot table. Output addresses can be reused after an instance is retired.

The trace reads native instance metadata using `freelist_at`'s layout (`0x800B301C`: pool + 4 + index * signed stride),
without calling additional guest functions. It installs no per-frame probes and performs no RGB hashing. Allocation
logs identify potentially shared outputs; they do not prove an RGB overwrite or establish the visual outcome.

On Windows, [Test-SplitScreenLighting.ps1](../../../tools/windows/Test-SplitScreenLighting.ps1) creates a fresh log
directory, backs up saves/configs and captures the trace, stdout, stderr and build identity. The user enables Dev Mode
and two players normally, records video separately and closes the game window to finish. `-DisablePrivateVertices`
provides the optional negative A/B case. Outside explicit capture, no lighting log is opened.

## Verification boundary

The instrumented attract reproduction forced the existing experimental option in memory; it did not save the option
or exercise independently controlled story-save gameplay. Its per-draw instrumentation patch and logs are local
temporary artifacts; the lightweight opt-in allocation logger and manual capture script are retained in source.
Historical run details remain in Git history for the retired `docs/future/two-player-split-screen.md` checkpoint;
current manual coverage is in the [test contract](../../specs/Tooie/testing-and-oracle.md#5-experimental-split-screen-manual-coverage).

The user subsequently confirmed the visual lighting fix in story mode on Windows (2026-10-31). The manual capture
completed with exit 0 and recorded primary, LOD and secondary model-allocation paths. This does not establish a complete
transformation/secondary-model playtest or Linux coverage; those remain `NOT_RUN`.

See [model renderer](model-renderer.md) and [warp/player state](warp-and-player-position.md).
