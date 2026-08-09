# P08 — RT64, entrada e frontend

Handoff completo: **[HANDOFF.md](./HANDOFF.md)**.

## Feito (commit `e2cdb30`)

| item | detalhe |
| --- | --- |
| Bug | `TOOIE_AUTO_START` não chamava `hide_all_contexts()` → launcher por cima do RT64 |
| Fix | `start_game` + `hide_all_contexts()` em `src/main/main.cpp` |
| Janela | título `Banjo-Tooie: Recompiled` |
| Versão | `0.1.0-p08` |
| RT64 | evidencia transição jiggy com mundo no recorte (`../docs/plan/evidence/p08-game-view.png`) |

## Como testar

```sh
cd build
TOOIE_AUTO_START=1 ./TooieRecompiled
# Esperado: sem SVG azul do launcher; jiggy / frames de jogo + música
# Escape: config RecompFrontend
```

## Residual (próximo agente)

1. **Prioridade:** animação `gctransitionDll` (ovl 674) não completa — jiggy trava.
2. `controls.json` + rumble/hotplug.
3. 4:3 original, fullscreen, fallback GPU.
4. Evidência logo Rare / menu / first playable.
