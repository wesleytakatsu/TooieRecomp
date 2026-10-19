---
title: "P08 — Caminho host dos controles"
description: "Comprovação do caminho de entrada host para controles via stubs e isolamento do CIC-6105."
status: validated
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-09
tags: [p08, input, controller, host, recomp-api, cic6105]
---

# P08 — caminho host dos controles

Data: 2026-08-09

## Objetivo

Comprovar que Banjo-Tooie usa o mesmo caminho de entrada do BanjoRecomp, sem
enviar comandos normais de controle ao hook PIF criado para o CIC-6105.

## Implementação

`TooieRecomp/src/game/boot_hw_stubs.cpp` registra no mapa `LOOKUP_FUNC`:

```text
0x8002FF90 osContStartReadData_recomp
0x80030014 osContGetReadData_recomp
0x800301E0 osContInit_recomp
0x80030550 osContSetCh_recomp
0x80030640 __osMotorAccess_recomp
0x800308B4 osMotorInit_recomp
```

`__osSiRawStartDma` em `0x80031A60` permanece dedicado ao espelho PIF e ao
challenge/response CIC-NUS-6105.

## Build e smoke

```text
$ ninja TooieRecompiled
[1/2] Building CXX object .../boot_hw_stubs.cpp.o
[2/2] Linking CXX executable TooieRecompiled

[tooie] registered 17 ultra LOOKUP hooks + 10 gap stubs
[tooie-diag] CIC6105 C/R on SI write buf=0x8007d850 cmd=0x80
fatal_errors=0
```

## Prova por breakpoint

```text
Thread "[Game] SCHEDULE" hit Breakpoint in osContStartReadData_recomp
#0 osContStartReadData_recomp
#1 func_80016810
#2 func_80014A88

Thread "[Game] JOY" hit Breakpoint in osContGetReadData_recomp
#0 osContGetReadData_recomp
#1 func_800165F0
#2 func_8001662C
#3 func_800166DC
```

Resultado: polling e leitura chegam ao N64ModernRuntime/RecompFrontend, como no
BanjoRecomp. Isso comprova o transporte host, não a aceitação do input por todas
as telas do jogo: title/file select ainda está bloqueado. Testes físicos de
gamepad, hotplug e rumble também permanecem pendentes.
