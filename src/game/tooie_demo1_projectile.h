#pragma once
// TOOIE_DIAG_DEMO1_PROJECTILE helpers (USER_DEMO1 first fire projectile chain, same-phase pair).
// Mirrors P08_DEMO1_PROJECTILE in tools/p08_attract_egg_hook.h (tests/tooie_demo1_projectile_test.cpp
// compares header/format/window with the hook text). Events (all raw word reads, no inferred meaning
// beyond the MIPS-documented roles listed in the report):
//   baeggsetup5   entry 0x80084A60 (_baeggsetup_entrypoint_5, a0=player a1=type byte a2 a3 floats-as-raw)
//   cheggfire2    entry 0x800871F0 (_cheggfire_entrypoint_2, v0 = spawned object)
//   sueggmove9    entry 0x8008A480 (_sueggmove_entrypoint_9, a0 = object)
//   suegg9        entry 0x8008A3C0 (_suegg_entrypoint_9, a0 = object)
//   sweep         entry 0x800CD820 (func_800CD820, v0 = hit object or 0), filtered to a3==0x42180000
//                 and a2-4 == a tracked spawned object
//   alias/untracked: object relocation candidate rows (same owner, new pointer) and capped untracked
//                 sueggmove9/sweep38 rows (obj=-1)
//   Payload contract: every non-mark row has w11 = raw bits of global 0x8012C760 (gs at the callback,
//   f11 = same as float). Object layout w10 = owner word *(obj+0). FINISH: w0=rows, w1=unmatched pending,
//   w2=alias events, w3=untracked rows, w4=retry entries deduped.
//   s_BE444 s_E9E88 s_E9EB4 s_106790 s_8A018 s_EEB40 s_EC3C4 s_CD6B4 s_CB854 (enter/leave) + s_cand rows:
//                 helpers called by func_800CD820 (800BE444 cell-list array, 800E9E88/800E9EB4 list begin/end,
//                 80106790 actor->component, 8008A018 candidate pos+radius, 800EEB40 squared distance,
//                 800EC3C4 actor descriptor, 800CD6B4 narrow phase, 800CB854 layer id). Payloads per kind are
//                 defined in boot_hw_stubs.cpp (tooie_d1p_hp) and the hook (p08_d1p_hp); float returns are
//                 raw $f0 bits. s_cand (ph=entry): one row per list entry between E9E88 start and E9EB4 end:
//                 a0=actor a1=entry ptr a2=index, w0=*(entry+8) w1=*(actor+0x28) w2=lhu(actor+0x14)
//                 w3=lhu(actor+0x18) w4=(actor==sweep a0 ignore) w5..7=actor+4..0xC raw.
//   Target prefix rows (own flag): tgtfound (once; header a0=marker a1=idx a2=comp a3=vec ptr; w0=lhu(marker+0x1A)
//                 w1=*(marker+0x28) w2=*(marker+0x10) w3=(lhu14<<16|lhu18) w4=*(marker+0) w5=*(*(marker)+8) w6=idx)
//                 tgtambig (ph=mark2, same layout as tgtfound, max 8) for a DIFFERENT handle matching the selector, never rebinds;
//                 and tgtrec (once per post 1..356: header a0=comp record a1=idx a2=vec begin a3=elem size v0=marker;
//                 w0..2=comp+4/+8/+0xC raw, w3=+0x10 w4=+0x14 w5=+0x18 w6=+0x1C w7=+0x20 w8=+0x28 w9=+0x6C w10=+0x70
//                 w11=gs). tgtfinish (w0=rows, w1=first-seen post, w2=ambiguity count, w3=distinct matching handles).
//   START/FINISH  markers emitted from the func_800151BC entry hook (post>=306 / post>390)
#include <cstdint>

