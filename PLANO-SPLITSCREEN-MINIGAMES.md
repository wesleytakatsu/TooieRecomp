---
title: "Implementation plan: suspend experimental story co-op in minigames"
status: checkpoint-with-known-limitation
type: implementation-plan
project: TooieRecomp
updated: 2026-11-02
baseline_commit: 034189002f15d9b6dfeece588da71d93a9bca48e
baseline_branch: codex/tk64-launcher
implementation_status: IMPLEMENTED_PARTIAL_USER_ACCEPTANCE
runtime_validation_status: PARTIAL_USER_TESTED_WITH_KNOWN_LIMITATION
audience: implementation-agent
tags: [plan, minigames, split-screen, lifecycle, native-parity]
---

# Experimental split-screen suspension: implementation contract

## 0. Execution boundary and source authority

Implementation is present; user validation is partial with a known deferred race limitation (section 16). Section 15 records
the implementation and corrections to the original design. Sections 5 and 14 describe the investigated baseline,
not current runtime behavior. Historical PROPOSED identifiers below are superseded by actual source where noted.

The analysis and this artifact belong to checkout `E:\projetos\TooieRecomp-win`, branch `codex/tk64-launcher`,
HEAD `034189002f15d9b6dfeece588da71d93a9bca48e`. The sibling `E:\projetos\TooieRecomp` has different state;
do not transplant line numbers, run its older build, or modify it accidentally. Recheck HEAD/status before editing.

Pre-existing uncommitted investigation documents at the start of this plan:

```text
 M docs/HANDOFF.md
 M docs/VAULT.md
 M docs/reverse-engineering/README.md
?? docs/reverse-engineering/systems/minigames.md
```

Preserve those changes. No game implementation, generated output, private asset, ROM, save, controller assignment,
or executable was changed to produce this plan. No game process was started or controlled. Temporary static-analysis
scripts/cache are implementation aids, not deliverables; remove them after validating this document.

Read `AGENTS.md`, `docs/AGENTS.md`, `docs/HANDOFF.md`, and the relevant specs before implementation. The user
explicitly requested this temporary root Markdown; its location is an intentional exception to the normal placement
of future proposals under `docs/future/`. After implementation and acceptance, migrate stable contracts/RE knowledge
into canonical documentation, then remove this plan. Do not retain it as a contradictory second spec.

Truth labels:

| Label | Meaning in this artifact |
| --- | --- |
| `PROVEN / STATIC_SOURCE` | Literal source/metadata identity, condition, or call present in this baseline |
| `PROVEN / STATIC_MIPS` | Instruction/data relationship inspected in generated native-code representation or verified ROM |
| `STRONG_INFERENCE` | Native identities and control flow agree; complete event traversal not observed |
| `UNKNOWN` | Not resolved; explicit discriminator/validation required |
| `PROPOSED` | Required or recommended future implementation; not an implemented fact |
| `NOT_RUN` | Test/build/playtest not executed for this proposed change |

Do not infer runtime acceptance from static decoding. Do not treat names containing `game`, `shoot`, `fly`, `MP`,
or `SOP` as a complete event detector. Do not promote an unresolved alias into a proven transition.

## 1. Bounded objective

Suspend only the **experimental extra story player** during the 15 requested minigame families, covering their 23
replay stages. Use native map lifecycle boundaries rather than deleting/freezing an already running player.

Selected policy, `PROPOSED`:

1. Before creating players for a recognized challenge room, decline the host's `1 -> 2` story-player promotion.
2. Let native player, camera, controller, actor, UI, timer, score, reward and transition logic run normally.
3. Keep that policy throughout the room's setup, instructions, active game, retry, win/loss and result UI.
4. On the next eligible exploration map load, create the experimental story pair through the existing path.
5. Preserve the saved `two_players` preference. A suspension is not a user setting change.
6. Include Saucer's native return/result context in shared Witchyworld: destination `0xD6` with native route
   selector 6 must also decline injection. This exception is established statically in sections 4.3/12.4.

Consequences: exploration before/after the match **inside the same challenge room also uses native single-player**.
The scope is not limited to the interval when a countdown is moving. Restoring co-op immediately after a victory
while staying in that room is outside this implementation. An event-only policy would need individually verified
begin/end hooks and safe mid-map actor/camera lifecycle work; neither is established here.

### 1.1 Required invariants

| ID | Requirement / expected result |
| --- | --- |
| R01 | All 23 catalog stages decline experimental player injection in their challenge rooms |
| R02 | Outside native multiplayer, native single-player count/viewports remain native; normally J1 uses full screen |
| R03 | J2 cannot collide, collect, damage, receive AI targeting, open a host character selector, or influence distance retention while absent |
| R04 | Preparation, countdown, scoring, HUD, controls, native camera, retry and results remain functional |
| R05 | Returning to an eligible exploration map restores the existing pair creation/placement/release behavior |
| R06 | `dev_cheats.json.two_players`, controller profiles/assignments and General Multiplayer remain unchanged |
| R07 | Native multiplayer subtype `0x1B` remains native for **every** native player count, including 1 |
| R08 | Original creation requests other than exactly 1 remain unchanged |
| R09 | Dev Mode locked or option Off follows original behavior; no new guest writes from suspension logic |
| R10 | Ordinary exploration, lobbies, cutscenes, save selection, native split-up/auxiliary actors and other activities are not reclassified accidentally |
| R11 | No global focus mutation, reward/unlock writes, forced actor deletion or state `0x74` freeze as a suspension mechanism |
| R12 | Existing independent Object Distance, Analog Camera and J1 Dev Mode functionality are not disabled merely because the story pair is suspended |
| R13 | No new runtime metrics, per-frame logging, permanent actor census or heap allocation for room classification |
| R14 | No manual edits under `RecompiledFuncs/`; implementation belongs to host integration/source |

R12 means preserving those features' existing contracts, including native-control exclusions already implemented by
Analog Camera. It does not authorize enabling analog rotation in unsupported minigames or changing their controls.

## 2. Primary evidence and reproduction inputs

### 2.1 Source inventory

| Evidence | Location / interpretation |
| --- | --- |
| Exact catalog and stage count | `decomp/include/gameflags.h:1907`, 23 consecutive `GFLAG_*_MINIGAME_REPLAY_*` entries |
| Room identities | `decomp/include/maps.h` and generated host names in `src/game/generated/map_names.gen.cpp` |
| Read-only native multibit flag API | `decomp/src/core2/1EB3750.c:219` (`func_800DA564`), `:250` (setter; do not invoke for classification) |
| Reward identity cross-check | `decomp/include/jiggies.h` |
| Actor layout | `decomp/include/common.h:37` (`Actor`); `:114` (`ActorData`) |
| Native section address/size/functions | `metadata/tooie.us.syms.toml`; metadata section indices are not overlay IDs |
| Native overlay ID and exported entries | `metadata/overlays.table.toml`, mirrored naming in `decomp/overlays.us.toml` |
| Decompiled source ownership | `decomp/src/overlays/ch/`, `decomp/src/overlays/su/` |
| Native instruction representation | `RecompiledFuncs/funcs_*.c`, read-only investigation input |
| Current injection and runtime hooks | `src/game/tooie_dev_warp_hook.cpp` |
| Saved preference and old room predicate | `src/game/tooie_dev_cheats.cpp:159`, `:163`; `include/tooie_dev_cheats.h:90` |
| Host wrapper registration | `src/game/boot_hw_stubs.cpp:6172` and following registrations |
| Current canonical investigation | `docs/reverse-engineering/systems/minigames.md` |

Generated function-file line numbers are baseline locators, not stable public interfaces. If output is regenerated,
resolve by full symbol name, metadata section identity and original VRAM. Overlays share virtual base `0x80800000`;
the same offset in two overlays is **not** the same function. Resident syscall thunks and relocatable overlay
callbacks also have different addresses. Never patch a bare `0x8080xxxx` process address without resolving ownership.

### 2.2 ROM-derived scalar verification performed for this analysis

Read-only input: `build-win/extract/decompressed.us.z64`.

```text
size: 35098224 bytes
SHA-1: af479771ff5e91ce3732f3d84146614ab71d8d79
```

The reader asserted both before extracting descriptor scalars. It used section `rom` from
`metadata/tooie.us.syms.toml` and relocation offsets emitted in the corresponding entrypoint getters.
Only scalar identities, callback names and source locators are retained here; no ROM blocks/assets are copied.

Example: `chstoneballcontrol_entrypoint_0` returns section 1608 `+0x50D4`.
Section ROM base is `0x2039F40`; the descriptor's first four BE halfwords are `0239 01B7 0637 0001`;
its `+0x0C/+0x34/+0x40` pointers resolve to `func_8080079C_chstoneballcontrol`,
`func_80800F9C_chstoneballcontrol`, and `func_80800360_chstoneballcontrol` respectively.
The event appendix records equivalent relationships for every family.

Do not infer a native state name solely from an integer passed to `func_80101FDC`. Numeric transitions below
are proof of the call/argument only unless a meaning is explicitly established by its branch and services.
`Actor+0x72 >> 10` is the native six-bit actor state, not the player's behavior state and not a replay stage number.

### 2.3 Flags are catalog identities, not an active-game state machine

`PROVEN / STATIC_SOURCE`: 23 flag identities, `0xBE3..0xBF9` inclusive.
`STRONG_INFERENCE`: they encode replay availability; names and placement between boss/cinema global catalogs agree.
Exact persistent read/write lifetime is not exhaustively traced. No proposal below reads or writes them at runtime.

`func_800D395C` reads byte `0x8012B3F2` and returns `byte == 2` (`RecompiledFuncs/funcs_109.c:58`). Its use in
story/replay branches is visible across controllers. That does not make it an active-minigame boolean.
Likewise, `susubgame_entrypoint_2` merely reads `*(u8 *)(a1+0)`; existence of a generic helper does not establish
a global active-state bit shared by Kickball, Dodgems, FPS, rail ride, packing and quiz.

## 3. Exact stage matrix

Stage ID is a proposed host/test identifier, never a replacement guest flag number.

| ID | Family / stage | Catalog flag | Primary story room | Native multiplayer room / related variant |
| --- | --- | --- | --- | --- |
| MG01 | Mayan Kickball quarterfinal | `0xBE3` | `0xC6` | `0x17F` |
| MG02 | Mayan Kickball semifinal | `0xBE4` | `0xC8` | `0x17F` |
| MG03 | Mayan Kickball final | `0xBE5` | `0xC9` | `0x17F`; `0xCA..0xCE` unresolved aliases |
| MG04 | Ordnance Storage | `0xBE6` | `0xDC` | `0x164`, `0x165` |
| MG05 | Dodgems 1 versus 1 | `0xBE7` | `0xDE` | `0x186` |
| MG06 | Dodgems 1 versus 2 | `0xBE8` | `0xDF` | `0x186` |
| MG07 | Dodgems 1 versus 3 | `0xBE9` | `0xE0` | `0x186` |
| MG08 | Hoop Hurry | `0xBEA` | `0xE5` | `0x17C` |
| MG09 | Balloon Burst | `0xBEB` | `0xE4` | `0x17B` |
| MG10 | Saucer of Peril | `0xBEC` | `0x124` | SOP variants `0x13B`, `0x13C`; shared return `0xD6`/route 6 (4.3) |
| MG11 | Mini-Sub Challenge | `0xBED` | `0x181` | `0x182` |
| MG12 | Chompa's Belly | `0xBEE` | `0x123` | `0x183` |
| MG13 | Clinker's Cavern | `0xBEF` | `0x162` | Shared FPS machinery; no separate MP Clinker room established |
| MG14 | Twinkly Packing | `0xBF0` | `0x10A` | `0x17D` |
| MG15 | Colosseum Kickball quarterfinal | `0xBF1` | `0x12E` | `0x180` |
| MG16 | Colosseum Kickball semifinal | `0xBF2` | `0x12F` | `0x180` |
| MG17 | Colosseum Kickball final | `0xBF3` | `0x130` | `0x180` |
| MG18 | Pot O' Gold | `0xBF4` | `0x13D` | No separate MP room established |
| MG19 | Trash Can Germs | `0xBF5` | `0x137` | `0x185` |
| MG20 | Zubbas' Hive | `0xBF6` | `0x139` | `0x188` |
| MG21 | Tower of Tragedy round 1 | `0xBF7` | `0x15F` | `0x189` |
| MG22 | Tower of Tragedy round 2 | `0xBF8` | `0x15F` | `0x189` |
| MG23 | Tower of Tragedy round 3 | `0xBF9` | `0x15F` | `0x189` |

Named identities: `PROVEN / STATIC_SOURCE`. Family/room associations are `STRONG_INFERENCE` unless the event
appendix supplies a direct native map branch/retry. MP columns describe related native map names, not a claim that
all individual replay modes enter that map. The story policy must also work in replay without assuming its route.

## 4. Map registry and classification policy

### 4.1 Proposed immutable registry

Use explicit map constants/IDs. A compact `constexpr` table/switch is sufficient. Do not introduce file I/O,
runtime string matching, locale-sensitive labels, per-frame map-name searches, or settings for each minigame.

```cpp
// PROPOSED DATA, not an existing declaration.
constexpr uint16_t kStoryChallengeRooms[] = {
    0xC6, 0xC8, 0xC9,        // Mayan; 0xC7 is ordinary Glitter Gulch Mine!
    0xDC,                   // Ordnance arena, not entrance 0x163
    0xDE, 0xDF, 0xE0,        // Dodgems arenas, not lobby 0xDD
    0xE5, 0xE4,              // Hoop, Balloon; ordering differs from map numbers
    0x124, 0x13B, 0x13C,     // Saucer and named SOP variants
    0x181,                   // Mini-Sub cave, not general sea-bottom 0x1A9
    0x123, 0x162, 0x10A,     // Chompa, Clinker, Packing
    0x12E, 0x12F, 0x130,     // Colosseum arenas, not lobby 0x12D
    0x13D, 0x137, 0x139,     // Pot, Trashcan, Hive
    0x15F,                   // All three quiz rounds
};
constexpr uint16_t kConservativeKickballAliases[] = {
    0xCA, 0xCB, 0xCC, 0xCD, 0xCE,
};
constexpr uint16_t kRelatedNativeMultiplayerRooms[] = {
    0x164, 0x165, 0x17B, 0x17C, 0x17D, 0x17F, 0x180,
    0x182, 0x183, 0x185, 0x186, 0x188, 0x189,
};
```

Primary story set has **23 unique rooms**, coincidentally the same number as replay stages; these are different
dimensions. Tower's 3 stages share 1 room; Saucer contributes 3 named room identities. Do not use map count to
prove stage coverage. Count/uniqueness must be validated independently in code/tests.

Conservative aliases: propose declining host injection for `0xCA..0xCE` because the native map enum names them
Kickball and existing Analog Camera already treats them as special. This is a deliberate conservative policy,
not proof that the shipping story/replay reaches them. Keep their unresolved reachability explicit. Do not
manufacture additional stages for them or require private save modifications to claim a pass.

Related native MP room set provides a secondary no-injection guard if such a room is reached with subtype data
temporarily inconsistent. **Subtype `0x1B` always takes native precedence**, irrespective of map classification.
Do not force native multiplayer to one player or full screen. Preserve its original count/viewport topology.

### 4.2 Negative controls and scope exclusions

| Map / case | Required classification |
| --- | --- |
| `0xC7` Glitter Gulch Mine main area | Exploration; never include it via a `0xC6..0xC9` range |
| `0xBB` Mayan stadium lobby, `0xDD` Dodgem lobby, `0x12D` Colosseum lobby | Preserve current co-op eligibility |
| `0x163` Ordnance entrance, `0x187` Clinker sewer entrance | Preserve current co-op eligibility |
| `0x1A9` general sea bottom | Preserve current co-op eligibility; Mini-Sub exit references it |
| `0xD6` Witchyworld main area | Preserve ordinary exploration loads; exclude only the proven native Saucer return context (route selector 6), not the whole world |
| `0x136` Cloud Cuckooland, `0x108` GI Floor 3, `0x10B` GI Floor 4 | Preserve exploration outside challenge rooms |
| `0x142` Jinjo Village, `0xB8` Mayahem Temple | Ordinary exploration regression controls |
| Existing excluded scenes / `0x158` save selection | Preserve old exclusion behavior |
| `0x161`, `0x16F`, `0x170` race rooms, `0x151` Jiggywiggy temple | Outside the requested 23-stage policy; no silent expansion |
| Targitzan native MP room `0x166`, normal temple `0x178` | Outside this catalog; native-MP subtype guard still applies |

