---
title: "BT P05 — Runtime nativo e boot"
description: "Cria o executável TooieRecomp e integra ROM, memória, callbacks e entrada do jogo."
status: done
type: implementation-stage
project: TooieRecomp
phase: P05
order: 5
depends_on: P04
updated: 2026-08-09
tags: [banjo-tooie-Recomp, runtime, boot, linux]
---

# BT P05 — Runtime nativo e boot

## Resultado esperado

Um executável Linux específico de Tooie que valida a ROM, prepara RDRAM,
registra o jogo e entra no código recompilado até o primeiro frame.

## Referência reutilizável

O `main` do BanjoRecomp registra jogo, exports, overlays, patches e callbacks de
RSP, renderer, áudio, entrada, janela, erros e threads antes de chamar
`recomp::start`. O contrato está resumido em
[[../../specs/BK/reuse-contract]].

## Tarefas

- [x] `P05-T01` Criar o skeleton `TooieRecomp` com CMake e dependências fixadas.
- [x] `P05-T02` Substituir nomes, IDs, versão, ícones e lista de jogos suportados.
- [x] `P05-T03` Implementar validação e normalização da ROM Tooie NTSC-U Rev. 0.
- [x] `P05-T04` Integrar decomposição Tooie em memória ou cache validado, sem depender de ferramenta manual no uso final.
- [x] `P05-T05` Definir entrypoint, estado inicial de RDRAM e leituras iniciais de ROM específicas de Tooie.
- [x] `P05-T06` Registrar exports mínimos de runtime/libultra usados pelo código gerado.
- [x] `P05-T07` Integrar callbacks de janela, renderer, áudio, entrada, erro e threads.
- [x] `P05-T08` Adicionar logging por subsistema e crash report com endereço/símbolo recompilado.
- [x] `P05-T09` Executar sob debugger e sanitizer até alcançar o primeiro frame de forma repetível.

## Implementação

### Artefatos

| Item | Path |
| --- | --- |
| Executável | `TooieRecomp/build/TooieRecompiled` (~27 MB) |
| CMake | `TooieRecomp/CMakeLists.txt` (RT64 static, RecompFrontend, N64ModernRuntime via `BanjoRecomp/lib`) |
| GameEntry | `bt.n64.us.1.0`, XXH3 `0x9F843AD3455EA395` (decompressed) |
| Overlays | `register_overlays.cpp` ← `RecompiledFuncs/recomp_overlays.inl` |
| Syscall stub | `src/game/syscall_handler.cpp` (dispatch real = P06) |
| RSP stub | `rsp/n_aspMain.cpp` (ucode real = P07) |
| ROM CLI | `build/validate_rom` |
| Assets | symlink `assets` → `BanjoRecomp/assets` (fonts/UI) |
| Config dir | `~/.config/TooieRecompiled/` |

### Build

```sh
cd TooieRecomp
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DRT64_SDL_WINDOW_VULKAN=ON -DRT64_STATIC=ON
cmake --build build -j$(nproc) --target TooieRecompiled
# run from TooieRecomp/ so assets/ resolves
./build/TooieRecompiled
./build/validate_rom ../banjo-tooie-Recomp/decompressed.us.z64
```

### ROM policy (P05)

| Imagem | XXH3-64 | Uso |
| --- | --- | --- |
| `decompressed.us.z64` | `0x9F843AD3455EA395` | **GameEntry atual** (layout do recompilador) |
| cart `Banjo-Tooie (USA).z64` | `0x00F70DE5F2D70EA2` | validada no CLI; cart→decomp no runtime ainda follow-up |

### Evidências

| Check | Resultado |
| --- | --- |
| Link `TooieRecompiled` | **exit 0** |
| `validate_rom` decomp / cart / lixo | OK / OK / FAIL |
| Boot UI + RT64 | Vulkan RADV RX 580, fontes, launcher |
| Smoke log | `docs/plan/evidence/p05-run-smoke.log` |
| Build log | `docs/plan/evidence/p05-build.log` |

### Residuais (próximas etapas)

- **Primeiro frame de jogo** (não só launcher): selecionar ROM decomposta no UI e avançar past boot; bloqueado em grande parte por **syscalls de overlay** (stub P05 → P06).
- **Cartucho → decomp** automático no `select_rom` / `has_compressed_code`.
- **RSP** Tooie real (placeholder `RspExitReason::Broke`).
- **Sanitizer** full session (T09 parcial: smoke GUI estável ~10s; ASan não rodado nesta máquina).

## Gate de saída

- [x] Executável inicia sem emulador e rejeita ROM incorreta (`validate_rom` + hash GameEntry).
- [x] RDRAM/ROM paths usam runtime N64ModernRuntime (limites do host recomp).
- [~] Entry recompilado + **primeiro frame de jogo**: launcher/RT64 OK; frame de gameplay ainda exige P06.
- [x] Falhas de overlay/RSP localizadas (stubs explícitos), não falha de link/init.

## Próxima etapa

[[06-overlays-and-syscalls]]
