# P07 — RSP e áudio (TooieRecomp)

Status: **active** — microcódigo + path de áudio funcionando; **música inicial
confirmada** no device default do sistema (2026-08-09).

## Entregas

1. **`n_aspMain` Tooie** via RSPRecomp (`n_aspMain.us.rev0.toml`, text ROM
   `0x1E4F3B0` size `0x17D0` em `decompressed.us.z64`). Não reutilizar o ASP de BK.
2. **Labels** pós-RSPRecomp (`tools/fix_rsprecomp_labels.py`) — ramos com
   endereço `> 0x1FFF` mascarados incorretamente pelo recompiler.
3. **Dispatch** `get_rsp_microcode`: só `M_AUDTASK` → `n_aspMain`.
4. **SDL audio** no dispositivo **default** (`SDL_OpenAudioDevice(nullptr,…)`).
5. **SI + CIC-NUS-6105** em `boot_hw_stubs.cpp` (espelho PIF + algoritmo X-Scale/mupen)
   — sem isso `gldbDll` trava e o load de mundo/stream `gsmaker` não termina.
6. **`TOOIE_DIAG=1`**: contadores de syscall, `rsp_aud`, `queue_*`, peak/nonzero, hang stack.

## Como regenerar o microcódigo

```sh
# Requer: RSPRecomp buildado (BanjoRecomp/.../N64Recomp/build/RSPRecomp)
#          banjo-tooie-Recomp/decompressed.us.z64
./tools/run_rsprecomp.sh
cmake --build build --target TooieRecompiled -j$(nproc)
```

## Como validar áudio

```sh
cd build
TOOIE_AUTO_START=1 ./TooieRecompiled
# Opcional: TOOIE_DIAG=1  |  TOOIE_LOG_RSP=1
```

Esperado no boot saudável: música inicial no fone/saída default.

## Arquivos-chave

| path | papel |
| --- | --- |
| `n_aspMain.us.rev0.toml` | config RSPRecomp |
| `rsp/n_aspMain.cpp` | microcódigo gerado |
| `src/main/main.cpp` | RSP dispatch + SDL |
| `src/game/boot_hw_stubs.cpp` | SI/CIC/scheduler host |
| `src/game/tooie_diag.cpp` | diagnóstico |
| `metadata/rsp_tasks.inventory.md` | OSTask / ROM blobs |
| `tools/run_rsprecomp.sh` | regen + fix labels |
| `tools/compare_asp_microcodes.py` | Tooie vs BK |

## Residual

- Soak longo, SFX, fala, pause, cutscenes, hotplug de áudio.
- Title/menu visual completo e input (P08).