### 4.3 Shared-area exception: Saucer return/result context

`CORRECTED/SUPERSEDED`: the earlier room-only proposal deferred this exception until a live trace. Native table
and control-flow inspection now establishes that route selector 6 loads shared Witchyworld `0xD6`, initializes
native result/score/dialogue processing there, and is cleared later. Treating that load as ordinary exploration
would violate the selected requirement to suspend through results. Proof and exact tables: section 12.4.

Required pure decision input: the current three-bit native route value, obtained read-only via
`func_800DA564(0xA34, 3)` when evaluating an otherwise eligible `0xD6` creation. Apply the existing native-MP/count,
option and unlock guards as before; add only `map == 0xD6 && saucer_route == 6` as a no-injection context.

Do not add `0xD6` to the unconditional room registry. Do not classify unrelated maps from a nonzero Saucer field.
The native route value already survives the relevant transitions; a duplicate host itinerary token is unnecessary
for this proven return path. Do not write/reset the field, use replay flag `0xBEC`, or guess a timeout.

After the result callback clears 6 -> 0 inside the same `0xD6` visit, keep the already-created native single-player
topology until the next eligible native player recreation. No artificial reload or mid-map J2 spawn. This means
exploration after that result in the same Witchyworld visit stays single-player until a subsequent eligible load.
An ordinary new `0xD6` load with selector 0 remains co-op eligible. This consequence must be explicit in UI/docs and
manual acceptance; instantaneous restoration after dialogue would require a separate safe lifecycle design.

Full live ride traversal, all abort variants, route corruption/unused value semantics and any other shared-map
escape remain `UNKNOWN`. Extend only on primary evidence; do not claim that all live routes have been played.

## 5. Investigated baseline host/native behavior and failure mechanism

### 5.1 Current creation path (`PROVEN / STATIC_SOURCE`)

`tooie_dev_on_create_players`, `src/game/tooie_dev_warp_hook.cpp:358`:

1. Reset enemy/travel host state and cheat requests; clear delayed J2 placement state.
2. Call `tooie_dev_forms::begin_map` using request==1, native subtype !=`0x1B`, and old `two_players_map`.
3. Compute `two = two_players_active() && requested == 1 && two_players_map(current_map)`.
4. Latch `story_two_players = two` for model-lighting allocation scopes.
5. If `two`, add a viewport when count is 1 (`func_800A8FDC`), call `func_80016BDC(2)`, replace `a0` with 2.
6. Set `creating_story_players` around original `func_800F73C4`.
7. Place J2 beside J1 and schedule delayed reposition/release.

Defects relevant to this task:

- `two_players_map` currently rejects only selected cutscenes and save selection.
- `two_players_active` is **requested preference + session unlock**, not proof that a co-op pair was created.
- The `two` expression lacks the explicit subtype `0x1B` guard already used by the forms path. Native MP with a
  native request of 1 must be tested; `requested == 1` alone cannot identify story mode.
- Broadly changing `two_players_map` also changes the J1 transformation selector because it shares this predicate.

### 5.2 Native contracts to preserve

| Address / symbol | Contract used here |
| --- | --- |
| `func_800F73C4` / `0x800F73C4` | Map player creation; original requested count in `a0` |
| `gcstatusDll_entrypoint_11` | Native count: 1 unless subtype `0x1B`, then byte at `0x8012B404` |
| `0x8012762C` | Native game subtype byte; `0x1B` is native multiplayer context |
| `0x80127632` | Live high-level mode s16; existing runtime guards use story 3, sometimes attract 5 |
| `0x80132DC2` | Current map u16 |
| `0x80135490 + 4*i` | Native player pointer table; do not equate arbitrary non-null slot 1 with host-created J2 |
| `0x801354DC` / `func_800F54E4` | Native global focus/current player; do not mutate for this fix |
| `0x801276D2` | Viewport count s16; keep native value when declining injection |
| `func_800A8D70` | Native map layout initialization, including viewport setup |
| `func_800A8FDC` | Existing host path adds second viewport through native helper |
| `func_80016BDC` | Native controller channel-count setter path used by existing injection |
| `func_800F65D0` | Native active-player predicate; host currently broadens result for J2 |
| `func_800991B0` | Per-player update; existing host release/cheat/camera publication integration |
| `yaw_update` / `0x8009C88C` | Yaw update; host Warp completion can reposition J2 |
| `func_8009E8D4` | Full form/reset hook; current host resets menus, analog camera and saved form state |
| `_gcgoto_entrypoint_1` / resident thunk `0x80087A58` | Native map transition request; preserve its behavior |

Native count MIPS: `RecompiledFuncs/funcs_238.c:5535`, instructions `0x80800118..0x80800140`.
No stable runtime-gameplay-mode==3 requirement should be added at the creation boundary without checking native
load sequencing: load-time mode is transitional. Existing forms code explicitly reevaluates live eligibility later.

## 6. Proposed architecture

### 6.1 Separate three concepts

```text
requested_coop: user preference && Dev Mode session unlocked
map_decision: whether this creation request may be promoted by host
created_story_pair: this map actually completed host-promoted creation of two owners
```

Do not redefine `two_players_active()` silently to mean all three. Its current signature has no guest/map input,
and several tests deliberately replace it with a preference stand-in. Preserve its meaning or rename explicitly
with a complete call-site migration. Prefer adding distinct policy/session APIs with documented thread ownership.

### 6.2 Proposed pure policy API

```cpp
// PROPOSED; names/types may be adjusted, semantics are normative.
namespace tooie_story_coop {
enum class BlockReason : uint8_t {
    None, OptionOff, DevModeLocked, OriginalMapExclusion,
    NativeMultiplayer, NativePlayerCount, ChallengeRoom, SaucerReturn,
};
struct CreationInput {
    uint16_t map;
    uint8_t native_subtype;
    int32_t native_requested_count;
    bool option_enabled;
    bool dev_mode_unlocked;
    uint8_t saucer_route = 0; // Native three-bit value; relevant only to map 0xD6.
};
struct CreationDecision {
    bool inject_second_player;
    BlockReason reason;
    // Optional immutable family identity for tests; no runtime string allocation.
};
CreationDecision decide_creation(CreationInput) noexcept;
}
```

Deterministic decision ordering: native multiplayer and native count guards before injection; original map
exclusions, challenge classifier and the Saucer shared-map exception; option/unlock gates. Exact reported reason precedence can differ if tested,
but injected count must never differ. The function must be testable without ROM, UI, SDL or renderer.

New source/header ownership recommendation: `include/tooie_story_coop.h`, `src/game/tooie_story_coop.cpp`.
A compact pure policy header plus existing hook-owned session is also acceptable. Do not refactor the unrelated
Dev Mode settings/config system merely to introduce this predicate.

### 6.3 Runtime session and owner identity

`PROPOSED`: a game-thread-owned state distinguishes creation scope from a committed pair. Minimum information:

```cpp
struct StoryPairState {       // PROPOSED
    uint16_t map;
    bool creation_in_progress;
    bool pair_created;
    uint32_t first_owner;
    uint32_t second_owner;
};
```

- Clear committed ownership before each map's player creation and at full session reset.
- During eligible creation, mark a scoped creation intent before native model setup. Player models may allocate
  before player pointers are published; do not require committed `second_owner` to preserve private lighting.
- Commit identities only after the original creation completes and valid distinct promoted-owner pointers are
  published. Track construction provenance as required by 6.6; two arbitrary non-null slots are insufficient.
- Guard runtime co-op extensions by the created-pair state, current map, non-native-MP subtype, requested option
  semantics, and pointer identity/liveness appropriate to the consumer.
- Return false while a recognized challenge room is loading/running, even if an old pointer remains readable.
- Map equality alone is insufficient for a same-map retry. Reset ownership on every creation/reset invocation.
- Do not mutate this state from render/input threads without an explicit publication model. If UI needs a status,
  expose an atomic/value snapshot rather than reading mutable guest pointers across threads.
- Preserve existing behavior when the user toggles the option Off mid-map: no new destruction of J2/viewports;
  runtime extensions retain their existing preference gates. Pair existence and current option are separate facts.
- Likewise, toggling On in a native single-player map must not fabricate a committed pair or begin mid-map spawning.

Avoid a new general session manager, actor registry or per-frame census. A map-scoped state and existing native
tables suffice for this objective. A generation ID is justified only for actual queued work crossing reset, not for
metrics; existing resets and owner comparison should be evaluated first.

### 6.4 Creation pseudocode and mutation budget

```cpp
// PROPOSED algorithm at the existing host wrapper.
const int32_t requested = int32_t(ctx->r4);
const auto decision = decide_creation(read_creation_input(rdram, requested));
reset_old_pair_and_delayed_work();
reset_enemy_and_travel_state();
reset_old_owner_requests_and_publications();
begin_forms_map_using_explicit_forms_policy(); // do not reuse new co-op predicate accidentally
{
    CreationScope scope(decision.inject_second_player); // includes existing private-vertex intent
    if (decision.inject_second_player) {
        add_native_second_viewport_if_needed();
        set_native_controller_channels_to_two();
        ctx->r4 = 2;
    }
    func_800F73C4(rdram, ctx);                  // exactly once, including every no-injection branch
}
if (decision.inject_second_player) {
    // Pointer validity alone is insufficient: native creation can bypass the promoted loop.
    if (commit_verified_promoted_owners()) {
        place_and_schedule_second_player_via_existing_path();
    }
}
```

These are explanatory calls, not existing function names. Preserve actual ABI, guest stack allocation, register
handling and original result semantics. In no-injection branches, do not call the host-added viewport/channel
helpers, do not overwrite `a0`, and do not repair viewports manually. Native map initialization owns viewport
restoration; controller polling count has a different lifetime (section 6.10).

If clean reproduction proves that an ordinary single-player challenge retains two viewports after declining
injection, isolate the ordering/call contract first. Two polled controller ports alone are not stale player/view
topology. Do not unconditionally write `0x801276D2 = 1` or call `osContSetCh(1)` as a guess. Native multiplayer
and some specialized presentation paths have their own topology.

### 6.5 Forms and queued-input isolation

Existing `SavedForm` contains values: `{valid, form, optional dragon, TransitionCache[17]}`. Native handoff words
span `0x80127040..0x80127083`; they are not `PlayerState*`/model ownership. `begin_map` resets menus, while the full
reset hook clears saved values. Preserve normal per-player handoff semantics across map loads.

Required adjustments:

1. Keep the old scene/save-select predicate separate from the new story-pair predicate. Do not disable the existing
   single-player J1 selector through a broad rename/change to `two_players_map`.
2. Ensure J2 menu availability, allocation/setup/teardown handoff scopes and requests refer only to the genuine
   host-created second owner; a native auxiliary/native multiplayer slot must not inherit J2 saved form state.
3. During suspension, retain only legitimate saved value state allowed by existing lifecycle. Do not serialize or
   restore guest pointers. Do not preserve a temporary behavior/timer past a native full reset by bypassing it.
4. Reset pending J2 form/cheat input at the boundary; held buttons must not transform a newly allocated owner
   on return. Implement host-shortcut rearming explicitly (section 6.7). The existing menu drain is not a universal
   solution: it consumes native input even while the menu is unavailable.
5. Resume through normal map creation. Do not promise that an illegal form persists into a challenge; native
   legality/reversion routines remain authoritative.

The existing forms helper derives an index from `PlayerState+0x184`. The camera contract documents that field
as native camera index, not a universal player slot. Do not extend that assumption into the new ownership API.
Use table/committed owner identity; any broader forms indexing correction requires its own demonstrated regression.

### 6.6 Native construction, reentrant activity and retirement (review correction)

`PROVEN / STATIC_MIPS`: the native requested count is not an unconditional allocation guarantee. The following
relationships materially constrain P2/P3; do not replace them with a harness that merely fills two table slots.

| Native anchor | Observed contract | Implementation consequence |
| --- | --- | --- |
| `func_800A72A4`, original `0x800A7348..0x800A7364` | Calls `func_800A8D70` before `func_800EA0CC(map, entrance, 0)` | Ordinary load path initializes layout before world/player creation; preserve this order |
| `func_800EA0CC`, original `0x800EA0FC..0x800EA110` | Passes `a0=0x80132DC0`, map/entrance in `a1/a2` to `_gsworldDll_entrypoint_2` | Resolve the real destination, not an earlier source-map snapshot |
| `gsworldDll_entrypoint_2`, original `0x808002B8..0x808002E4` | Stores destination map at world state `+2`; later, `0x80800478..0x80800484`, gets native count and calls `func_800F73C4` | `0x80132DC2` is already the destination at this creation boundary; world-state `+0` is not the separate live-mode address `0x80127632` |
| `func_800F73C4`, original `0x800F73FC..0x800F7410` | Clears eight player-pointer slots and associated bytes | Do not read pre-call slot pointers as new owners; this routine is initialization, not an old-owner destructor |
| Same function, `0x800F742C..0x800F7458` | Skips the requested-count loop if any of `func_800DB9B0`, `_glcutDll_entrypoint_1` (`0x800887B0`), or `func_800F99E8` returns nonzero | Injection intent cannot by itself prove the promoted players were constructed |
| Same function, `0x800F7464..0x800F7474` | Requested-count loop calls `func_800F61E4` | Distinguish normal promoted allocation from descriptor/auxiliary allocation |
| `func_800F61E4`, `0x800F61EC..0x800F61F0` | Calls `func_800F6140(-1)` | Native allocation chooses a free controller; do not assume an arbitrary allocator call is this path |
| `func_800F6140`, `0x800F6170..0x800F61B4` | Chooses camera/controller, calls `func_800F43A8(slot)`, publishes returned pointer, then binds its input | Allocation/model initialization precedes table publication; owner readiness has multiple phases |
| `func_800F8B94`, `0x800F8BF8..0x800F8D24` | Iterates native records (stride `0x3C`) selected by entrance and creates through `func_800F6204(0)`; assigns native roles/state | Distinct, non-null slots after creation may include native actors, even when the requested-count loop was skipped |
| `func_800F6204`, `0x800F620C` | Calls `func_800F6140` with the incoming controller argument | Preserve descriptor-owned players and their controller topology |
| `func_800F73C4`, `0x800F74B8..0x800F74D0` | May create a fallback through `func_800F61E4` and assign native role 4 | One such allocation is not proof of a promoted pair |
| Same function, `0x800F7510` and `0x800F7528` | Calls hooked `func_800F65D0` before the outer creation wrapper returns | A committed-after-return-only activity guard changes current initialization behavior |
| `func_800F759C`, `0x800F75E8..0x800F7608` normal branch | Calls `func_800F3880(owner)`, releases camera association, then clears slot | Owner retirement is not limited to full map creation/reset |
| `func_800F3880`, `0x800F3888..0x800F3894` | Calls hooked `func_80098E64`, then frees the PlayerState allocation | Existing forms teardown hook is before the outer PlayerState free; preserve serialization before invalidating its value handoff |

Baseline locators: `RecompiledFuncs/funcs_292.c:2787`, `funcs_252.c:3753`, `funcs_52.c:1950`,
`funcs_153.c:3305`, `funcs_113.c:9673`, `funcs_134.c:2943`, `funcs_188.c:4589`, `funcs_160.c:444`,
`funcs_246.c:1461`, `funcs_303.c:204`. These are read-only source anchors.

Required design refinements (`PROPOSED`):

1. Separate **intent**, **provisional constructed owners**, and **committed pair**. The minimum state in 6.3 is
   conceptual; add bounded construction provenance if necessary. A successful decision plus two readable pointers
   does not satisfy ownership. Never adopt descriptor/fallback actors as experimental J2.
2. Characterize creation with the real `func_800F73C4` and real allocation dispatch, with world/asset services
   replaced only at documented leaves. Cover each skipped-loop predicate independently, descriptor creation,
   fallback creation and the ordinary promoted loop. Do not duplicate guest predicates into the pure room policy
   or pre-call side-effectful native getters merely to predict allocation.
