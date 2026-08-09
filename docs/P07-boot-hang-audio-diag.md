# P07 — Diagnóstico: hang de boot e silêncio

## Sintoma

Tela parada + silêncio no headset selecionado no sistema.

## Device (descartado como causa)

- `SDL_OpenAudioDevice(nullptr)` → default SDL/Pulse.
- PipeWire: stream `TooieRecompiled` no sink default (ex.: FELITRON USB).
- Volume app 100%, não corked, não mudo.

## Hang 1 — SI no-op

Stack: `gsworldDll:2 → gsmaker:2 → gldbDll:0`.

`gldbDll` chama `cic_send_challenge` → `__osSiRawStartDma` →
`osRecvMesg(BLOCK)`. Com SI no-op, a mesg de conclusão nunca chega.

**Fix:** `tooie___osSiRawStartDma` com espelho PIF 64 B +
`ultramodern::send_si_message()`.

## Hang 2 — CIC 6105 incorreto

SI destravado, mas decrypt XOR do asset em `gldb` usava resposta CIC errada
(LUT switch incompleto). Stream de `gsmaker_entrypoint_16` nunca via
terminador → busy loop (`726:2 > 2:16`), `peak=0`.

**Fix:** algoritmo X-Scale/mupen (`n64_cic_nus_6105`, len=30), zera
`pif[0x2e/0x2f]`, reempacota resposta em `0x30`.

## Resultado

- `leave ovl=726` (gsworld completa).
- `peak` / `maxpeak` > 0; `nonzero` alto.
- **Música inicial** confirmada por ouvido (2026-08-09).

## Ferramenta

```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DIAG_INTERVAL_MS=2000 ./TooieRecompiled
```
