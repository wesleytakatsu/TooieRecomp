---
title: "Minigame controllers and native event flow"
description: "Verified descriptor/callback identities and native state/route evidence preserved from the retired minigame implementation plan."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-02
tags: [reverse-engineering, minigames, controllers, callbacks]
---

# Minigame controllers and native event flow

Knowledge consolidated from the static analysis committed in `50bd9a2bc86810e1c16aec3094cb8565a6562f9f`.
`PROVEN / STATIC_MIPS` and `STATIC_ROM` labels retain their original proof boundary; no new live event traversal
or ROM-table capture was performed during this documentation migration. Baseline source spans refer to the generated
output audited there, which this maintenance does not edit. Unknown semantic identities remain `UNKNOWN`.

This page documents native mechanisms. Current host policy: [[minigames]] and
[[specs/Tooie/window-ui-and-config]]. Manual acceptance: [[specs/Tooie/testing-and-oracle]].
Historical pseudocode, proposed fixes and delivery logs remain in Git history, not in this current RE contract.

## Event controllers

### Mayan Kickball

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

**Unresolved boundaries:** Reachability/meaning of 0xCA..0xCE and all replay entrance combinations remain UNKNOWN. No normal-story visit to those aliases is claimed.

**Additional primary anchors:**

- `func_808046C4_chstoneballcontrol` — `RecompiledFuncs/funcs_244.c:L4018-L4237`.
- `func_80801944_chstoneballcontrol` — `RecompiledFuncs/funcs_187.c:L2088-L2429`.
- `func_80800908_chstoneballcontrol` — `RecompiledFuncs/funcs_409.c:L2550-L3023`.

### Ordnance Storage

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

**Unresolved boundaries:** Full mapping of gcstatus session subtype values to every Ordnance replay variant remains UNKNOWN; never infer story solely from native player count 1.

**Additional primary anchors:**

- `func_80800E64_chtntmineshoot` — `RecompiledFuncs/funcs_22.c:L3076-L3653`.
- `func_80801A54_chtntmineshoot` — `RecompiledFuncs/funcs_297.c:L1861-L2153`.

### Dodgem Challenge

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

**Unresolved boundaries:** All state-number-to-dialogue meanings are not assigned here; state setter parameter forwarding is proven, not a complete semantic enumeration.

**Additional primary anchors:**

- `func_808006CC_chdodgemcontrol` — `RecompiledFuncs/funcs_355.c:L284-L786`.
- `func_8080422C_chdodgemcontrol` — `RecompiledFuncs/funcs_193.c:L2851-L2953`.

### Hoop Hurry

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

**Unresolved boundaries:** The exact full lifecycle of the related variant descriptor is not established by the getter alone; replay room assignment must be observed, not guessed.

**Additional primary anchors:**

- `func_808004BC_chhoopswoop` — `RecompiledFuncs/funcs_52.c:L5600-L5874`.
- `func_80800910_chhoopswoop` — `RecompiledFuncs/funcs_170.c:L1590-L2215`.

### Balloon Burst

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

**Unresolved boundaries:** No complete semantic naming of all balloon actor states is claimed; no native callback modification is proposed.

**Additional primary anchors:**

- `func_80800790_chballoonburst` — `RecompiledFuncs/funcs_273.c:L2015-L2276`.
- `func_808003C4_chballoonburst` — `RecompiledFuncs/funcs_401.c:L530-L1089`.

### Saucer of Peril

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

**Unresolved boundaries:** Complete clean runtime traversal, all abort variants and native field 0xA34 reset paths outside the verified advance helper remain UNKNOWN. Selector 6 is now proven to initialize a return/result context; it is not proof that the event has already finished. sucoasterflydata contains prop selectors rather than obvious map IDs; do not import that table as a map itinerary by name alone.

**Additional primary anchors:**

- `func_80800514_chflysaucer` — `RecompiledFuncs/funcs_339.c:L87-L274`.
- `func_80800000_chflysaucer` — `RecompiledFuncs/funcs_212.c:L4849-L4892`.
- `func_8080003C_chflysaucer` — `RecompiledFuncs/funcs_248.c:L3965-L4012`.
- `func_80800E48_chflysaucer` — `RecompiledFuncs/funcs_69.c:L1969-L2260`.
- `func_80801568_chflysaucer` — `RecompiledFuncs/funcs_91.c:L921-L1435`.

### Mini-Sub Challenge

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

**Unresolved boundaries:** Exact semantic labels for every event payload/result choice and all replay entrance variants remain UNKNOWN. No restoration based on merely observing 0x1A9 as an immediate constant is acceptable.

**Additional primary anchors:**