3. A narrow option is a scoped host wrapper around resident `func_800F61E4`: in this baseline its only direct JAL
   sites are the normal loop and fallback above; `func_800F6204` identifies the different descriptor route.
   Record returned slots/owners during an injection attempt, validate publication, and require evidence of the
   two normal allocations before commit. This is an implementation candidate, not permission to infer every
   future indirect caller or change native allocator behavior. Recheck callsites/metadata before registering it.
4. Keep private-vertex intent active before publication. For the reentrant `func_800F65D0` calls, preserve the
   previously working J2 initialization behavior only for a verified provisional promoted owner already published
   in the current construction. Do not expose provisional owners to input/render threads or AI as a committed
   running pair. Verify the native focus/role outputs with this guard; do not mutate focus to compensate.
5. Only a committed verified pair schedules placement/release. If original creation produces only native/fallback
   owners, publish pair absent, cancel delayed work and leave native outputs intact. Do not retry allocation,
   delete actors, clear guest state or force rollback of camera/controller topology inside this fix. An actual
   promotion/topology failure on an eligible ordinary map is a blocker to diagnose, not a synthetic success.
6. On teardown of a recorded owner, capture its role before calling native teardown; allow the existing value
   serialization scope to finish. Retire its host ownership/publication and cancel its pending work before the
   caller frees/reuses PlayerState memory. A reused address in the same map must not inherit participation.
   Pair gameplay availability becomes false when either owner retires, but retain the other still-live owner's
   scoped serialization identity until its own teardown. Clearing both identities on the first destructor must
   not discard the second outgoing player's form bank. Never retain a retired owner's guest pointer for later use.
   Section 6.9 specifies the native activity check inside serialization; retaining a saved role alone is insufficient.
   New owners join only through verified creation; no per-frame pointer adoption. Audit the special branch of
   `func_800F759C` separately; do not force its pending-removal path to become immediate destruction.
7. Do not place an unconditional pair reset at native goto request time: a request can precede outgoing teardown
   and J2 form serialization. Revoke gameplay eligibility and preserve narrowly scoped outgoing ownership/value
   handoff as needed. `begin_map`, owner retirement, and `tooie_forms_on_reset` are distinct boundaries.

This closes an ambiguity in the original plan: committed identity is necessary for runtime consumers, but it
cannot be the sole precondition for every callback executed inside construction or native teardown.

### 6.7 Host input rearming and analog reservation (review correction)

`PROVEN / STATIC_SOURCE`:

- `Menus::reset`, `src/game/tooie_dev_forms.cpp:7`, assigns `players = {}`: previous button bits become zero,
  `drain` becomes false, and pending/open state disappears. The next held direction can appear newly pressed.
- `Menus::input`, `:41`, updates `previous`, then checks `drain` **before** its unavailable return. A draining
  unavailable menu returns true; `src/main/main.cpp:1064` then zeros native buttons/stick. Reusing this drain to
  suppress all absent-J2 input can swallow native multiplayer input.
- `tooie_dev_cheats::reset_requests`, `src/game/tooie_dev_cheats.cpp:155`, clears atomic request bits only;
  `player_chords` history is retained. A reset is not a complete edge/rearm policy. Chord histories also must not be
  mutated from the game thread while the input thread calls `step` without synchronization.
- Analog eligibility is an atomic per-controller publication. `refresh_player` republishes only the current
  player's controller; absent J2 gets no update. `reset_motion` clears motions, scoped deltas, all eligibility bits
  and samples, but is currently called by the forms full-reset hook, not `begin_map`/map creation.
- `special_map` omits `0x181`, `0x13B`, `0x13C`. Its other camera/owner/mode guards are independent and remain
  necessary; a slot/pair guard must not disable normal single-player Analog Camera.

Required behavior (`PROPOSED`):

1. Publish host-shortcut availability per actual owner/port using the existing synchronized value-publication
   model. When J2 is absent, discard its pending host form/cheat requests and do not enqueue replacements. Keep
   native port reads and native button/stick values intact, including native MP with 1/2/4 participants.
2. On owner change or suspension/return, disarm host J2 shortcuts until an input sample observes neutral/released
   shortcut controls, then require a fresh chord. This must work even if the absent port was not polled during
   the challenge. Implement rearming on the input side or with synchronized publication; do not race-reset
   `player_chords`. Define neutral for Debug/R3, the shortcut direction and menu confirmation explicitly.
3. Separate **host shortcut suppression** from **native input consumption**. Keep menu drain for its existing
   active-menu confirmation purpose; an unavailable owner must not consume original native input through drain.
   Include pending `open_requested` from the ESC button in cancellation, not just confirmed transformations.
4. Reset analog motion/reservation at every player recreation (including same-map retry), before new owners are
   published. Reuse `tooie_analog_camera::reset_motion()` on its owning game thread; preserve settings. Independently
   republish eligible surviving/new J1 and later genuine J2 through existing native eligibility checks.
5. Close the three known dedicated-room omissions for native-control parity, either through a shared pure room
   classifier plus the existing camera-only exclusions, or explicit additions. Do not replace the camera's wider
   exclusion list with the narrower 23-stage catalog. Do not mark all `0xD6`/`0x1A9` special.
6. Test a held right stick through exploration -> challenge. On the first input poll after the boundary, C bindings
   must be available in the special room; Z+C remains native. Also test return to exploration: no old angular
   delta/smoothing follows a new owner. Controller assignment changes use port identity, not slot==port guesses.
7. Include the Saucer route-6 return/result context in this native-control check. `special_map(0xD6)` alone is false;
   inspect actual native camera eligibility during result/score/dialogue before assuming other filters suffice.
   If a context exclusion is needed, scope it to the native route-6 context, not all Witchyworld and not the
   existence/absence of J2. After native result completion, J1 camera eligibility remains independently governed
   by the live camera contract even if co-op restoration is waiting for a later map recreation.

### 6.8 Form handoff precedence during suspension

`PROVEN / STATIC_SOURCE`: `TransitionForm` temporarily installs an entire saved bank and saved form byte during
allocation/setup; `PlayerScope` also affects Dragon reads. Keeping J1's selector available is a separate decision
from replaying old host handoff values during challenge initialization.

`PROPOSED`: tests must seed distinct J1/J2 banks, capture outgoing J2, run a native challenge entry that changes
the transition bank, initialize the single native owner, then return to exploration. Assert:

- Native Stony/Submarine/Bee/Blaster or other event-required entry state is not replaced by an old saved host form.
- J2 values never affect J1 during the single-owner visit. No native actor at camera index 1 overwrites saved J2.
- J1's latest legitimate native teardown/reversion wins on exit; a held host request cannot replace it.
- Optional absent-J2 value continuity is preserved only across the existing legal handoff lifecycle; full native
  reset still clears it. Do not fabricate an independent persistent inventory/save/form subsystem.

If the baseline value cache overrides a demonstrated native challenge entry, narrowly separate J1 menu
availability from transition-cache application at this boundary. The static existence of `TransitionForm` proves
the override mechanism, not a live minigame failure; that failure remains `UNKNOWN` pending the discriminator.

### 6.9 Teardown activity is a separate scoped capability

`PROVEN / STATIC_MIPS`: `func_800F482C(0)` traverses occupied player slots in ascending order. For each, it calls
`func_800F3880`, releases camera association, and clears the slot before continuing. Thus J1 can already be freed
and slot 0 null when J2 enters native teardown. See `RecompiledFuncs/funcs_27.c:4`, original
`0x800F4850..0x800F489C`, and `funcs_303.c:204`.

Inside `func_80098E64`, `0x80098EBC..0x80098EC8` calls `func_8008DAA8(self)` and skips the transition-serialization
block if it returns false. `func_8008DAA8`, original `0x8008DAB0..0x8008DAB4`, calls `func_800F65D0` with
`PlayerState+0x184`. Therefore a new runtime guard requiring both current players/pair active can prevent J2
serialization precisely after J1 has retired. Merely preserving `SavedForm` or the outgoing role does not run the
skipped native serialization. Anchors: `RecompiledFuncs/funcs_309.c:312`, `funcs_4.c:3373`.

Required integration (`PROPOSED`):

- Keep gameplay pair availability false after an owner retires. AI, collision expansion, distance retention,
  delayed placement and host shortcuts must not reactivate merely to preserve handoff.
- Inside the existing `tooie_forms_on_teardown` wrapper, establish a narrowly scoped outgoing-owner serialization
  capability before calling original `func_80098E64`. It refers only to a previously verified promoted J2 still
  alive at this call, with the same current slot/owner and native activity argument. Preserve pre-existing option,
  unlock and native-MP behavior. No auxiliary/native MP owner receives this capability.
- The activity wrapper calls native `func_800F65D0` once, then may preserve the old host J2 activity result for
  that exact synchronous serialization scope even if the other owner has retired. Do not require live J1 for this
  exception, do not mutate global focus, and do not skip native Goliath reversion or other teardown work.
- Snapshot native serialized values after the original, restore J1's global bank as before, retire the outgoing
  identity, then let the original caller free PlayerState. A scoped owner pointer must never survive that free.
- Do not identify the scope by `arg == 1` alone: resolve its outgoing owner and capture the native argument
  actually read from `+0x184`; table slot, camera index and controller port remain separate identities.

Required regression: execute real ascending `func_800F482C(0)` and real `func_80098E64` with distinct J1/J2 forms
and handoff data; stub only documented world/model/subsystem leaves. Force the first owner absent before the
second teardown and verify J2's serialized bank is still captured without re-enabling any gameplay consumer.
Repeat with Goliath's native reversion, native auxiliary slot, option Off, native MP, and single-owner challenge.
The existing forms harness substitutes teardown; it cannot by itself prove this native activity boundary.

### 6.10 Viewport topology and controller polling have different lifetimes

`PROVEN / STATIC_MIPS`: `func_800A8D70`, original `0x800A8D84..0x800A8DC8`, calls native status/map-section
services and tests section flag `0x20`. If false, it writes viewport count 1 at `0x801276D2`; if true, it uses
`gcstatusDll_entrypoint_11`. It then initializes that many viewports via `func_800A877C` and clears camera
association/state tables. Its body contains no controller-channel setter. Anchor: `RecompiledFuncs/funcs_269.c:45..263`.
Do not simplify this branch to a map-ID range or make the host write count 1 for every suppressed context.

`PROVEN / STATIC_MIPS`: `func_80016BDC` calls resident `osContSetCh` at `0x80030550` with the low byte of `a0`
(`RecompiledFuncs/funcs_282.c:401..428`). The four direct native JAL sites in this generated baseline are all in
`chgameselect`, not in per-map player creation:

| Native function / original callsite | Requested channel count | Generated source anchor |
| --- | --- | --- |
| `func_8080000C_chgameselect` / `0x80800024` | 4 | `RecompiledFuncs/funcs_103.c:3157` |
| `func_80803888_chgameselect` / `0x808038AC` | 2 if incoming `a2 != 0`, otherwise 1 | `RecompiledFuncs/funcs_197.c:1617` |
| `func_808038EC_chgameselect` / `0x80803900` | 1 | `RecompiledFuncs/funcs_243.c:403` |
| `func_80803B3C_chgameselect` / `0x80803B4C` | Result of `gcstatusDll_entrypoint_10` | `RecompiledFuncs/funcs_286.c:950` |

This is a direct-call census, not proof that no indirect caller can ever change channels.

`PROVEN / STATIC_SOURCE`: ultramodern stores the last count in `max_controllers`; `osContSetCh` updates it and
`osContGetReadData` visits only that many ports. `osContInit` resets it to 4. No map-lifecycle reset exists in
this runtime source. The librecomp adapter leaves ports outside the read count untouched in guest memory;
therefore reducing the count is not an input-buffer flush or proof that old J2 buttons disappeared.
Sources: `lib/N64ModernRuntime/ultramodern/src/input.cpp:86..94`, `:118..122`, `:153..173`;
`lib/N64ModernRuntime/librecomp/src/cont.cpp:45..67`. Existing fidelity contract:
`docs/specs/Tooie/input-and-controllers.md`, section 3, Guest Read Contract & Absence Semantics.

Required integration/acceptance (`PROPOSED`):

- Declining injection must not lower the channel count, disconnect a physical controller, clear guest pad buffers,
  or change host assignments. It may leave two ports polled after an experimental pair, while only native J1 exists.
- Polling a port is not owner admission. Native actor/control logic remains native; host J2 shortcuts require
  verified availability/rearming even if input callbacks continue arriving for port 2 during the challenge.
- Model viewport reset and polling persistence independently in the harness. Do not make a fake native map load
  reset both to 1: that would conceal the production contract. Also do not assert exactly one polled port as a
  condition of successful suspension.
- Exercise real layout initialization with section flag `0x20` clear/set and native counts 1/2/4; preserve original
  outputs and call order before the creation wrapper. Return to exploration must request two channels through
  the existing promotion path, even if they were already two. Native multiplayer keeps its own channel setters.
- Hold movement/attack/debug controls on port 2 while the challenge's sole native player runs. Verify no host
  J2 action, scoring or camera contribution. If an event's original logic directly consumes that port, isolate
  that event/caller before proposing additional filtering; broad channel reduction is not the default fix.

No runtime game failure or need to change the vendored input runtime is established by these static facts.

## 7. File-by-file change map

All entries below are `PROPOSED`, not current modifications.

| File / symbol | Required adjustment | Preserve |
| --- | --- | --- |
| New pure policy module | Immutable room registry, native MP/count guards, decision unit surface | No guest access, settings I/O or string allocation |
| Creation input adapter in `tooie_dev_warp_hook.cpp` | Read native Saucer route for eligible 0xD6 loads with an isolated context; pass its value to pure policy | Original registers/request, no flag writes, no overlay actor pointer dependency |
| `tooie_dev_cheats.cpp::two_players_active` | Keep current requested-option meaning explicit | Settings mutex/atomics and persisted JSON behavior |
| `two_players_map` callers | Separate old general story/form eligibility from co-op injection eligibility | J1 selector and old scene exclusions |
| `tooie_dev_warp_hook.cpp::tooie_dev_on_create_players` | Evaluate decision before promotion; native call once; creation scope and committed pair ownership | Existing placement delay and private lighting |
| `tooie_dev_on_player_update` | J2 delayed release only for created second owner; cancel stale schedule at boundary | Native `func_800991B0` call and analog publication pre/post update |
| `tooie_dev_on_player_active` | Separate running/provisional creation/teardown capabilities; broaden only the verified owner in its allowed scope | Native multiplayer activity semantics and J2 native handoff after J1 retires |
| `tooie_dev_on_yaw_update` | Only reposition genuine current J2 during host Warp | J1 teleport and native yaw update |
| `set_player_model` / `creating_story_players` | Preserve pre-publication private-vertex creation scope; distinguish committed pair | Native model allocator/teardown ownership |
| `tooie_dev_forms_hook.cpp` | Explicit J1/J2 policy; no stale J2 publication/handoff into native auxiliary slots | Native form legality, Dragon auxiliary refresh, full reset semantics |
| `tooie_dev_forms.cpp::Menus` / `tooie_dev_cheats.cpp` input admission | Separate shortcut rearming from native input consumption; synchronized edge ownership | Active-menu confirmation drain, native MP input, J1 shortcuts |
| `func_800F61E4` / existing allocation wrappers | If needed, record scoped normal-allocation provenance without changing native allocation | Descriptor/fallback actors and callback ordering; see 6.6 |
| `tooie_dev_enemy_ai.cpp::enabled` | Require effective story pair before two-owner targeting/cache/arbitration | Existing native mode, range, family and owner guards |
| `tooie_dev_collectibles_hook.cpp::enabled` | Require effective pair before cooperative Glowbo cleanup behavior | Native credit/deletion order, shared inventory |
| `tooie_dev_travel_menu.cpp::enabled` | Require effective pair before retaining a J1 menu across two viewport queues | Native Warp Pad/Silo callbacks and J1-only interaction |
| `tooie_object_distance.cpp::nearer_second` | Consider only genuine live story J2 | Entire normal Object Distance feature and fade math |
| `tooie_dev_cheats_hook.cpp` | Prevent old J2 queued request/reload from hitting a new native owner after reset | Existing J1 cheat behavior; no unrelated log cleanup in this task |
| `src/main/main.cpp` input routing | Audit J2 host shortcut admission/publication at suspended-map boundary | General Multiplayer settings, native port reads, per-player assignments |
| `tooie_analog_camera*.cpp` | Reset motion/reservations at each recreation; close 0x181/0x13B/0x13C omissions with independent camera guards | Proportional camera, logical Z+C, native special controls, independent ownership |
| `tooie_dev_mode_ui.cpp` | Explain automatic suspension in challenge rooms in the existing option description | Saved toggle remains On; no extra per-event setting or forced UI toggle |
| `boot_hw_stubs.cpp` and hook headers | Register only genuinely needed new lifecycle integration | Existing registrations/original paths; no overlay-native gameplay rewrite |
| `CMakeLists.txt`, platform test runners | Add policy/lifecycle regression target/dependencies as needed | Tests outside production game executable |
| Existing subsystem fixtures | Replace preference-only stubs with meaningful effective-session controls where exercised | Disabled/native MP cases and existing targeted regressions |
| Documentation | Update stable contract after implementation; RE only for verified discoveries | `UNKNOWN` and `NOT_RUN` labels until resolved |

