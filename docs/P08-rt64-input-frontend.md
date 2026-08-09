# P08 — RT64, entrada e frontend

Handoff completo: **[HANDOFF.md](./HANDOFF.md)**.

## Feito

| item | detalhe |
| --- | --- |
| Bug | `TOOIE_AUTO_START` não chamava `hide_all_contexts()` → launcher por cima do RT64 |
| Fix | `start_game` + `hide_all_contexts()` em `src/main/main.cpp` |
| Janela | título `Banjo-Tooie: Recompiled` |
| Versão | `0.1.0-p08` |
| RT64 | jiggy → cena 3D atribuída a **Spiral Mountain**; personagem/HUD não aparecem nas capturas |
| Scheduler | fila de threads sem auto-elo; TIDY coalescido; `DPC_STATUS_FREEZE` adaptado à execução imediata do RT64 |
| Progresso | M3 visual: cena 3D ativa; smoke ~98k syscalls, `gcsectionDll` 677:3; jogabilidade não comprovada |
| Launcher | layout/animação do BanjoRecomp; jogo visível somente depois de Start Game |
| Música | preview Linux oculto; escuta com `TOOIE_DISABLE_LAUNCHER_MUSIC=1` validada pelo utilizador |
| Controles | path host = BanjoRecomp; SI bruto só CIC-6105 |
| SDL DB | build copia `recompcontrollerdb.txt` do BanjoRecomp |

## Como testar

```sh
cd build
TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./TooieRecompiled
# Esperado: jiggy → cena 3D + música; Escape = config
```

Evidência monorepo: `../docs/plan/evidence/p08-first-area-playable.md`.

## Residual

1. Teste físico teclado/gamepad/rumble/hotplug.
2. Logo Rare / menu título isolados.
3. 4:3, fullscreen/foco/multi-monitor, fallback GPU.

## BLOQUEADO — próximo agente

Input no **title/file select do jogo** ainda falha (launcher OK).
Tentativa EEPROM LOOKUP: FAIL validação humana.
Handoff: `../docs/plan/evidence/p08-title-input-eeprom.md`.
