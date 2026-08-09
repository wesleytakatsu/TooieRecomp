# P08 — RT64, entrada e frontend

## Feito

- RT64/Vulkan apresenta frames de jogo (evidência: transição jiggy).
- `TOOIE_AUTO_START` agora chama `recompui::hide_all_contexts()` após
  `start_game` — sem isso o launcher cobria o framebuffer e só se ouvia áudio.
- Título da janela: `Banjo-Tooie: Recompiled`.
- Versão: `0.1.0-p08`.

## Como testar

```sh
cd build
TOOIE_AUTO_START=1 ./TooieRecompiled
# Esperado: sem SVG do launcher; transição jiggy / frames do jogo + música
# Escape abre menu de config (RecompFrontend)
```

## Residual

- Completar animação de transição (`gctransitionDll`) até menu/logo Rare.
- `controls.json` persistente + rumble/hotplug.
- 4:3 original, fullscreen multi-monitor, fallback de GPU.