Changing the signatures of `enabled`, `nearer_second`, or wrapper names requires `graft callers`/exhaustive literal
audit before edits. Several test executables supply their own `namespace tooie_dev_cheats { bool two_players_active(); }`
definitions. A new dependency must be linked or stubbed intentionally in each affected harness; do not leave tests
testing an obsolete preference-only approximation while production uses different logic.

Input anchors: `src/main/main.cpp:1019`, `:1056`, `:1062`, `:1072` publish analog samples, process the host
character selector and enqueue cheats. Suspend only inappropriate host J2 shortcuts/publications through the
game-thread-to-host value publication contract. Do not disconnect N64 port 2, modify controller profiles or merge
J2 input into J1 as a substitute for removing experimental participation. Native multiplayer must still receive
its assigned ports. The input callback must not inspect mutable guest player pointers from another thread.

## 8. Implementation sequence and stop conditions

| Step | Deliverable | Acceptance / stop condition |
| --- | --- | --- |
| P0 | Recheck baseline/status/instructions; confirm room/controller evidence below | Preserve existing dirty docs; no generated edits |
| P1 | Immutable catalog + pure creation policy + unit matrix | All primary rooms/aliases tested; exact negatives; MP1 and non-1 requests untouched |
| P2 | Existing map creation wrapper uses policy; native construction characterization; provisional and committed ownership | Native original runs once; no-injection has zero host promotion helper calls; descriptor/fallback actors not adopted |
| P3 | Runtime consumers and reset integrate effective pair identity | No auxiliary/native MP adoption; no stale J2 action after retry/return |
| P4 | Integration regressions for normal -> challenge -> retry -> exploration | Counts, owner identity, native helpers, pointer publication and release timing covered |
| P5 | Required Windows test stage and game build | Both exit 0; executable actually rebuilt; report paths/hash |
| P6 | Manual 23-stage/event-flow validation | User result per matrix, explicit blocked/unreachable cells |
| P7 | Review/diagnostic cleanup/documentation | No temporary probes in game; no tests/asset dumps shipped; checks clean |
| P8 | After acceptance, migrate stable docs/remove root plan, then final user-requested commit | Explicit intended paths, staged diff review; no push without request |

If another game event begins in an exploration map without a native player-recreation boundary, P2 cannot suspend it
by changing an already completed creation decision. Stop the all-event claim and identify the actual transition.
Saucer's proven return load already requires the 0xD6/route-6 exception in section 4.3. Do not force-delete J2,
change native focus, reload the map artificially, skip the original controller, or modify global save flags.

## 9. Validation design

### 9.1 Pure policy: required cases

- Iterate all 23 stage records independently, even when they share a room.
- Check unique primary rooms separately from stage count; check each conservative alias and each related MP room.
- For every room: option Off, Dev Mode locked, native count 0/1/2/3/4, native subtype `0x1B` and ordinary subtype.
- Assert identity/passthrough for native MP count 1; this is not covered by testing only MP counts 2/4.
- Test each negative control in section 4.2. Include `0xC7` explicitly to detect an accidental inclusive range.
- Verify flag identities contiguous/unique in metadata-only tooling; runtime classification must not depend on them.
- Unknown map IDs retain the original eligibility rules; do not default all unknown maps to minigame.
- Loading mode and gameplay mode are separate inputs/contexts; do not require a stable live mode too early.
- Test 0xD6 with each route value 0..7: only the proven route-6 exception adds suspension; keep unused value
  meanings UNKNOWN. On every unrelated eligible map, changing this field alone must not change the decision.
  Combine route 6 with native MP/count, option Off and locked Dev Mode passthrough cases independently.

### 9.2 Integration harness: exercise production wrapper and state

New target recommendation: `tooie_story_coop_test`, `EXCLUDE_FROM_ALL`. Test production decision and wrapper, not a
copy of their conditions. Native assets/world construction can be replaced by narrowly documented stand-ins.

Capture at minimum:

```text
original_create_calls, original_a0
native_viewport_add_calls, channel_helper_calls and arguments
created owner pointers / pair publication timing
J2 placement calls, scheduled release pending/cleared
original_active result vs host-broadened result
native model allocation during creation before player-table publication
menu/request/sample/cache cleanup at each recreation/reset
```

Those counters belong to the harness, not production instrumentation.

Mandatory sequences:

1. Exploration (native 1 -> host 2) -> challenge (native 1) -> same-map retry (native 1) -> exploration (host 2).
2. Exploration -> challenge -> native replay/multiplayer with count 1/2/4 -> exploration.
3. Native auxiliary slot 1 exists, option toggled On: no co-op adoption, no artificial collision/targeting.
4. Host pair created, then setting Off: retain existing lifecycle behavior; no new mid-map destruction.
5. Challenge resident, setting Off/On/unlock: remain native until an eligible map creation, no delayed spawn.
6. Delayed J2 release pending when map changes: no placement/release into new map or reused pointers.
7. Held J2 shortcut through suspension/return: no stale confirmation/cheat; J1 normal input continues.
8. Native `0x1B`, requested 1: original `a0`/viewports/controllers preserved, no private story-player scope.
9. Model construction before owner table publication still gets existing private lighting when injection is valid.
10. Native full reset clears co-op state and input/cache publication; same-map reload is a fresh lifecycle.
11. Each native requested-loop skip predicate is true independently; native descriptor actors/fallback may still
    exist. No false pair commit, host release or J2 shortcut. Original controllers/state construction still run.
12. Real original creation calls `func_800F65D0` before outer return: provisional ownership permits only the
    verified promoted owner and preserves native initialization outputs. Descriptor/retired owners stay native.
13. Destroy a committed owner through native teardown, then reuse its address in the same map: its saved value
    handoff is captured before free, ownership is retired, and no old pending action adopts the replacement.
14. Seed a saved host form, then let challenge entry change the native transition bank before allocation/setup:
    native event-required form wins; absent J2 bank stays isolated; full reset clears both as before.
15. Hold Debug+direction/A across a period with no port-2 polling, then restore the pair: no automatic menu,
    confirmation or cheat until release/neutral and a fresh chord. Native MP buttons/stick remain unchanged even
    if an old host menu was draining or had `open_requested` pending.
16. Analog reservation previously true for both controllers; enter challenge and poll input before any new player
    update: reservation/samples are reset, C input remains available, and no old smoothing is applied on return.
17. Saucer: 0x124/route 5 -> 0xD6/route 6 -> native result callbacks -> route 0 without recreation -> eligible
    map load. No pair is created for the result load; clearing the route does not spawn mid-map J2. A later
    ordinary 0xD6/route-0 recreation can promote normally. Read the flag before native actor initialization,
    preserve caller registers and prove no setter/host route token is needed for this path.
18. Ascending native destruction retires J1 first, then serializes J2 while gameplay pair state is false:
    native transition bank still captures J2 in the restricted teardown scope; all gameplay consumers remain
    disabled. Cover native reversion and non-host owners; see 6.9.
19. Real native layout setup precedes creation and restores its native viewport count independently of persistent
    controller polling. Start with two polled ports, enter/retry a single-owner challenge, keep both port callbacks
    active, then return. No host channel-reduction call or guest pad-buffer clearing; absent-J2 shortcuts stay
    disarmed. Separately cover native section flag `0x20` and native multiplayer counts 1/2/4 (section 6.10).
20. Mini-Sub result choice: distinguish story `goto(0x181,1)` / `goto(0x1A9,0x1B)` from replay frontend dispatch.
    In `func_80800BA8_chminisubgame`, event/choice tag `0xCEB` branches on `func_800D395C`: nonzero calls
    `_gcfrontend_entrypoint_11` for callback argument `a2 == 1`, otherwise `_gcfrontend_entrypoint_10`, and skips
    both local map-goto branches. Execute this native branch with frontend/goto leaves recorded; do not require a
    story destination or synthesize host pair restoration on a replay callback. Later creation owns that decision.

Cases 11/12 must execute real native creation control flow, not only the wrapper's stand-in. If a new allocator
wrapper is used, compile its production implementation into the harness and exercise dispatch/registration too.
Cases 13/14/18 must include existing forms hooks; independent fake `saved[]` implementations cannot prove isolation.
Case 18 additionally requires real native teardown/activity control flow, not the current teardown stand-in.
Case 19 must execute real layout control flow; fake channel reset on map entry is not a valid substitute.

### 9.3 Existing regressions to retain

`tooie_dev_cheats_test`, `tooie_dev_forms_test`, `tooie_glowbo_collection_test`, `tooie_dev_enemy_ai_test`,
`tooie_dev_travel_menu_test`, `tooie_object_distance_test`, `tooie_analog_camera_test`,
`recompinput_player_slots_test`, plus the required platform suite.

Source/registration anchors: `CMakeLists.txt:717`, `:729`, `:746`, `:779`, `:813`, `:825`;
`tools/windows/Build-Tooie.ps1:458..474`; `tools/build_linux.sh:19`.
These are baseline anchors; do not invent a parallel build workflow.

### 9.4 Documented build/verification commands for implementation

```powershell
# All CLI commands retain the local RTK convention.
rtk proxy pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Test -Jobs 12
rtk proxy pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Build -Jobs 12
rtk proxy git diff --check
rtk proxy git status --short
```

`-Stage Test` does not update `build-win/TooieRecompiled.exe`. The Build stage must complete before manual testing.
If the user has the game open, finish independent checks first and ask them to close it before the final link;
do not kill/control their game or test by overwriting an unrelated executable. Linux validation is native Linux
via the documented `tools/build_linux.sh test`; Windows results do not prove Linux.

If any vendored dependency is changed despite the host-only design, first inspect its existing dirty state, record
patch/ORDER/UPSTREAMS/MANIFEST, and run `LC_ALL=C bash tools/vendor_verify.sh --reverse-check` through the existing
documented shell workflow. A host-only implementation does not need speculative vendor edits.

### 9.5 Manual acceptance matrix

Each `MG01..MG23` starts `NOT_RUN`. For each stage, record the following columns separately:

```text
story entry; preparation/instructions; active play; J2 input/contact isolation;
win; loss/timeout; retry; result/menu readability; leave; restored story pair;
individual replay if accessible; native MP if applicable; native/debug-Off regression
```

Mark unsupported scenarios `N/A` with a reason; inaccessible ones `BLOCKED`/`NOT_RUN`, not PASS. Do not warp into
the arena and claim this proves its story entry/camera transition. Do not alter save unlocks to make a cell pass.

For controls, try J2 movement, attack and debug shortcut during the event. J1 must retain the native event controls
and scoring. Verify no hidden J2, second shadow, pickup credit, projectile, AI response or camera contribution.
On return, confirm J2 is controllable, privately lit, can collect one Glowbo once, can be pursued by a supported
enemy, and can use Analog Camera with Z+C priority if that option is enabled.

For Saucer, record the entire ride, intermediate loads and end/abort/retry paths, including result/score dialogue
in 0xD6 with route 6 and the deliberate wait for the next eligible recreation after it clears to 0. For both Kickballs, finish all
three rounds rather than testing only the first. For quiz, cover all three rounds in the same room. For Packing,
verify room exploration before starting remains single-player under the selected room-wide policy.

Use existing save/config backup procedure. Never choose Save and Quit as a test harness shortcut; it writes the save.

## 10. Diagnostics, review and documentation completion

Default implementation requires no new logger. Existing native calls and a targeted host harness should prove
the creation decision. If a map-transition ambiguity cannot be resolved statically, a temporary default-Off trace
may record **one event per map creation or identified controller state change**, bounded and reversible. No
per-frame actor scans, CaptureStackBackTrace, texture/model dumps or timer metrics in the final executable.

Useful temporary fields, if needed: current map, native subtype/mode, original requested count, decision reason,
viewport count before/after native creation, host-created owner identities, controller overlay identity. Never use
the trace as a permanent gameplay dependency. Remove its code, registration, temporary scripts and output before
final cleanup/commit. Preserve unrelated pre-existing opt-in diagnostics unless separately authorized for cleanup.

Before completion:

- Review actual diff against this bounded objective. No generated output/manual native-controller changes.
- Verify runtime no-injection paths still call original creation and original event controllers exactly once.
- Verify persistent settings unchanged and native multiplayer (especially one human) preserved.
- Run required tests/build and document exact commands/exit codes/HEAD/executable hash.
- Update `docs/HANDOFF.md`, `docs/specs/Tooie/window-ui-and-config.md`, relevant input/testing specs and
  `docs/reverse-engineering/systems/minigames.md`; update vault links.
- Run `markdown_vault_audit` when available; otherwise state unavailable and perform explicit local link/frontmatter
  checks. Run `git diff --check`; report worktree and commit/push status.
- Do not commit this task unless requested at the implementation/review stage. No push without explicit request.
- Remove this root plan after accepted implementation; retain the stable behavior and verified RE in canonical docs.

## 11. Event-by-event native analysis

The following sections identify native controllers and concrete source anchors for **all 15 families**, followed
by family-specific changes, expected behavior and validation discriminators. The actual fix remains centralized;
native state/event callbacks are investigation and test boundaries, not 15 independent patch targets.

<!-- EVENT_ANALYSIS_APPENDIX -->

### 11.1. Mayan Kickball

- **Stages:** MG01 / MG02 / MG03; **rooms:** `0xC6 / 0xC8 / 0xC9`.
- **Native overlay:** `chstoneballcontrol`; overlay ID `409`; metadata section `1608`.
- **Descriptor getter:** `chstoneballcontrol_entrypoint_0` at `RecompiledFuncs/funcs_273.c:L5049-L5060`.
- **Descriptor scalar identity:** section `+0x50D4`; ROM base `0x2039F40`; BE header `0x0239/0x01B7/0x0637/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_8080079C_chstoneballcontrol` | `RecompiledFuncs/funcs_332.c:L2911-L3228` |
| `+0x28` | `func_8080120C_chstoneballcontrol` | `RecompiledFuncs/funcs_191.c:L5099-L5209` |
| `+0x34` | `func_80800F9C_chstoneballcontrol` | `RecompiledFuncs/funcs_131.c:L733-L1233` |
| `+0x40` | `func_80800360_chstoneballcontrol` | `RecompiledFuncs/funcs_115.c:L646-L1552` |

**Native flow / ownership analysis:** Main descriptor is returned by entrypoint 0. Separate descriptors from entrypoints 2/3/4 share participant callbacks; these native participants are not evidence of extra human story players. `func_808046C4_chstoneballcontrol` explicitly switches on 0xC6/0xC8/0xC9 and stores 0/1/2 at its own extension +0x14; 0xBB and 0x17F have separate branches. This seals the three primary room identities without treating every named alias as a new round.

**State/event evidence:** The main callback reads Actor+0x72 >> 10. `func_80801944_chstoneballcontrol` forwards its state parameter to func_80101FDC; `func_80800908_chstoneballcontrol` requests state 8. Numeric state 8 is not automatically the final round. Native transition/result logic in func_80800360_chstoneballcontrol calls the current-map getter and native goto, including a same-map entrance-1 path and another entrance-4 path.

**Required host adjustment (PROPOSED):** Reject host injection in each of the three arenas and conservatively in 0xCA..0xCE; leave 0xBB eligibility unchanged. Preserve the Stony form, NPC participants, scoring, native cameras and stage progression. No callback replacement is necessary for the room-wide policy.

**Expected result:** Each round starts with the original human count, uses the original event camera/HUD, and progresses to the next native round/lobby without host-created J2. Entering another blocked round never briefly recreates J2.

