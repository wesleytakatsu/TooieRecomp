# P08 — RT64, entrada e frontend

Handoff completo: **[HANDOFF.md](./HANDOFF.md)**.

## Feito

| item | detalhe |
| --- | --- |
| Bug | `TOOIE_AUTO_START` não chamava `hide_all_contexts()` → launcher por cima do RT64 |
| Fix | `start_game` + `hide_all_contexts()` em `src/main/main.cpp` |
| Janela | título `Banjo-Tooie: Recompiled` |
| Versão | `0.1.0-p08` |
| RT64 | jiggy → terreno/água da abertura; câmera quase no chão, sem troca de tomada, personagem ou HUD |
| Scheduler | fila de threads sem auto-elo; TIDY coalescido; `DPC_STATUS_FREEZE` adaptado à execução imediata do RT64 |
| Progresso | pré-M3: geometria/áudio ativos, mas logos, câmera descendente, menu e demos não progridem |
| Overlays | lifecycle/relocations corrigidos; smoke 28 s com 39.075 dispatch/return, ainda não commitado |
| Core math | gaps `0x800136E4`/`0x800138D0`/`0x80013970` implementados como wrap/cos/sin em graus; não resolveram a tomada |
| Launcher | layout/animação do BanjoRecomp; jogo visível somente depois de Start Game |
| Música | preview Linux oculto; escuta com `TOOIE_DISABLE_LAUNCHER_MUSIC=1` validada pelo utilizador |
| Controles | path host = BanjoRecomp; SI bruto só CIC-6105 |
| SDL DB | build copia `recompcontrollerdb.txt` do BanjoRecomp |

## Como testar

```sh
cd build
TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./TooieRecompiled
# Estado atual: jiggy → terreno/água com câmera baixa + música; Escape = config
```

Evidência/handoff monorepo: `../docs/plan/evidence/p08-session-handoff.md`.

## Residual

1. Restaurar a máquina de estados da abertura (`gsattract`/`chintrocam`): logos,
   descida de câmera e demos.
2. Capturar e validar menu/título/gameplay real.
3. Retomar input, rumble/hotplug.
4. 4:3, fullscreen/foco/multi-monitor, fallback GPU.

## Diferido — input

Input no **title/file select do jogo** ainda falha (launcher OK).
Tentativa EEPROM LOOKUP: FAIL validação humana.
Handoff: `../docs/plan/evidence/p08-title-input-eeprom.md`.