- `func_80800BA8_chminisubgame` — `RecompiledFuncs/funcs_158.c:L5030-L5685`.
- Replay resident thunks: `RecompiledFuncs/funcs_75.c:951..959`, `funcs_310.c:128..136`.

### Chompa's Belly

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

**Unresolved boundaries:** Not all target variant semantics/colors were decoded; that knowledge is not required for room-wide suspension. Do not assign colors solely from descriptor IDs.

**Additional primary anchors:**

- `func_8080078C_chmonsterinterior` — `RecompiledFuncs/funcs_234.c:L6863-L7343`.
- `func_80800C38_chmonsterinterior` — `RecompiledFuncs/funcs_37.c:L805-L1092`.

### Clinker's Cavern

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

**Unresolved boundaries:** All gcstatus FPS subtype-to-map values and exact exit/replay variants remain UNKNOWN; the room decision does not require guessing them.

**Additional primary anchors:**

- `func_808008E0_chclinker` — `RecompiledFuncs/funcs_391.c:L1217-L1855`.
- `func_8080100C_chclinker` — `RecompiledFuncs/funcs_195.c:L980-L1218`.
- `func_808011A8_chclinker` — `RecompiledFuncs/funcs_258.c:L2220-L2483`.

### Twinkly Packing

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

**Unresolved boundaries:** The entire physical room remains blocked under this design even when no challenge is active. If that UX is rejected later, a separate safe mid-room suspension design is required.

**Additional primary anchors:**

- `func_80800748_chfactorytwinklies` — `RecompiledFuncs/funcs_181.c:L1262-L1808`.
- `func_80800B80_chfactorytwinklies` — `RecompiledFuncs/funcs_406.c:L5485-L5786`.

### Colosseum Kickball

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

**Unresolved boundaries:** Detailed role of every secondary descriptor and complete round-selector field mapping remain UNKNOWN; primary arena identities come from maps.h and the family itself is separate from Mayan.

**Additional primary anchors:**

- `func_80801764_chcollballcontrol` — `RecompiledFuncs/funcs_294.c:L1814-L2155`.
- `func_808008FC_chcollballcontrol` — `RecompiledFuncs/funcs_401.c:L4162-L4653`.

### Pot O' Gold

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

**Unresolved boundaries:** A separate native MP Pot room has not been established. Do not invent one or infer it from a numeric constant passed to an unrelated service.

**Additional primary anchors:**

- `func_80800434_chpotogoldgame` — `RecompiledFuncs/funcs_119.c:L3886-L5308`.
- `chpotogoldgame_entrypoint_0` — `RecompiledFuncs/funcs_177.c:L218-L297`.

### Trash Can Germs

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

**Unresolved boundaries:** All main/variant numeric phase semantics and every exit destination are not exhaustively decoded; room-wide suspension avoids coupling to those unknowns.

**Additional primary anchors:**

- `func_808003D0_chtrashcangame` — `RecompiledFuncs/funcs_186.c:L2643-L3342`.
- `func_80800728_chtrashcangame` — `RecompiledFuncs/funcs_81.c:L297-L739`.

### Zubbas' Hive

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

**Unresolved boundaries:** Which result/choice values select each destination and all secondary actor behavior labels remain UNKNOWN; no direct persistent flag write is proposed.

**Additional primary anchors:**

- `func_80800018_chwaspnestgame` — `RecompiledFuncs/funcs_355.c:L2955-L3529`.
- `func_80800F98_chwaspnestgame` — `RecompiledFuncs/funcs_241.c:L1245-L1797`.

### Tower of Tragedy

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

**Unresolved boundaries:** Full quiz global struct/round-index offsets and all presenter/participant descriptor meanings remain UNKNOWN. They are not required to keep one room suspended across three rounds.

**Additional primary anchors:**

- `func_808058F0_chquiz` — `RecompiledFuncs/funcs_113.c:L5705-L8485`.
- `func_80805404_chquiz` — `RecompiledFuncs/funcs_138.c:L297-L1396`.
- `func_80803BFC_chquiz` — `RecompiledFuncs/funcs_241.c:L4732-L5679`.

## Shared native machinery

### Saucer transition-selector table (PROVEN / STATIC_MIPS + STATIC_ROM)

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
handles `0xD6` as well as the dedicated rooms; the route advancement/return subsection below resolves the selector-6 return/result branch.
Do not blacklist all of `0xD6`.

### FPS controller

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

### Rejected identity shortcuts

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

### Saucer route advancement and return/result initialization

`PROVEN / STATIC_ROM + STATIC_MIPS`: two additional native jump tables were verified against the same canonical
decompressed ROM and section 1692 base documented in the canonical asset manifest. Combined with the existing goto table, they resolve the
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