**Manual/integration discriminators:** Run all three stages, loss/retry on each, final reward and lobby return. Repeat with J2 pressing attack/movement/debug shortcut. Native MP 0x17F must retain its original human count, including one human.

**Unresolved boundaries:** Reachability/meaning of 0xCA..0xCE and all replay entrance combinations remain UNKNOWN. No normal-story visit to those aliases is claimed.

**Additional primary anchors:**

- `func_808046C4_chstoneballcontrol` — `RecompiledFuncs/funcs_244.c:L4018-L4237`.
- `func_80801944_chstoneballcontrol` — `RecompiledFuncs/funcs_187.c:L2088-L2429`.
- `func_80800908_chstoneballcontrol` — `RecompiledFuncs/funcs_409.c:L2550-L3023`.

### 11.2. Ordnance Storage

- **Stages:** MG04; **rooms:** `0xDC`.
- **Native overlay:** `chtntmineshoot`; overlay ID `559`; metadata section `2204`.
- **Descriptor getter:** `chtntmineshoot_entrypoint_1` at `RecompiledFuncs/funcs_295.c:L143-L154`.
- **Descriptor scalar identity:** section `+0x26D8`; ROM base `0x20A96C0`; BE header `0x0158/0x02BC/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808011D4_chtntmineshoot` | `RecompiledFuncs/funcs_303.c:L1141-L1414` |
| `+0x28` | `func_80801604_chtntmineshoot` | `RecompiledFuncs/funcs_212.c:L273-L343` |
| `+0x34` | `func_80800E10_chtntmineshoot` | `RecompiledFuncs/funcs_158.c:L3604-L3671` |
| `+0x40` | `func_80801714_chtntmineshoot` | `RecompiledFuncs/funcs_199.c:L3429-L3951` |

**Native flow / ownership analysis:** The challenge controller descriptor is entrypoint 1, not entrypoint 0 (mine target/explosion actor). Its callbacks coordinate countdown/result/event handling. Entry 2 describes another actor with a separate state sequence. The shared chfpsgame machinery governs FPS player/session handling and must remain native.

**State/event evidence:** func_80800E64_chtntmineshoot ultimately calls func_80101FDC with the requested state. The controller event callback func_80801714_chtntmineshoot repeatedly branches on func_800D395C, demonstrating distinct story/replay paths. func_80801A54_chtntmineshoot requests goto(0x163, 3); that destination is the entrance room, not the 0xDC arena.

**Required host adjustment (PROPOSED):** Reject injection in 0xDC; guard related native rooms 0x164/0x165 without altering subtype 0x1B. Leave 0x163 as an exploration/entry negative control. Do not reset target counters or timer items to implement suspension.

**Expected result:** Original FPS controls, explosive-target updates, timer, win/loss and exit run with no experimental J2, even if the co-op option remains On. Native MP shooting/deathmatch remains untouched.

**Manual/integration discriminators:** Test entry from 0x163, countdown, target destruction, timeout, retry, success, and exit back to 0x163. Validate 0x164 and 0x165 separately in native multiplayer and accessible individual replay.

**Unresolved boundaries:** Full mapping of gcstatus session subtype values to every Ordnance replay variant remains UNKNOWN; never infer story solely from native player count 1.

**Additional primary anchors:**

- `func_80800E64_chtntmineshoot` — `RecompiledFuncs/funcs_22.c:L3076-L3653`.
- `func_80801A54_chtntmineshoot` — `RecompiledFuncs/funcs_297.c:L1861-L2153`.

### 11.3. Dodgem Challenge

- **Stages:** MG05 / MG06 / MG07; **rooms:** `0xDE / 0xDF / 0xE0`.
- **Native overlay:** `chdodgemcontrol`; overlay ID `445`; metadata section `1752`.
- **Descriptor getter:** `chdodgemcontrol_entrypoint_0` at `RecompiledFuncs/funcs_12.c:L2289-L2300`.
- **Descriptor scalar identity:** section `+0x4DB4`; ROM base `0x206D490`; BE header `0x02CB/0x0306/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_8080010C_chdodgemcontrol` | `RecompiledFuncs/funcs_382.c:L1177-L1647` |
| `+0x28` | `func_808015E8_chdodgemcontrol` | `RecompiledFuncs/funcs_62.c:L841-L873` |
| `+0x34` | `func_808013D0_chdodgemcontrol` | `RecompiledFuncs/funcs_260.c:L806-L1202` |
| `+0x40` | `func_80800938_chdodgemcontrol` | `RecompiledFuncs/funcs_66.c:L2640-L3409` |

**Native flow / ownership analysis:** The main descriptor has header 02CB/0306/0000/0001. Other getters describe additional game actors; do not delete opponents or confuse them with host J2. Initializer func_808013D0_chdodgemcontrol explicitly compares current map against 0xDE/0xDF/0xE0 and 0x186.

**State/event evidence:** func_8080422C_chdodgemcontrol forwards its requested state to the native state setter. Result/transition helper func_808006CC_chdodgemcontrol contains several goto(0xDD,5) paths and a same-current-map goto(...,1) branch. Native game actors have their own callbacks/state beyond the story player table.

**Required host adjustment (PROPOSED):** Block all three arena IDs, not 0xDD. Keep opponent count/balloon allocation/collision rules original. Preserve native count/topology in MP 0x186; do not apply a blanket count=1 write.

**Expected result:** One experimental story human participates; the three stages keep 1/2/3 native opponents. Lobby re-entry restores normal co-op creation.

**Manual/integration discriminators:** Validate each opponent count, win/loss/retry, bumper interaction, balloons/score HUD, stage unlock progression and 0xDD return. Also test native MP with one and multiple humans.

**Unresolved boundaries:** All state-number-to-dialogue meanings are not assigned here; state setter parameter forwarding is proven, not a complete semantic enumeration.

**Additional primary anchors:**

- `func_808006CC_chdodgemcontrol` — `RecompiledFuncs/funcs_355.c:L284-L786`.
- `func_8080422C_chdodgemcontrol` — `RecompiledFuncs/funcs_193.c:L2851-L2953`.

### 11.4. Hoop Hurry

- **Stages:** MG08; **rooms:** `0xE5`.
- **Native overlay:** `chhoopswoop`; overlay ID `259`; metadata section `1016`.
- **Descriptor getter:** `chhoopswoop_entrypoint_0` at `RecompiledFuncs/funcs_265.c:L1314-L1325`.
- **Descriptor scalar identity:** section `+0x1EC0`; ROM base `0x1FB94E0`; BE header `0x02B7/0x0355/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_8080016C_chhoopswoop` | `RecompiledFuncs/funcs_76.c:L3168-L3518` |
| `+0x28` | `func_80800620_chhoopswoop` | `RecompiledFuncs/funcs_14.c:L453-L554` |
| `+0x34` | `func_80800024_chhoopswoop` | `RecompiledFuncs/funcs_241.c:L6093-L6190` |
| `+0x40` | `func_80800320_chhoopswoop` | `RecompiledFuncs/funcs_181.c:L282-L619` |

**Native flow / ownership analysis:** Entrypoint 0 returns the main story descriptor; entrypoint 1 returns a related variant, and entrypoint 2 describes another challenge actor. Main event dispatcher func_80800320_chhoopswoop directly dispatches event 0x95 to func_808004BC_chhoopswoop; it also contains payload-based event paths and a replay-specific branch.

**State/event evidence:** Main update requests states 4 and 5 at 0x80800298/0x808002E0. Start helper requests state 2 at 0x808005AC. Result helper func_80800910_chhoopswoop requests state 6 and has a native goto(0xE5,1) path. Those numbers belong to this controller, not a universal minigame enum.

**Required host adjustment (PROPOSED):** Block story 0xE5; preserve MP 0x17C native behavior. Apply before actor/controller initialization; no need to hook event 0x95 for suspension.

**Expected result:** Native movement and hoop scoring work in the original viewport and native camera from preparation through retry. J2 cannot add score or collide while absent.

**Manual/integration discriminators:** Test waiting before start, event start, scoring, timer expiry, success, retry in the same map and departure. Verify there is no delayed J2 release after waiting more than two seconds.

**Unresolved boundaries:** The exact full lifecycle of the related variant descriptor is not established by the getter alone; replay room assignment must be observed, not guessed.

**Additional primary anchors:**

- `func_808004BC_chhoopswoop` — `RecompiledFuncs/funcs_52.c:L5600-L5874`.
- `func_80800910_chhoopswoop` — `RecompiledFuncs/funcs_170.c:L1590-L2215`.

### 11.5. Balloon Burst

- **Stages:** MG09; **rooms:** `0xE4`.
- **Native overlay:** `chballoonburst`; overlay ID `257`; metadata section `1008`.
- **Descriptor getter:** `chballoonburst_entrypoint_0` at `RecompiledFuncs/funcs_222.c:L6243-L6254`.
- **Descriptor scalar identity:** section `+0x1C40`; ROM base `0x1FB6350`; BE header `0x02B1/0x034F/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_80800098_chballoonburst` | `RecompiledFuncs/funcs_120.c:L1514-L1796` |
| `+0x28` | `func_808008F0_chballoonburst` | `RecompiledFuncs/funcs_71.c:L6951-L7052` |
| `+0x34` | `func_80800024_chballoonburst` | `RecompiledFuncs/funcs_209.c:L709-L800` |
| `+0x40` | `func_808001FC_chballoonburst` | `RecompiledFuncs/funcs_185.c:L2357-L2736` |

**Native flow / ownership analysis:** Entrypoint 0 returns the main story descriptor; entrypoint 1 is another variant. func_808001FC_chballoonburst dispatches event 0x95 to func_80800790_chballoonburst and contains a capod camera lookup path on event 0xE with payload 4. That camera path must not be displaced by a host second camera.

**State/event evidence:** Main update requests states 4/5 at 0x80800184/0x808001C0; start helper requests state 2 at 0x8080088C. Result helper func_808003C4_chballoonburst requests state 6 and contains goto(0xE4,1). Event/update transition numbers are local to this game.

**Required host adjustment (PROPOSED):** Block story 0xE4 and classify related 0x17B as no-host-injection. Do not swap 0xE4/0xE5 because the flag list orders Hoop before Balloon while the map enum orders Balloon before Hoop.

**Expected result:** Native balloons, aiming/movement, timer, score and camera work with original player topology; no invisible second scorer or second camera.

**Manual/integration discriminators:** Test both success and timeout, same-map retry, event camera transitions, exit and restored exploration pair; test native MP separately.

**Unresolved boundaries:** No complete semantic naming of all balloon actor states is claimed; no native callback modification is proposed.

**Additional primary anchors:**

- `func_80800790_chballoonburst` — `RecompiledFuncs/funcs_273.c:L2015-L2276`.
- `func_808003C4_chballoonburst` — `RecompiledFuncs/funcs_401.c:L530-L1089`.

### 11.6. Saucer of Peril

- **Stages:** MG10; **rooms:** `0x124 / 0x13B / 0x13C`; **shared return/result context:** `0xD6` with native route 6.
- **Native overlay:** `chflysaucer`; overlay ID `430`; metadata section `1692`.
- **Descriptor getter:** `chflysaucer_entrypoint_1` at `RecompiledFuncs/funcs_376.c:L2392-L2403`.
- **Descriptor scalar identity:** section `+0x4668`; ROM base `0x20585B0`; BE header `0x0202/0x02A2/0x0690/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808024E0_chflysaucer` | `RecompiledFuncs/funcs_34.c:L998-L2671` |
| `+0x14` | `func_808035C4_chflysaucer` | `RecompiledFuncs/funcs_367.c:L2590-L3095` |
| `+0x28` | `func_80803150_chflysaucer` | `RecompiledFuncs/funcs_74.c:L3722-L3818` |
| `+0x34` | `func_80802ED8_chflysaucer` | `RecompiledFuncs/funcs_121.c:L371-L929` |
| `+0x40` | `func_8080185C_chflysaucer` | `RecompiledFuncs/funcs_38.c:L1482-L2970` |

**Native flow / ownership analysis:** Descriptor entrypoint 1 supplies the main saucer callbacks. Initializer func_80802ED8_chflysaucer explicitly branches on current map 0x124/0x13B/0x13C/0xD6. Transition helper func_80800514_chflysaucer dispatches on extension byte +0x26 through section-1692 table +0x3E48, with six native goto destinations: 0x124 entrance 8; 0x13C entrance 1; 0x124 entrance 8; 0x13B entrance 1; 0x124 entrance 1; 0xD6 entrance 8. Table-index mapping is recorded below, verified from ROM.

**State/event evidence:** func_80800000_chflysaucer reads a three-bit native field via func_800DA564(0xA34,3), then publishes the result at extension +0x26. func_8080003C_chflysaucer writes it via func_800DA7A8(0xA34,value,3) and mirrors the byte. This is a useful itinerary discriminator, not any of the global replay flags. State helper func_80801568_chflysaucer and current-state update remain native. Reward queries include index 0x18/type 1; decomp/include/jiggies.h identifies that Jiggy as Saucer of Peril.

**Required host adjustment (PROPOSED):** Block all three dedicated SOP maps before creation and the proven 0xD6/route-6 return/result context. Ordinary 0xD6 loads remain eligible. Read the native route at creation through the flag getter; no duplicate host itinerary token. After the result clears route 6 to 0 in the same map, restore J2 only at the next eligible native recreation (sections 4.3/12.4).

**Expected result:** No J2 reappearance between repeated 0x124 visits and 0x13B/0x13C segments. Native rail camera, shooting, accumulated score, reward and result flow survive intermediate loads. Co-op returns only when ordinary exploration is established.

**Manual/integration discriminators:** Record every native map load across an entire ride, all intermediate segments, normal finish, abort, failure, retry and individual replay. Verify ordinary Witchyworld 0xD6 entry before the ride is still co-op, return/results remain native single-player, and a later eligible creation restores co-op. A test of only the starting room is insufficient.

**Unresolved boundaries:** Complete clean runtime traversal, all abort variants and native field 0xA34 reset paths outside the verified advance helper remain UNKNOWN. Selector 6 is now proven to initialize a return/result context; it is not proof that the event has already finished. sucoasterflydata contains prop selectors rather than obvious map IDs; do not import that table as a map itinerary by name alone.

**Additional primary anchors:**

- `func_80800514_chflysaucer` — `RecompiledFuncs/funcs_339.c:L87-L274`.
- `func_80800000_chflysaucer` — `RecompiledFuncs/funcs_212.c:L4849-L4892`.
- `func_8080003C_chflysaucer` — `RecompiledFuncs/funcs_248.c:L3965-L4012`.
- `func_80800E48_chflysaucer` — `RecompiledFuncs/funcs_69.c:L1969-L2260`.
- `func_80801568_chflysaucer` — `RecompiledFuncs/funcs_91.c:L921-L1435`.

### 11.7. Mini-Sub Challenge

- **Stages:** MG11; **rooms:** `0x181`.
- **Native overlay:** `chminisubgame`; overlay ID `202`; metadata section `792`.
- **Descriptor getter:** `chminisubgame_entrypoint_0` at `RecompiledFuncs/funcs_369.c:L4055-L4066`.
- **Descriptor scalar identity:** section `+0x1424`; ROM base `0x1F8B1E0`; BE header `0x0158/0x04AD/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808000C8_chminisubgame` | `RecompiledFuncs/funcs_238.c:L1197-L1882` |
| `+0x28` | `func_80800430_chminisubgame` | `RecompiledFuncs/funcs_21.c:L5463-L5531` |
| `+0x34` | `func_80800018_chminisubgame` | `RecompiledFuncs/funcs_261.c:L5444-L5568` |
| `+0x40` | `func_80800484_chminisubgame` | `RecompiledFuncs/funcs_287.c:L2255-L3393` |

**Native flow / ownership analysis:** Main descriptor is entrypoint 0; entrypoint 1 is a target/secondary actor. The main event callback explicitly tests reward index 0x1F with type 1; jiggies.h identifies JRL_MINI_SUB_CHALLENGE. Event codes and replay branch live inside func_80800484_chminisubgame.

**State/event evidence:** Main update examines actor state and requests state 3 at 0x808002F0/0x80800418. Event callback requests state 5 in several paths. Result helper func_80800BA8_chminisubgame can request states 2/4 and goto either (0x181,1) or (0x1A9,0x1B). The caller branch distinguishes result/choice; do not call both destinations equivalent arenas.