namespace tooie_demo1_projectile {

constexpr int kMode = 5;
constexpr int kStage = 0;
constexpr int kSub = 0;
constexpr int kPostLo = 306;
constexpr int kPostHi = 390;
constexpr int kMaxRows = 600;
constexpr int kMaxObjs = 8;
constexpr uint32_t kSweepRadiusBits = 0x42180000u;  // 38.0f passed by suegg_entrypoint_9

// First-branch trace of func_800CD820 (tracked fire object only, post window below): helper calls made
// from inside the query (via LOOKUP_FUNC, hooked identically by PC in the oracle) are logged as s_* rows.
constexpr int kSweepTraceLo = 354;
constexpr int kSweepTraceHi = 355;
constexpr int kMaxTraceRows = 300;
constexpr int kMaxCandEntries = 12;
constexpr bool in_sweep_trace(int post) { return post >= kSweepTraceLo && post <= kSweepTraceHi; }

// Target producer prefix (TOOIE_DIAG_DEMO1_TARGET / P08_DEMO1_TARGET, separate flag + CSV, same 37 columns).
// Source: func_80106790(actor) = vector_at(*0x80136EE0, lhu(actor+0x1A)>>5) = *(vec+4) + idx*(*vec) (0x9C-byte
// actor record); _subaddiecustomhits_entrypoint_5 (overlay ep5 @0x8080029C) reads its center from that record
// (+4 vec) when *(*(marker)+8) bit1 is set (otherwise func_800EC75C path). The marker is recognised by
// id halfwords lhu(+0x14)=0x0671 and lhu(+0x18)=0x0E3F at the func_80106790 call (no actor scan).
constexpr int kTgtPostLo = 1;
constexpr int kTgtPostHi = 356;   // consumed 0..355
constexpr int kTgtMaxRows = 500;
constexpr uint32_t kTgtIdA = 0x0671u;  // lhu(marker+0x14). Class meaning/uniqueness UNPROVEN (not an actor id claim).
// Selector = id halfword 0x0671 at +0x14 AND bit0 of lhu(+0x18) (func_800CD820 predicate P6 tests that bit; the
// remaining +0x18 bits are mutable state and observed to differ between runtimes: 0x0E21 in both PAR3 captures,
// 0x0E3F only later at the oracle hit). +0x28 flag bits are state as well and are logged, never required.
constexpr bool is_target_marker(uint32_t lhu14, uint32_t lhu18) { return lhu14 == kTgtIdA && (lhu18 & 1u) != 0; }
// Binds the first match; a DIFFERENT handle that also matches is counted as ambiguity and never rebinds.
struct TargetSelector {
    uint32_t marker = 0;
    uint32_t handles[8] = {};
    int nhandles = 0;      // distinct matching handles seen
    int ambiguous = 0;     // matches by a handle other than the bound one
    // returns 1 = newly bound, 2 = ambiguity (new distinct handle), 0 = no match / already bound handle
    int feed(uint32_t handle, uint32_t lhu14, uint32_t lhu18) {
        if (!is_target_marker(lhu14, lhu18)) return 0;
        for (int i = 0; i < nhandles; ++i) {
            if (handles[i] == handle) return 0;
        }
        if (nhandles < 8) handles[nhandles++] = handle;
        if (marker == 0) {
            marker = handle;
            return 1;
        }
        ++ambiguous;
        return 2;
    }
};
constexpr bool in_tgt_window(int mode, int stage, int sub, int post) {
    return mode == kMode && stage == kStage && sub == kSub && post >= kTgtPostLo && post <= kTgtPostHi;
}
// First-writer prefix (same flag as the target prefix, rows "updrec", default OFF). Source (PROVEN MIPS): the per-frame
// actor loop func_800FF62C (frame 0x58) calls func_80100368(actor) (= jalr *(actor+0x10), returns the class
// descriptor) at exactly two jal sites, 0x800FF6D4 and 0x800FF710 (ra 0x800FF6DC / 0x800FF718), then jalr *(desc+0xC)
// (or *(desc+0x10) when actor+0x74 bit4). The generated function never assigns r31 before those calls (its only r31
// assignment is the epilogue restore), so a host-side ra test cannot identify them.
// DIRECT loop call (host) = func_80100368 entered while a func_800FF62C scope is active AND ctx SP == scope entry SP -
// 0x58 (a getter call nested inside the actor callback runs on a deeper SP). Oracle: ra in {0x800FF6DC, 0x800FF718}
// (same two jal sites; real MIPS ra). Scope nests/restores; ctx is never mutated.
// IDENTITY ROLES (ABI): MarkerHandle = the bound lhu(+0x14)==0x0671 handle passed to func_80106790 (s_d1t_marker, *NOT*
// an argument of func_80100368 in the actor loop). ActorRecordAddress = the 0x9C record = vector_at(*D_80136EE0,
// lhu(MarkerHandle+0x1A)>>5) = what func_800FF62C iterates and passes as a0 to func_80100368. The call arg is compared with
// the freshly resolved ActorRecordAddress (begin/stride re-read at every use: the record vector relocates), never with the
// MarkerHandle. The 39 dumped words are read from the ActorRecordAddress; the MarkerHandle is logged in w10 of every row.
// Phase contract: "pre" = dump at the direct getter call of the tracked record, i.e. BEFORE its update callback;
// "post" = dump at the next DIRECT loop getter call (next actor/loop) or at the logic entry (800151BC), whichever comes
// first (v0 = 1 getter, 2 logic entry). They bracket the tracked actor's update; the interval may include later actors'
// updates, so it is not a per-actor-only causal claim.
// updrec: ph=pre|post a0=ActorRecordAddress a1=chunk a2=RAW rng state D_8012C7F0 (not a drawn value) a3=ctx SP (host) /
// ra (oracle) v0=*(record+0x10) (pre) | flush source (post) w0..w9=record words [chunk*10, chunk*10+n) (4 chunks, 39
// words) w10=MarkerHandle. ph=postmiss (one row) = the MarkerHandle no longer resolves to a record (vector unusable or
// lhu(marker+0x14)!=0x0671): a0=0, w10=MarkerHandle, no memory is dumped.
constexpr int kUpdPostLo = 80;
constexpr int kUpdPostHi = 86;
constexpr int kUpdMaxRows = 80;  // 8 rows per update (pre 4 + post 4)
constexpr int kUpdRowsPerCall = 8;
constexpr uint32_t kUpdLoopFrame = 0x58u;
constexpr uint32_t kUpdRaSite0 = 0x800FF6DCu;  // oracle: return address of jal 0x800FF6D4
constexpr uint32_t kUpdRaSite1 = 0x800FF718u;  // oracle: return address of jal 0x800FF710
constexpr int kUpdRecWords = 39;  // 0x9C / 4
constexpr int kUpdChunkWords = 10;  // w10 = MarkerHandle, w11 of every non-"mark" row is overwritten with the game speed
constexpr int kUpdChunks = 4;
constexpr bool in_upd_window(int post) { return post >= kUpdPostLo && post <= kUpdPostHi; }
constexpr bool is_upd_oracle_ra(uint32_t ra) { return ra == kUpdRaSite0 || ra == kUpdRaSite1; }
constexpr int upd_chunk_words(int c) {
    return c < 0 || c >= kUpdChunks ? 0 : (kUpdRecWords - c * kUpdChunkWords < kUpdChunkWords ? kUpdRecWords - c * kUpdChunkWords : kUpdChunkWords);
}
// Host call-local scope around the original func_800FF62C (thread_local instance in the wrapper; saved/restored by it).
struct UpdLoopScope {
    bool active = false;
    uint32_t loop_sp = 0;
    static constexpr UpdLoopScope enter(uint32_t entry_sp) { return UpdLoopScope{true, entry_sp - kUpdLoopFrame}; }
    constexpr bool direct(uint32_t sp) const { return active && sp == loop_sp; }
};
// Accept = direct loop call whose a0 is the freshly resolved ActorRecordAddress (0 = unresolved => reject), inside the post
// window with row budget left.
constexpr bool upd_accept(const UpdLoopScope& sc, uint32_t sp, uint32_t record_arg, uint32_t expected_record, int post,
                          uint32_t rows) {
    return expected_record != 0 && record_arg == expected_record && sc.direct(sp) && in_upd_window(post) &&
           rows + uint32_t(kUpdRowsPerCall) <= uint32_t(kUpdMaxRows);
}
// RNG-consumer / CACEC-extra-accept trace (same TARGET flag, default OFF; rows "rngcall", "cacecx", "e3e8c"). Source facts:
// D_8012C7F0 is stepped by func_800DC0C0 (float result) and func_800DC090 (raw state in v0), LCG s' = s*1664525 + 1013904223
// (u32); both are reached only through LOOKUP_FUNC (coverage script). The ON-by-default object frustum feature
// (tooie_wrap_func_800CACEC) turns an original reject into an accept only while a draw scope is open.
// func_800E3E8C no longer opens that scope: func_800DE498 and every unclassified E3E8C caller keep the original v0.
// Scopes that still expand: func_8011447C (kind 2, preserved) and func_800B97A4 (kind 3, pure-draw allowlist below).
//   rngcall: ph=dc0c0|dc090 a0=state before a1=state after a2=caller origin a3=scope kind v0=result bits
//            w0=event seq w1=extra accepts so far w2=scope-entry origin w3=lcg(a0)==a1
//   cacecx : (host only) a0=volume a1=CACEC caller origin a2=radius bits a3=scope kind v0=1 w0..w2=pos bits w3=scope-entry
//            origin w4=event seq w5=rng state
//   e3e8c  : ph=enter(host leave) a0=pos ptr a1=radius bits a2=caller origin v0=final result w0=event seq w1=extra accepts
//            added inside this call
// Caller origin: host = (int32)(__builtin_return_address(0) - &tooie_d1x_anchor) (resolve offline with nm of the hashed
// binary; ctx->r31 is NOT used, it is unreliable); oracle = real MIPS ra. Scope kind: 0 none, 1 retired (E3E8C wrapper
// does not open it), 2 func_8011447C, 3 func_800B97A4. These rows do not count towards the 500-row tgtrec cap.
// Pure-draw allowlist. Guest VRAM identity, not a host PC. A display-list or matrix write is not SIM feedback.
// func_800B97A4 (LOOKUP at funcs_123.c, one site) accept path emits G_MTX 0xDA380002, G_DL 0xDE000000, G_MOVEMEM
// 0xD8380002 plus mlMtx* / guMtxF2L / osVirtualToPhysical. No 800DC090/800DC0C0 on that path. func_800B956C on v0!=0
// advances the model-node loop (L_800B975C) and skips 800B9108. func_800DDDC4 / func_800DDF14 / func_800DDFA0 are
// draw-shaped DE2A4 gates but have zero LOOKUP sites, so a wrapper would not run; they are not registered.
// func_800DE498 stores the widened v0 and is excluded. Unclassified E3E8C callers stay original.
constexpr uint32_t kCacecPureDrawB97A4 = 0x800B97A4u;
constexpr int kCacecScopeKindB97A4 = 3;
constexpr bool tooie_cacec_pure_draw_vram(uint32_t vram) { return vram == kCacecPureDrawB97A4; }
// enabled = production object-frustum flag. orig_v0 = original predicate. expanded_accepts = 1 if the wider sphere accepts.
// Logic/FX and any VRAM absent from the allowlist keep orig_v0 even when the wider frustum would accept.
constexpr uint32_t tooie_cacec_apply_v0(uint32_t caller_vram, uint32_t orig_v0, uint32_t expanded_accepts, bool enabled) {
    if (!enabled || orig_v0 != 0u || !tooie_cacec_pure_draw_vram(caller_vram) || expanded_accepts != 1u) return orig_v0;
    return 1u;
}
constexpr int kRngPostLo = 79;
constexpr int kRngPostHi = 84;
constexpr int kRngMaxRows = 300;
constexpr int kCacecMaxRows = 100;
constexpr bool in_rng_window(int post) { return post >= kRngPostLo && post <= kRngPostHi; }
constexpr uint32_t lcg_next(uint32_t s) { return s * 1664525u + 1013904223u; }
// Record address from the vector header words: begin + idx*elem (vector_at); 0 if inputs are unusable.
constexpr uint32_t vector_at(uint32_t begin, uint32_t elem, uint32_t idx) { return begin + idx * elem; }
// Expected ActorRecordAddress of a bound MarkerHandle: needs a live marker (lhu14 still the class id) and a usable vector.
constexpr uint32_t upd_expected_record(uint32_t marker_lhu14, uint32_t begin, uint32_t elem, uint32_t idx) {
    return (marker_lhu14 == kTgtIdA && begin != 0 && elem != 0) ? vector_at(begin, elem, idx) : 0u;
}

constexpr int kMaxUntracked = 64;  // cap on untracked sueggmove9/sweep38 rows (liveness discriminator)

// Object identity resolution. A spawned object is identified by its first-seen ordinal; the stable
// key is the owner word at object+0 (sweep a0 = *(obj)), the pointer may change if the object is
// relocated/defragmented. alias=true means "same owner, different pointer" (relocation candidate).
struct Resolve {
    int idx;
    bool alias;
};
inline Resolve resolve(const uint32_t* ptrs, const uint32_t* owners, int n, uint32_t ptr, uint32_t owner) {
    for (int i = 0; i < n; ++i) {
        if (ptrs[i] == ptr) return {i, false};
    }
    if (owner != 0) {
        for (int i = 0; i < n; ++i) {
            if (owners[i] == owner) return {i, true};
        }
    }
    return {-1, false};
}
// Load-stub retry: the entry executed again with the same return address and identical a0..a3 while
// the first entry is still pending (oracle sees the loader re-executing the trampoline).
constexpr bool is_retry(bool same_kind, uint32_t top_ra, uint32_t ra, bool same_args) {
    return same_kind && top_ra == ra && same_args;
}

constexpr bool in_window(int mode, int stage, int sub, int post) {
    return mode == kMode && stage == kStage && sub == kSub && post >= kPostLo && post <= kPostHi;
}

constexpr const char* kCsvHeader =
    "ev,ph,tick,seq,post,consumed,obj,a0,a1,a2,a3,v0,vmask,"
    "w0,w1,w2,w3,w4,w5,w6,w7,w8,w9,w10,w11,"
    "f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11";

constexpr const char* kRowFmt =
    "%s,%s,%llu,%llu,%d,%d,%d,%08x,%08x,%08x,%08x,%08x,%x,"
    "%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,"
    "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n";

}  // namespace tooie_demo1_projectile
