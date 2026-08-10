---
title: "BT P04 — Recompilação estática"
description: "Gera C/C++ nativo a partir do MIPS de Banjo-Tooie e fecha o primeiro build compile-only."
status: done
type: implementation-stage
project: TooieRecomp
phase: P04
order: 4
depends_on: P03
updated: 2026-08-09
tags: [banjo-tooie-Recomp, n64recomp, generated-code, compile]
---

# BT P04 — Recompilação estática

## Resultado esperado

O código MIPS classificado pelo projeto produz unidades nativas e compila em
Linux sem símbolo ausente.

## Tarefas

- [x] `P04-T01` Fixar uma versão compatível do N64Recomp usada pelo runtime escolhido.
- [x] `P04-T02` Executar geração do código base, overlays e tabelas auxiliares.
- [x] `P04-T03` Separar fontes geradas por base, core e overlay para builds incrementais úteis.
- [x] `P04-T04` Corrigir classificações de símbolos, jump tables e seções com base na causa, nunca editando C gerado.
- [x] `P04-T05` Resolver imports externos por libultra, runtime ou stub explícito rastreável.
- [x] `P04-T06` Compilar com Clang em warnings estritos e guardar a lista de exceções justificadas.
- [x] `P04-T07` Adicionar regra de regeneração limpa e teste que detecte saída gerada obsoleta.
- [x] `P04-T08` Medir cobertura do código executável recompilado e registrar lacunas.

## Implementação

### N64Recomp fixado

| Item | Valor |
| --- | --- |
| Path | `BanjoRecomp/lib/N64ModernRuntime/N64Recomp` |
| Commit (submodule) | `2b6f05688de2abc7d86da5b4a89b84c2c6acbabe` (CI BK default) |
| Binary | `…/N64Recomp/build/N64Recomp` |
| Política local | `context.use_lookup_for_all_function_calls = true` em `src/main.cpp` |

**Por quê lookup forçado:** o decomp Tooie ainda tem muitos símbolos de função
com tamanho zero; a resolução estrita de `jal` falha. LOOKUP em runtime cobre
esses alvos até o decomp completar tamanhos. Documentado; **não** fazer upstream
sem discussão.

### Config e geração

| Arquivo | Papel |
| --- | --- |
| `TooieRecomp/tooie.us.rev0.recomp.toml` | ELF + reloc list + stubs |
| `TooieRecomp/metadata/relocatable_sections.recomp.txt` | 750 seções no dump |
| `TooieRecomp/metadata/recomp_stubs.txt` | 22 stubs rastreados |
| `TooieRecomp/tools/run_recomp.sh` | regeneração limpa |
| `TooieRecomp/tools/check_generated_fresh.py` | detecta saída obsoleta |
| `TooieRecomp/CMakeLists.txt` | lib estática compile-only |

```sh
./TooieRecomp/tools/run_recomp.sh
cmake -S TooieRecomp -B TooieRecomp/build-recomp \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build TooieRecomp/build-recomp -j$(nproc)
python3 TooieRecomp/tools/check_generated_fresh.py
```

### Stubs explícitos (não silenciosos)

Gerados por iteração fail→stub até o recomp passar (cop0, TLB, placeholders):

`boot___osGetCause`, `boot___osGetSR`/`SetSR`, exception/dispatch/enqueue,
`boot_handle_CpU`, cache/TLB helpers, `ovl_entrypoint_alternate` (jal 0x80000000),
`func_80013B70`, `func_8001DDC0`, etc. Lista completa:
`TooieRecomp/metadata/recomp_stubs.txt`.

### Layout gerado

N64Recomp agrupa funções em `RecompiledFuncs/funcs_*.c` (+ `funcs.h`, `lookup.cpp`)
— útil para builds incrementais (T03), não um arquivo monólito.

## Evidências (2026-08-09)

| Check | Resultado |
| --- | --- |
| `N64Recomp tooie.us.rev0.recomp.toml` | **exit 0** |
| Arquivos gerados | 417 `funcs_*.c` + header/lookup (~89 MB) |
| Declarações em `funcs.h` | **20898** funções |
| Stubs | **22** (lista versionada) |
| Compile Clang Release | **exit 0** → `libRecompiledFuncs.a` (~15 MB) |
| Warnings | poucos `-Wpointer-sign` em DDIV (justificados; gerado) |
| Freshness check | **PASS** |
| Logs | `docs/plan/evidence/p04-n64recomp.log`, `p04-compile.log` |

## Debug esperado

Erros devem ser corrigidos na camada que os produz:

- função ausente: mapas de símbolos;
- endereço ou seção incorreta: metadados/Splat;
- relocation inválida: gerador de relocations;
- chamada externa: contrato runtime/libultra;
- comportamento especial: patch nativo documentado / stub listado.

## Gate de saída

- [x] `RecompiledFuncs` é regenerável (`run_recomp.sh`) e compila do zero.
- [x] Não há edição manual em fonte gerada.
- [x] Stubs/lookup são explícitos e documentados (não “ignore silencioso” de reloc).

## Próxima etapa

[[05-native-runtime-and-boot]]