**Story versus replay result dispatch (PROVEN / STATIC_MIPS):** In that helper, tag `0xCEB` reaches
`0x80800DFC`. If `func_800D395C()` returns zero, callback argument `a2 == 1` chooses `(0x181,1)`; other values
choose `(0x1A9,0x1B)` before the shared goto at `0x80800E88`. With a nonzero result, `a2 == 1` instead calls
resident `0x80088188` (`_gcfrontend_entrypoint_11`); other values call `0x80088180`
(`_gcfrontend_entrypoint_10`). Both replay branches jump past the local goto. Tag `0xCEB` is an event/choice
identifier here, not a map, actor phase or replay-availability flag. The original incoming `a2` is saved at
`sp+0x40`; do not interpret the register after nested calls as the original choice. Frontend entrypoint semantics
and all eventual replay destinations remain UNKNOWN in this analysis; the local branch distinction is proven.

**Required host adjustment (PROPOSED):** Block the 0x181 cave and related native MP 0x182; preserve general sea-bottom 0x1A9. Do not force the submarine player form or destroy the second descriptor actor to remove J2.

**Expected result:** Native sub controls, targets, torpedo aiming, score and timer remain original. Retry remains single-player; returning to eligible sea-bottom exploration permits co-op creation again.

**Manual/integration discriminators:** Story cave entry, start, target score, success/failure, repeat and leave;
verify ordinary 0x1A9 before/after. Test native 0x182 count=1 and >1 separately. Exercise the replay frontend
choices independently of story retry/exit; integration case 20 must not assume every result callback loads a map.

**Unresolved boundaries:** Exact semantic labels for every event payload/result choice and all replay entrance variants remain UNKNOWN. No restoration based on merely observing 0x1A9 as an immediate constant is acceptable.

**Additional primary anchors:**

- `func_80800BA8_chminisubgame` — `RecompiledFuncs/funcs_158.c:L5030-L5685`.
- Replay resident thunks: `RecompiledFuncs/funcs_75.c:951..959`, `funcs_310.c:128..136`.

### 11.8. Chompa's Belly

- **Stages:** MG12; **rooms:** `0x123`.
- **Native overlay:** `chmonsterinterior`; overlay ID `336`; metadata section `1320`.
- **Descriptor getter:** `chmonsterinterior_entrypoint_0` at `RecompiledFuncs/funcs_50.c:L2326-L2337`.
- **Descriptor scalar identity:** section `+0x1E80`; ROM base `0x1FFC5A0`; BE header `0x0158/0x0451/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808000BC_chmonsterinterior` | `RecompiledFuncs/funcs_365.c:L5634-L6057` |
| `+0x28` | `func_8080060C_chmonsterinterior` | `RecompiledFuncs/funcs_130.c:L1537-L1638` |
| `+0x34` | `func_8080003C_chmonsterinterior` | `RecompiledFuncs/funcs_128.c:L3157-L3260` |
| `+0x40` | `func_808002D4_chmonsterinterior` | `RecompiledFuncs/funcs_398.c:L1447-L2150` |

**Native flow / ownership analysis:** Correct controller is chmonsterinterior. Main descriptor is entrypoint 0; entrypoint 1 describes a related variant, and entries 2/3/4 describe target variants. func_80800C38_chmonsterinterior calls func_800D0A9C(0x2E,1), matching TDL_CHOMPAS_BELLY in jiggies.h. func_8080078C_chmonsterinterior has a direct goto(0x123,1) retry path. This corroborates ownership beyond overlay naming.

**State/event evidence:** Main update requests states 4/5. Main event callback requests states 7 and 2 at 0x808003E8/0x80800524. Result helper requests state 6 at 0x808008D8. These must remain native; the target actors have independent 2/3/4 transitions and are not player slots.

**Required host adjustment (PROPOSED):** Block 0x123 and related MP 0x183. Do not patch chfishshootgame: its initialization queries reward 0x21/type 1, matching Chris P. Bacon rather than Chompa. chchompaslunch is a separate small actor, not the central controller.

**Expected result:** Native target colors/patterns, score, timer, shots, result and retry run without experimental J2, while leaving all native target actors intact.

**Manual/integration discriminators:** Start from actual Chompa entry, shoot each target type, exercise score/time finish and loss, same-room repeat, exit and restored pair. Verify both story and accessible replay.

**Unresolved boundaries:** Not all target variant semantics/colors were decoded; that knowledge is not required for room-wide suspension. Do not assign colors solely from descriptor IDs.

**Additional primary anchors:**

- `func_8080078C_chmonsterinterior` — `RecompiledFuncs/funcs_234.c:L6863-L7343`.
- `func_80800C38_chmonsterinterior` — `RecompiledFuncs/funcs_37.c:L805-L1092`.

### 11.9. Clinker's Cavern

- **Stages:** MG13; **rooms:** `0x162`.
- **Native overlay:** `chclinker`; overlay ID `560`; metadata section `2208`.
- **Descriptor getter:** `chclinker_entrypoint_1` at `RecompiledFuncs/funcs_43.c:L2129-L2140`.
- **Descriptor scalar identity:** section `+0x1550`; ROM base `0x20AC150`; BE header `0x0158/0x02BF/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808007BC_chclinker` | `RecompiledFuncs/funcs_297.c:L2828-L3046` |
| `+0x28` | `func_808008C0_chclinker` | `RecompiledFuncs/funcs_135.c:L1004-L1029` |
| `+0x34` | `func_80800574_chclinker` | `RecompiledFuncs/funcs_341.c:L3833-L3860` |
| `+0x40` | `func_80800E1C_chclinker` | `RecompiledFuncs/funcs_379.c:L3896-L4332` |

**Native flow / ownership analysis:** Entrypoint 0 returns an individual Clinker actor descriptor; entrypoint 1 returns the controller descriptor. The shared chfpsgame entrypoint-0 descriptor performs native FPS session work and is analyzed in the shared appendix. Do not equate an arbitrary FPS session with this one minigame.

**State/event evidence:** Controller update func_808007BC_chclinker reads actor state and updates native HUD/timer-related services, including item ID 0xCC. func_808008E0_chclinker eventually forwards a new state to func_80101FDC; func_8080100C_chclinker explicitly requests 3/2/4. The scalar 0xCC in timer service calls is not proof of a map transition to map 0xCC.

**Required host adjustment (PROPOSED):** Block 0x162; preserve sewer entrance 0x187, ordinary GI Floor 4 and unrelated FPS areas. Keep native Breegull Blaster/FPS control routing and counters untouched. Both controller and individual Clinker callbacks remain original.

**Expected result:** Original FPS viewpoint/weapon, Clinker disappearance/count, countdown, success/failure and exit work with no injected story J2.

**Manual/integration discriminators:** Actual sewer/cavern transition, FPS setup, all target kills, timeout, completion, exit and restored normal co-op. Also run an unrelated FPS area as a negative control.

**Unresolved boundaries:** All gcstatus FPS subtype-to-map values and exact exit/replay variants remain UNKNOWN; the room decision does not require guessing them.

**Additional primary anchors:**

- `func_808008E0_chclinker` — `RecompiledFuncs/funcs_391.c:L1217-L1855`.
- `func_8080100C_chclinker` — `RecompiledFuncs/funcs_195.c:L980-L1218`.
- `func_808011A8_chclinker` — `RecompiledFuncs/funcs_258.c:L2220-L2483`.

### 11.10. Twinkly Packing

- **Stages:** MG14; **rooms:** `0x10A`.
- **Native overlay:** `chfactorytwinklies`; overlay ID `333`; metadata section `1308`.
- **Descriptor getter:** `chfactorytwinklies_entrypoint_0` at `RecompiledFuncs/funcs_87.c:L2691-L2702`.
- **Descriptor scalar identity:** section `+0x2218`; ROM base `0x1FF71E0`; BE header `0x0158/0x0442/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808001A0_chfactorytwinklies` | `RecompiledFuncs/funcs_19.c:L5579-L6066` |
| `+0x28` | `func_8080064C_chfactorytwinklies` | `RecompiledFuncs/funcs_85.c:L4404-L4505` |
| `+0x34` | `func_80800048_chfactorytwinklies` | `RecompiledFuncs/funcs_34.c:L6298-L6580` |
| `+0x40` | `func_8080040C_chfactorytwinklies` | `RecompiledFuncs/funcs_43.c:L401-L889` |

**Native flow / ownership analysis:** Entrypoint 0 returns main story descriptor; entrypoint 1 a related variant; entries 3/4/5 are Twinkly variants with shared callbacks and distinct descriptor identities. Do not disable those actors to suppress J2. Result evaluation queries the event reward through its own native controller flow.

**State/event evidence:** Main update requests 4/5; main event callback requests state 7. func_80800748_chfactorytwinklies requests state 6 and has goto(0x10A,1). Twinkly actor update func_808018A0_chfactorytwinklies independently requests 4/2/3; these are pickup actors, not active-game phase IDs.

**Required host adjustment (PROPOSED):** Block whole Packing Room 0x10A, including exploration/preparation/result there. Preserve GI Floor 3 0x108 and native MP 0x17D topology. Do not suspend only after the timer begins, which would leave a previously created J2 alive.

**Expected result:** Only native player participation, native Twinkly spawning/collection/deposit/scoring and timer. Same-map repeat stays native; leaving for exploration restores the pair.

**Manual/integration discriminators:** Stand in room before starting for >2 seconds; start challenge; collect/deposit all relevant Twinkly variants; win/loss/retry; leave. Confirm selected room-wide policy with no premature J2 release.

**Unresolved boundaries:** The entire physical room remains blocked under this design even when no challenge is active. If that UX is rejected later, a separate safe mid-room suspension design is required.

**Additional primary anchors:**

- `func_80800748_chfactorytwinklies` — `RecompiledFuncs/funcs_181.c:L1262-L1808`.
- `func_80800B80_chfactorytwinklies` — `RecompiledFuncs/funcs_406.c:L5485-L5786`.

### 11.11. Colosseum Kickball

- **Stages:** MG15 / MG16 / MG17; **rooms:** `0x12E / 0x12F / 0x130`.
- **Native overlay:** `chcollballcontrol`; overlay ID `467`; metadata section `1840`.
- **Descriptor getter:** `chcollballcontrol_entrypoint_0` at `RecompiledFuncs/funcs_357.c:L13-L24`.
- **Descriptor scalar identity:** section `+0x4DA8`; ROM base `0x2082840`; BE header `0x003F/0x019E/0x0637/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_808006FC_chcollballcontrol` | `RecompiledFuncs/funcs_308.c:L3679-L4125` |
| `+0x28` | `func_80801038_chcollballcontrol` | `RecompiledFuncs/funcs_56.c:L5114-L5224` |
| `+0x34` | `func_80800DD8_chcollballcontrol` | `RecompiledFuncs/funcs_34.c:L7301-L7780` |
| `+0x40` | `func_808002B8_chcollballcontrol` | `RecompiledFuncs/funcs_251.c:L1870-L2766` |

**Native flow / ownership analysis:** Separate overlay from Mayan Kickball, with a separate main descriptor and participant descriptors from entries 2/3/4. Similar callback architecture is not permission to replace it with Mayan scoring or to assume identical state semantics. Native MP arena is 0x180.

**State/event evidence:** func_80801764_chcollballcontrol forwards requested state to func_80101FDC; func_808008FC_chcollballcontrol requests state 8. Main event helper func_808002B8_chcollballcontrol has native current-map retry/entrance-1 logic and an entrance-5 path distinct from Mayan entrance 4.

**Required host adjustment (PROPOSED):** Block all three 0x12E/0x12F/0x130 arenas; keep lobby 0x12D unchanged. Preserve native scoring direction, balls/opponents, Stony legality and tournament progression. Do not use an overly broad range including the lobby.

**Expected result:** All three rounds use native participation and game rules; transitions between arenas remain suspended; lobby/exploration return restores co-op.

**Manual/integration discriminators:** Run each round including loss/retry and final victory, and ensure original Colosseum scoring remains intact. Test native MP 0x180 with one and multiple humans.

**Unresolved boundaries:** Detailed role of every secondary descriptor and complete round-selector field mapping remain UNKNOWN; primary arena identities come from maps.h and the family itself is separate from Mayan.

**Additional primary anchors:**

- `func_80801764_chcollballcontrol` — `RecompiledFuncs/funcs_294.c:L1814-L2155`.
- `func_808008FC_chcollballcontrol` — `RecompiledFuncs/funcs_401.c:L4162-L4653`.

### 11.12. Pot O' Gold

- **Stages:** MG18; **rooms:** `0x13D`.
- **Native overlay:** `chpotogoldgame`; overlay ID `592`; metadata section `2336`.
- **Descriptor getter:** `chpotogoldgame_entrypoint_4` at `RecompiledFuncs/funcs_72.c:L2696-L2713`.
- **Descriptor scalar identity:** section `+0x2578`; ROM base `0x20CABE0`; BE header `0x0163/0x0405/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_80800124_chpotogoldgame` | `RecompiledFuncs/funcs_324.c:L2637-L3288` |
| `+0x34` | `func_80800000_chpotogoldgame` | `RecompiledFuncs/funcs_151.c:L1091-L1338` |
| `+0x40` | `func_80800A94_chpotogoldgame` | `RecompiledFuncs/funcs_186.c:L1209-L2541` |

**Native flow / ownership analysis:** Important export distinction: entrypoint 4 returns the main actor descriptor; entrypoint 0 is an actual behavior/event helper, not a descriptor getter. The descriptor points to initializer func_80800000_chpotogoldgame, update func_80800124_chpotogoldgame and event callback func_80800A94_chpotogoldgame.

**State/event evidence:** func_80800434_chpotogoldgame ends through native state forwarding. entrypoint 0 resolves marker->actor and requests local state 0xE in the func_800D395C branch, state 8 otherwise. That is another concrete reason not to use one numeric active-state rule for all minigames.

**Required host adjustment (PROPOSED):** Block interior 0x13D before player creation. Keep exterior Cloud Cuckooland exploration and activation/opening mechanics original. Do not alter the Pot activation persistent flag, score or native focus queries.

**Expected result:** Original aiming/targets, timer, score, setup and result path use native single-player presentation and controls, with no host-created second participant.

**Manual/integration discriminators:** Activate/access through normal story progression, enter, start, hit targets, timeout/win, repeat if offered, and leave. Validate exterior co-op before/after.

**Unresolved boundaries:** A separate native MP Pot room has not been established. Do not invent one or infer it from a numeric constant passed to an unrelated service.

**Additional primary anchors:**

- `func_80800434_chpotogoldgame` — `RecompiledFuncs/funcs_119.c:L3886-L5308`.
- `chpotogoldgame_entrypoint_0` — `RecompiledFuncs/funcs_177.c:L218-L297`.

### 11.13. Trash Can Germs

- **Stages:** MG19; **rooms:** `0x137`.
- **Native overlay:** `chtrashcangame`; overlay ID `604`; metadata section `2384`.
- **Descriptor getter:** `chtrashcangame_entrypoint_0` at `RecompiledFuncs/funcs_293.c:L3446-L3457`.
- **Descriptor scalar identity:** section `+0x31B4`; ROM base `0x20D50A0`; BE header `0x0158/0x0475/0x0808/0x0003`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_80800180_chtrashcangame` | `RecompiledFuncs/funcs_333.c:L3180-L3691` |
| `+0x40` | `func_8080093C_chtrashcangame` | `RecompiledFuncs/funcs_312.c:L2119-L2609` |

**Native flow / ownership analysis:** Entrypoint 0 is the main controller; entrypoint 1 is another controller variant; entrypoint 2 describes a germ actor with separate state/event handling. More native actors do not imply more human player slots.

**State/event evidence:** Main update func_80800180_chtrashcangame and event callback func_8080093C_chtrashcangame coordinate the challenge; func_808003D0_chtrashcangame and func_80800728_chtrashcangame contain func_800D395C branches. Another variant helper func_80801EE8_chtrashcangame forwards native state; no universal activity flag is demonstrated.

**Required host adjustment (PROPOSED):** Block 0x137 and guard related native MP 0x185. Leave native germ creation, damage and cleanup behavior untouched; do not broaden the existing Glowbo-specific fix to arbitrary minigame items.

**Expected result:** Native germ collisions/attacks/counting, movement and result work for J1 alone; J2 cannot damage germs or contribute to challenge progress while absent.

**Manual/integration discriminators:** Actual entry/start, germ interaction, success and timeout/failure, repeat and exit. Confirm no hidden J2 projectile/attack survives the transition. Test native MP 0x185 separately.

**Unresolved boundaries:** All main/variant numeric phase semantics and every exit destination are not exhaustively decoded; room-wide suspension avoids coupling to those unknowns.

**Additional primary anchors:**

- `func_808003D0_chtrashcangame` — `RecompiledFuncs/funcs_186.c:L2643-L3342`.
- `func_80800728_chtrashcangame` — `RecompiledFuncs/funcs_81.c:L297-L739`.

### 11.14. Zubbas' Hive

- **Stages:** MG20; **rooms:** `0x139`.
- **Native overlay:** `chwaspnestgame`; overlay ID `198`; metadata section `776`.
- **Descriptor getter:** `chwaspnestgame_entrypoint_0` at `RecompiledFuncs/funcs_186.c:L366-L377`.
- **Descriptor scalar identity:** section `+0x1F80`; ROM base `0x1F86C40`; BE header `0x0158/0x01E3/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_8080047C_chwaspnestgame` | `RecompiledFuncs/funcs_34.c:L291-L827` |
| `+0x28` | `func_80800400_chwaspnestgame` | `RecompiledFuncs/funcs_194.c:L2925-L3023` |
| `+0x34` | `func_808002EC_chwaspnestgame` | `RecompiledFuncs/funcs_19.c:L25-L214` |
| `+0x40` | `func_80800724_chwaspnestgame` | `RecompiledFuncs/funcs_142.c:L2379-L3137` |

**Native flow / ownership analysis:** Entrypoint 0 returns the main controller; entrypoint 1 describes a secondary actor. Main callbacks maintain distinct native player/score-related services and have replay-sensitive branches. The map belongs to the hive challenge, not all of Cloud Cuckooland.

**State/event evidence:** Main update func_8080047C_chwaspnestgame requests state 3 at 0x808006F0; event callback requests state 5 at 0x80800804 and state 3 at 0x80800AC8. Result helper func_80800F98_chwaspnestgame chooses among destinations including 0x136 and 0x139 before calling native goto; repeat and exit must be tested independently.

**Required host adjustment (PROPOSED):** Block 0x139 and related native MP 0x188; preserve 0x136 exterior. Retain native Bee form/control requirements, flying/shooting camera, enemy wave/target logic and results.

**Expected result:** Original hive event and native camera/control scheme with no experimental J2; replay/retry remains native and exterior exploration restores co-op.

**Manual/integration discriminators:** Enter in the legitimate story form, start, score/waves, timeout/loss, success, repeat and exterior return. Exercise native MP count 1 and >1.

**Unresolved boundaries:** Which result/choice values select each destination and all secondary actor behavior labels remain UNKNOWN; no direct persistent flag write is proposed.

**Additional primary anchors:**

- `func_80800018_chwaspnestgame` — `RecompiledFuncs/funcs_355.c:L2955-L3529`.
- `func_80800F98_chwaspnestgame` — `RecompiledFuncs/funcs_241.c:L1245-L1797`.

### 11.15. Tower of Tragedy

- **Stages:** MG21 / MG22 / MG23; **rooms:** `0x15F for all three rounds`.
- **Native overlay:** `chquiz`; overlay ID `629`; metadata section `2484`.
- **Descriptor getter:** `chquiz_entrypoint_1` at `RecompiledFuncs/funcs_392.c:L5574-L5585`.
- **Descriptor scalar identity:** section `+0x7334`; ROM base `0x20ED950`; BE header `0x0158/0x0529/0x0000/0x0001`.
- **Evidence:** descriptor/getter/callback relations PROVEN / STATIC_MIPS; complete live event traversal NOT_RUN.

| Descriptor pointer slot | Resolved native callback | Baseline source span |
| --- | --- | --- |
| `+0x0C` | `func_80802664_chquiz` | `RecompiledFuncs/funcs_291.c:L1111-L1831` |
| `+0x28` | `func_808029A4_chquiz` | `RecompiledFuncs/funcs_189.c:L5491-L5602` |
| `+0x34` | `func_80802628_chquiz` | `RecompiledFuncs/funcs_276.c:L4821-L4860` |
| `+0x40` | `func_80802A2C_chquiz` | `RecompiledFuncs/funcs_23.c:L2310-L4195` |

**Native flow / ownership analysis:** chquiz contains multiple descriptors: entrypoint 0 presenter-related actor, entrypoint 1 controller, entrypoint 2 another actor, entrypoint 3 another participant/UI-related actor. Those semantic qualifiers are tentative; their raw callback identity is proven. suquiz is the separate exported quiz subsystem. All three requested replay stages share the story quiz room, so a per-round host toggle is unnecessary.

**State/event evidence:** Controller event func_80802A2C_chquiz has func_800D395C branches and quiz subsystem interaction; larger helpers func_80805404_chquiz, func_808058F0_chquiz and func_80803BFC_chquiz also contain story/replay paths. Do not equate three distinct catalog flags with three actor-state numbers or three distinct map loads.

**Required host adjustment (PROPOSED):** Block 0x15F for the entire visit: introductions, round transitions, questions, scoreboards, eliminations and final result. Related native MP room 0x189 remains native. Do not alter quiz question order, persistent completion flag or native participant/answer routing.

**Expected result:** Original single-player quiz layout, answer controls, question/score display and round transitions; J2 cannot answer as an extra story participant. No J2 recreation between rounds in the same room.

**Manual/integration discriminators:** Exercise all three rounds separately, answer input, scoreboard, loss/elimination and final success, and the transition out. Individual replay round selection and native MP 0x189 require independent coverage.

**Unresolved boundaries:** Full quiz global struct/round-index offsets and all presenter/participant descriptor meanings remain UNKNOWN. They are not required to keep one room suspended across three rounds.

**Additional primary anchors:**

- `func_808058F0_chquiz` — `RecompiledFuncs/funcs_113.c:L5705-L8485`.
- `func_80805404_chquiz` — `RecompiledFuncs/funcs_138.c:L297-L1396`.
- `func_80803BFC_chquiz` — `RecompiledFuncs/funcs_241.c:L4732-L5679`.

## 12. Shared machinery and narrow discriminators

### 12.1. Saucer transition-selector table (PROVEN / STATIC_MIPS + STATIC_ROM)

The selector is the controller extension byte `+0x26`; table dispatch subtracts 1, bounds-checks 6, and uses
section 1692 `+0x3E48`. Entries below were verified against the canonical decompressed ROM and inspected
destination instruction blocks in `func_80800514_chflysaucer`. Table order is not inferred from source label order.

| Selector | Native branch target | `gcgoto` map | Entrance |
| --- | --- | --- | --- |
| `1` | `0x8080056C` | `0x124` | `8` |
| `2` | `0x80800580` | `0x13C` | `1` |
| `3` | `0x80800594` | `0x124` | `8` |
| `4` | `0x808005A8` | `0x13B` | `1` |
| `5` | `0x808005BC` | `0x124` | `1` |
| `6` | `0x808005D0` | `0xD6` | `8` |

This establishes target selection, not the whole live itinerary or all abort/reset paths. The main initializer
handles `0xD6` as well as the dedicated rooms; section 12.4 resolves the selector-6 return/result branch.
Do not blacklist all of `0xD6`.

### 12.2. FPS controller

`chfpsgame_entrypoint_0` returns section 2216 `+0x21A0`, header `0158/02C4/FFFF/0001`.
Its pointer `+0x0C` resolves to `func_808008D4_chfpsgame`, which calls, in order:

```text
func_808015EC_chfpsgame
func_80800FF4_chfpsgame
func_80801758_chfpsgame
func_808018E0_chfpsgame
```

Initializer `func_808002FC_chfpsgame` reads native status through resident `0x80088380`, initializes a session,
obtains native player count through `0x80088368` (`_gcstatusDll_entrypoint_11`), and dispatches among local
setup paths using the native subtype/session value. Preserve that dispatch and native count.

Primary locators: `RecompiledFuncs/funcs_82.c:L3712-L3723`, `RecompiledFuncs/funcs_176.c:L5252-L5301`, `RecompiledFuncs/funcs_361.c:L2109-L2291`.

Native FPS presence is not a sufficient classifier for just Ordnance/Clinker. Both have dedicated map IDs;
other FPS areas and native deathmatch must remain unchanged.

### 12.3. Rejected identity shortcuts

- `chfishshootgame` is not Chompa: `func_8080000C_chfishshootgame` queries `(0x21,1)` at original
  `0x80800040..0x80800048`; `jiggies.h` names 0x21 `JRL_CHRIS_P_BACON`. Chompa controller queries `(0x2E,1)`
  and has direct retry to map 0x123. Do not patch both because both involve shooting.
- `chchompaslunch` is a separate actor with its own short state sequence. It is not the main score/timer controller.
- `chpotogoldgame_entrypoint_0` executes logic; entrypoint 4 is the descriptor getter. Export index 0 is not a
  universal actor-descriptor convention.
- `sucoasterflydata` table +0x290 has stride 0x28, but its leading halfwords are 0x2FD/0x1A4/0x1A5/0x2AA/0x2AD;
  entrypoint 0 compares them to a map-linked object field shifted by 22. Do not treat those numbers as the
  Saucer map itinerary or add them to the room blacklist. The directly verified chflysaucer goto table is preferable.
- A literal 0xCC passed to a timer/HUD service is not a map transition. A map requires a proven current-map
  comparison or native goto argument; substring searches alone are insufficient.

Primary locators: `RecompiledFuncs/funcs_293.c:L4345-L4627`, `RecompiledFuncs/funcs_37.c:L805-L1092`,
`RecompiledFuncs/funcs_278.c:L3890-L4038`, `RecompiledFuncs/funcs_412.c:L3801-L3927`.

### 12.4. Saucer route advancement and return/result initialization

`PROVEN / STATIC_ROM + STATIC_MIPS`: two additional native jump tables were verified against the same canonical
decompressed ROM and section 1692 base used in 2.2. Combined with the existing goto table, they resolve the
normal route values without assuming that code-label order equals table order.

| Current route | Advance table `+0x3E60` target | Value stored by advance helper | Goto after store | Initialization table `+0x4214` target for current value |
| --- | --- | --- | --- | --- |
| 0 | `0x80800638` | 1 | `(0x124,8)` only when `func_800D395C()==0`; replay branch stays local | `0x80802F80` (native initial/replay/availability branches) |
| 1 | `0x8080067C` | 2 | `(0x13C,1)` | `0x80803104` (requires 0x124; actor state 2) |
| 2 | `0x80800698` | 3 | `(0x124,8)` | `0x80803054` (requires 0x13C; resume helper) |
| 3 | `0x808006B4` | 4 | `(0x13B,1)` | `0x80803080` (requires 0x124; resume helper) |
| 4 | `0x808006D0` | 5 | `(0x124,1)` | `0x808030AC` (requires 0x13B; resume helper) |
| 5 | `0x808006EC` | 6 | `(0xD6,8)` | `0x808030D8` (requires 0x124; resume helper) |
| 6 | `0x80800708` | 0 | **No goto in this branch** | `0x80803020` (requires 0xD6; result helper) |

Advance helper: `func_808005FC_chflysaucer`, `RecompiledFuncs/funcs_20.c:2015`, original
`0x80800614..0x80800710`. Every store goes through `func_8080003C_chflysaucer`; native persistent-between-segments
field and actor extension byte remain in sync. Above table describes direct helper behavior, not proof that all
possible callbacks execute in that order for every replay/abort.

Initializer: `func_80802ED8_chflysaucer`, `RecompiledFuncs/funcs_121.c:371`, original
`0x80802F54..0x80803138`. Route 6's table target is `0x80803020`, which checks map 0xD6 and calls
`func_80801034_chflysaucer` (actor position initialization), then `func_80801130_chflysaucer`. The latter calls
`func_80800E48_chflysaucer` and chooses native score/reward/dialogue paths, including text IDs `0x10C6`, `0x10C7`,
`0x10E2`, `0x10E3`, `0x10E4` and score-item `0x8D` updates. Exact wording/meaning of each text is not required
or claimed. These callbacks still have work to execute after returning to the shared map.

Result anchors: `RecompiledFuncs/funcs_195.c:1539` and `:2720`; `funcs_69.c:1969`. An event callback path in
`func_8080185C_chflysaucer`, original `0x80801A38..0x80801A48` (`funcs_38.c:1482`), invokes the advance helper,
then updates its own controller subphase. Therefore clearing route 6 to 0 can happen **without** another map or
player creation. Route zero is not authorization to spawn J2 into an already-created room.

Three separate state dimensions must not be conflated:

```text
Actor+0x72 >> 10             actor state (func_80801568_chflysaucer)
controller extension +0x25   controller subphase (func_808012A4_chflysaucer)
controller extension +0x26   route selector, mirrored from native field (0xA34, 3 bits)
```

`func_808012A4_chflysaucer` reads/writes extension `+0x25` (`RecompiledFuncs/funcs_68.c:4835`); do not interpret
its numeric arguments as route values or native actor states. Likewise, route selector 6 is not actor state 6.

Creation adapter contract (`PROPOSED`):

1. At the existing creation boundary, current map already names the destination. After checking relevant
   option/unlock/count/native-MP gates, if destination is 0xD6, obtain route with original resident
   `func_800DA564(0xA34,3)` in an isolated copied `recomp_context` and proper guest-stack space. Retain original
   caller arguments/return context; classification must not overwrite requested count or native registers.
2. `func_800DA564` is a read-only multibit getter (`decomp/src/core2/1EB3750.c:219..247`). For 0xA34 it reads
   `D_8012C770[2]` via the native bitfield API, not the global replay availability bank. Do not hardcode a raw
   byte address or depend on the Saucer actor/overlay being initialized: player creation precedes that initialization.
3. Supply the scalar to `decide_creation`; block only the proven `0xD6 && route==6` context in addition to the
   immutable rooms. No per-frame scans, overlay-specific runtime callback replacement, flag writes or duplicate
   host itinerary state. Native MP and original counts remain authoritative.
4. Keep the pair absent for that creation lifetime. Native callbacks may clear the route later; next eligible
   native recreation restores co-op. The same map ID does not mean the same creation lifetime.

`CORRECTED/SUPERSEDED`: waiting for live evidence before even planning the shared-map guard would leave a known
result path uncovered. Static control flow is sufficient to require this guard; live/manual behavior, all abort
variants, and UI/camera correctness remain `NOT_RUN`/`UNKNOWN` until implementation and user validation.

## 13. Open questions and falsifiers

| ID | Unknown / risk | Smallest useful discriminator | Implementation consequence |
| --- | --- | --- | --- |
| U01 | Saucer full live route and abort variants; native route-6 return/results now statically resolved | Trace map creation/selector changes through result dialogue, clear-to-zero and later recreation | Implement proven 0xD6/route-6 guard now; extend only additional demonstrated escapes |
| U02 | Reachability of Mayan aliases 0xCA..0xCE | Inspect native replay/transition table references or observe legitimate load | Keep conservative no-injection policy and explicit UNKNOWN coverage; do not invent stages |
| U03 | Individual replay native subtype/count | Observe original create request and subtype without changing them | Preserve native 0x1B even with one human; never coerce count/viewports |
| U04 | Live viewport restoration/event input consumption; polling lifetime is statically resolved separately | Real layout setup plus persistent two-port input through clean challenge entry | Do not treat two polled ports as failed suspension or force channel count 1; isolate any demonstrated event-specific input consumer |
| U05 | J2 form continuity while absent | Production teardown -> challenge single owner -> exploration allocate sequence | Preserve existing value handoff; no guest pointers or native legality bypass |
| U06 | Native auxiliary slot mistaken for story J2 | Fixture with option On and non-host second owner | Require committed pair identity in every co-op extension |
| U07 | Host input publication/held chord across suspension | SDL/host input fixture plus clean retry/return | Clear/drain only stale owner state; preserve J1/native input |
| U08 | Event-only rather than room-wide suspension desired later | User changes requirement explicitly | New scoped design; do not retrofit mid-map freezing under this plan |
| U09 | Requested count vs native descriptor/fallback allocation | Execute real creation across each skip predicate and reentrant activity query | Commit only promoted owners; private lighting and provisional activity have distinct gates |
| U10 | Retirement/address reuse within one map | Real teardown hook -> PlayerState free -> same-address replacement | Preserve outgoing value serialization, retire identity, never adopt replacement by pointer equality alone |
| U11 | Saved host bank vs native challenge-required form | Native entry changes bank between outgoing teardown and new allocation/setup | Separate selector availability from cache application only if demonstrated; native entry wins |
| U12 | Native serialization after first owner retirement | Execute real ascending destruction and native activity-gated teardown | Preserve J2 handoff in an owner-specific serialization scope without reactivating gameplay |

The existence of these unknowns is not permission to skip a required stage or report it passing. Static
investigation supports a concrete host implementation, but release acceptance requires the independent
production-harness/build/manual validation described above.

## 14. Historical status at document delivery

- Catalog: 15 family sections, 23 stage records, exact consecutive replay flag identities verified.
- All main family descriptor getters and callback pointers resolved from verified ROM/metadata; source anchors retained.
- Saucer transition-selector table decoded; complete live ride and abort timing NOT_RUN.
- Production implementation: NOT_IMPLEMENTED. Game executable unchanged.
- Behavioral tests, build and game playtest for this change: NOT_RUN (document-only task).
- Initial-delivery document validation executed successfully (before the review additions below): 15 family sections, 23 stage/flag records matching native enum,
  23 unique primary rooms, 133 native symbol identities, 115 exact generated source spans, five UTF-8/frontmatter/
  whitespace/fence documents, 121 wiki links and four Markdown links. `markdown_vault_audit` was unavailable;
  an explicit local validator performed these checks. This is document validation, not behavioral test coverage.
- Baseline executable SHA-256 remained `305bf03074a261186fc87c44187532b41b617305a03d1bd41964e2b3cecefd85`.
- NEXT_SINGLE_ACTION: implement P1 (pure room/catalog decision and its independent guard matrix), then P2 in order.

### 14.1 Source review after initial delivery

Review scope: native player creation/load/teardown plus host forms, cheats, analog input and publication. The
15-family/23-stage catalog remains unchanged; no new event or speculative map blacklist was added.

Material corrections integrated into normative sections 6.3..6.8, change map and validation matrix:

- Native requested count can be bypassed; descriptor/fallback allocation requires provenance, not pointer counting.
- Native creation queries activity before return; committed-only gating would change initialization.
- Retirement can occur before the next map creation; address reuse must not inherit host participation.
- Existing menu/request resets have different edge/drain semantics; neutral rearming must preserve native input.
- Analog reservation is not currently reset at every map creation; the three known special-room omissions need
  explicit handling without globally disabling single-player camera control.
- Saved form-bank precedence needs a native-entry discriminator; preserving the J1 menu does not imply applying
  a stale cache over an event-required form.
- Ordinary load layout ordering and destination-map publication now have a direct native call-chain anchor.

All above are static source/MIPS conclusions or explicitly marked proposed adjustments. No implementation,
behavioral regression, build or live minigame traversal ran during this review. The initial-delivery validation
counts are historical document checks, not a claim that new behavioral cases have passed.

Review document audit: 15 family sections, 23 stage flags checked against the native enum, 23 unique primary
rooms, 146 native symbols and 115 generated source spans checked; five documents, 121 wiki links and four
Markdown links checked. Corrected two span endpoints that used the line after EOF instead of the inclusive
last line. `markdown_vault_audit` unavailable; explicit read-only local validator used. Game executable SHA-256
still matches the baseline above. Document checks do not prove gameplay.

### 14.2 Follow-up source review: event return and outgoing serialization

Two material gaps were resolved beyond the preceding lifecycle review:

1. Saucer's shared Witchyworld return is **not** an unproven optional extension anymore. ROM jump tables
   `+0x3E60`/`+0x4214`, the native result helper and clear-without-goto branch establish a route-6 result context.
   Pure policy/input adapter, negative controls, per-event instructions and integration cases now require this
   guard. Native field read replaces the previously suggested conditional host itinerary token for this path.
   Full live traversal and all abort paths remain unverified.
2. Keeping J2's serialization identity after J1 retires does not by itself execute native handoff: the original
   teardown has its own activity gate. Section 6.9 now requires a restricted outgoing-owner activity scope and
   a real-native ascending destruction regression. Gameplay availability stays false during this exception.

Native actor state, Saucer controller subphase and route are documented separately to prevent numeric-state
confusion during implementation. No game code, generated source or executable changed; no build/gameplay ran.

Latest document audit: 15 family sections, 23 native replay-flag records, 23 unique unconditional primary rooms
(shared 0xD6 context tested separately), 152 native symbol identities, 115 generated spans, five documents,
121 wiki links and four Markdown links checked. Three Saucer tables (20 entries) rechecked from the verified
ROM. `git diff --check` passed. These are static/document checks; new behavioral regressions remain NOT_RUN.

### 14.3 Follow-up source review: layout versus polling lifetime

Corrected an ambiguity in section 6.4/U04: native map initialization restores viewport topology, but does not
necessarily restore controller polling to one port. Native direct setters are in game selection; runtime channel
count persists until explicitly changed or initialized. Ports beyond that count retain their guest pad bytes.
Section 6.10 and integration case 19 now distinguish these contracts and prohibit a synthetic map-reset fixture
or channel reduction as a shortcut. Existing per-owner host admission/rearming remains required with port 2 polled.

Evidence is static source/MIPS and a direct-call census, not a live 23-event input audit. No game/runtime code,
generated output or executable changed; behavioral regressions/build/playtest remain NOT_RUN.

Document audit after this correction: 15 family sections, 23 stage/flag records matching the native enum,
23 unique unconditional primary rooms, 158 native symbol identities, 115 full-function generated spans,
five UTF-8/frontmatter/whitespace/fence documents, 121 wiki links and four Markdown links passed the explicit
local validator (exit 0). New layout/setter/port-adapter anchors were inspected directly; four native direct
setter callsites were enumerated separately. `markdown_vault_audit` remains unavailable. `git diff --check`
passed; executable SHA-256 still matches section 14's baseline. These checks do not establish runtime acceptance.

### 14.4 Follow-up source review: replay return and reference precision

Mini-Sub's native result-choice dispatch is now documented separately for story and replay (section 11.7,
integration case 20). The replay path bypasses the local retry/exterior map-goto branches; do not restore the
host pair from a generic result callback. No additional unconditional room or shared-map exclusion was established.

The previous range audit checked existence/start/return but did not enforce an inclusive end at the function's
own closing line. Corrected 113 reference occurrences (112 unique ranges) which included the first line of the
next function. All 115 full-function reference occurrences must now start at their named definition, contain
exactly one definition, and end at its closing line. The two earlier EOF corrections remain valid. Historical
audit statements above describe the weaker checks actually run then; they do not establish strict end precision.

No gameplay implementation, generated-source change, build or playtest was performed. Native frontend helper
semantics beyond the observed dispatch remain UNKNOWN; production regressions remain NOT_IMPLEMENTED/NOT_RUN.

Current document checks (exit 0): strict inclusive boundaries passed for all 115 full-function references;
15 families, 23 stage flags, 23 unique primary rooms, 158 native symbols, five documents, 121 wiki links and
four Markdown links also passed the existing local validator. Executable hash remains the baseline value.
`git diff --check` passed. No additional runtime coverage is implied.

## 15. Implementation checkpoint — 2026-11-02

**Current authority:** production host implementation is present in this working tree. The historical baseline and
PROPOSED pseudocode above are retained for traceability until manual acceptance. No generated/vendor source was edited.
No commit or push was requested for this implementation checkpoint. Saved configs, ROMs/assets and controller assignments
were not edited. No game process was started or controlled by the agent.

### 15.1 Implemented ownership and policy

- `include/tooie_story_coop.h`: constexpr stage/room catalog and creation policy. Native subtype/count precedence,
  original scene exclusions, conservative aliases, related MP rooms and the conditional Saucer return are explicit.
- `src/game/tooie_story_coop.cpp`: game-thread construction provenance, committed owner identity, retirement and
  restricted outgoing serialization. `created_pair` is existence; `running` adds the saved/current preference gate.
  The eight-slot native table remains native; only promoted owners 0/1 can acquire story co-op identity.
- `tooie_dev_warp_hook.cpp`: native original is called exactly once; only eligible promotion adds the existing
  viewport/channel helpers. Native route read uses a copied context and guest stack. The registered normal-allocation
  wrapper records publication after original `func_800F61E4`. Two valid normal allocations are required to commit;
  a skipped loop with descriptor owners or one fallback does not commit. Reentrant activity is available before
  outer return only for the verified provisional J2. Placement/release/Warp cannot target a native auxiliary.
- Native player destruction still owns free/layout/roles. `tooie_forms_on_teardown` captures values after native work,
  then retires host identity before the caller frees PlayerState. J1 retirement disables gameplay but preserves the
  still-live outgoing J2 identity for its synchronous serialization scope. Full reset clears that identity/cache.
- AI, Glowbo cleanup, retained travel UI and nearest-J2 object distance require `running`. Forms, input owner binding
  and private lighting distinguish owner existence from an option switched Off in an already-created room.
- J1 selector availability retains the original scene policy. It is not disabled merely because a minigame declined
  co-op. Auxiliary slot/camera 1 cannot use J2 form cache or menu. Forms resolve table identity, not camera-as-player.
- Input publishes native controller bindings atomically. A per-port owner epoch handles unpolled absence and return;
  it is a correctness token, not a measurement counter. Main input and boundary cancellation share `shortcut_mutex`,
  preventing requests admitted before reset from landing after cancellation. The input callback reads no guest pointers.
- Neutral rearming requires released Debug/R3, D-pad, A/B and neutral navigation. It suppresses host shortcuts only;
  unavailable menus clear pending open/drain without swallowing native buttons/stick. Physical connectivity and native
  polling are unchanged. Native J1 cheat behavior remains available; native/auxiliary J2 is not promoted to host J2.
- Analog Camera resets samples/reservation/smoothing at recreation and recognized retirement, independently of saved
  camera settings. Known Mini-Sub/Saucer room omissions are closed; native auxiliary cameras cannot inherit J2 control.
- Existing Dev Mode option explains automatic challenge suspension. No extra user setting or log/metric was introduced.

### 15.2 Corrections/refinements established during implementation

1. **J1 transition precedence:** old `TransitionForm` replayed the saved bank over a native challenge entry. The regression
   seeds cached Bee, changes the native entry to Submarine and requires Submarine to win. J1 now always reads the native
   handoff bank, which already contains J1 values after isolated outgoing serialization. J2 alone installs its saved bank
   in its verified allocation/setup scope. Dragon appearance is invalidated on challenge/native bank change, while normal
   same-bank continuity remains per-owner. This separates menu availability from handoff override as section 6.8 required.
2. **Existence versus preference:** private model vertices and legitimate already-created form/menu owners must not lose
   isolation just because `two_players` was switched Off without destroying J2. `created_pair` and `running` are separate
   production APIs; the regression checks private vertices after Off. Collision/AI/Warp retain their option gate.
3. **Input cancellation needs serialization:** clearing atomics alone leaves an enqueue/reset race. A narrow host mutex
   surrounds admission and boundary cancellation, but never native player construction/update/teardown. Tokens additionally
   require neutral on a new ownership cycle even when port 2 received no samples during suspension.
4. **Camera index is not a player:** forms use actual table/verified identity and a scoped prepublication allocation index.
   A nested auxiliary call cannot inherit an outer ordinary PlayerScope as its identity.

These are implementation refinements to the plan, not claims that all game events were played. Shared-map Saucer
result-camera behavior and every abort path still require user validation; no blanket Witchyworld camera exclusion was added.

### 15.3 Automated validation and proof boundary

Production target `tooie_story_coop_test` is `EXCLUDE_FROM_ALL`, required by Windows and Linux test lists. It executes
real generated creation, normal/descriptor allocation, pre-return activity, native layout, ascending destruction,
teardown/activity/Goliath reversion and Mini-Sub story/replay result control flow. Synthetic leaves provide assets,
model instances, world/subsystem services, device/channel endpoints and final frontend/goto actions. It does not render,
load a real map or establish scoring/contact/physics correctness in all events. Delayed J2 release uses the production
clock in the harness. Source registration is checked through the real boot lookup in `bcf28_helpers_test`.

Latest Windows test command (exit 0):

```powershell
rtk proxy pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Test -Jobs 12
```

Result: **30/30 required targets PASS**; `tooie_story_coop_test` **9339 checks PASS**; Analog Camera **83 checks PASS**.
Existing subsystem fixtures now explicitly establish allocation provenance instead of treating the saved option as
proof of a pair. This retains the production gate in those regressions.

Integration helpers were built in the same VS developer environment with `cmake --build build-win --target
bcf28_helpers_test core1_ra_wrappers_test stick_angle_thunk_test -j 12`, then each `build-win/<target>.exe` executed.
All three passed (exit 0), including the new actual `0x800F61E4` lookup binding. Build command/result and final executable
hash are recorded below after the final link. No new instrumentation exists in the game executable; counters/logs
used by regressions belong to their test executables only.

Manual MG01..MG23 matrix: **NOT_RUN**. Real native input polling through all events, shared-map result camera,
all win/loss/abort/replay routes, visual viewport restoration and universal form legality remain **NOT_RUN/UNKNOWN**.
Native Linux test/build: **NOT_RUN**. Do not promote these cells from synthetic policy/control-flow tests.

`NEXT_SINGLE_ACTION`: user executes section 9.5 beginning with a normal story entrance into one available minigame,
then retry and exit, checking J2 absence/native controls and restoration at the next eligible load. Retain this plan
until manual acceptance; afterward migrate any remaining stable knowledge, remove the plan and commit only on request.

Final game build: `rtk proxy pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Build -Jobs 12`
completed with exit 0. Updated artifact: `E:\projetos\TooieRecomp-win\build-win\TooieRecompiled.exe`.
SHA-256: `31adba3723e973bd3760a66e60a690616847c2c7e02c5b36876de74f2d6dd0be`. Baseline HEAD remains `034189002f15d9b6dfeece588da71d93a9bca48e`;
implementation is uncommitted. Root plan remains intentionally present for pending manual acceptance.

Final documentation checks (exit 0): 8 UTF-8/frontmatter/whitespace/fence documents, 127 wiki links,
15 relative Markdown links, 115 strict inclusive native function references and 23 stage records.
`markdown_vault_audit` unavailable; explicit local validator used. `git diff --check` passed.
Generated/vendor paths are unchanged; no files staged. Existing dirty investigation documents were preserved.

## 16. User checkpoint and deferred limitation — 2026-11-02

This section supersedes section 15 delivery status about no manual test/commit request. User tested the game,
reported apparent success in some unspecified events, and reported J2 remains accessible during the train/cart race
with the bird. User explicitly requested retaining current behavior, documenting the limitation, reviewing and committing.

- Runtime status: PARTIAL_USER_TESTED_WITH_KNOWN_LIMITATION. No specific MG01..MG23 cell can be marked PASS because
  the successful events were not individually identified. No map/controller/subtype sample was captured for the race;
  its formal identity and native event path remain UNKNOWN. The report itself is PROVEN / USER_REPORT.
- Existing policy remains unchanged: native recreation/room-based suspension plus conditional Saucer return. It does
  not detect every live event, especially activities started inside a populated exploration map. This is a compatible
  explanation, not a proven runtime trace of the reported race. Do not add a blanket exploration-map exclusion.
- The reported race is a documented deferred limitation, not a fix included in this commit. Native controls and
  the tested current game behavior remain unchanged during this review. No temporary runtime diagnostics are added.
- Stable documentation: `docs/reverse-engineering/systems/minigames.md`,
  `docs/specs/Tooie/window-ui-and-config.md` and `docs/HANDOFF.md` record the limit and evidence boundary.
- Keep this root plan as the requested technical reference while acceptance remains partial; do not treat it as an
  active request to continue fixing events. Remove it after future complete acceptance and migration, as originally planned.
- NEXT_SINGLE_ACTION (only if later requested): identify the native controller and event lifecycle for the reported
  train/cart race before designing event-level suspension. No further gameplay change is authorized by this checkpoint.

Review/commit validation is recorded in the commit message. Prior build/hash and source evidence remain in section 15.
