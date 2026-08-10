---
title: "BT P08 — runtime, launcher e música"
description: "Evidência do desbloqueio DP/VI e do ciclo de música do launcher."
status: verified
project: TooieRecomp
phase: P08
updated: 2026-08-09
tags: [banjo-tooie-Recomp, rt64, launcher, audio, linux, evidence]
---

# BT P08 — runtime, launcher e música

## Build

```text
cd TooieRecomp/build
ninja TooieRecompiled
status=0
```

O build recompilou `main.cpp`, `boot_hw_stubs.cpp`, `threadqueue.cpp` e o
runtime compartilhado sem warning ou erro novo.

## Desbloqueio de renderização

Antes do ajuste, a MAIN bloqueava em `func_800151BC(1)` aguardando a fila
`0x80079010`; o contador parava em aproximadamente 17.262 syscalls e o último
overlay permanecia `674` (`gctransitionDll`).

O trace GDB das conclusões DP mostrou:

```text
DP-HIT 1 state84=0x40000001 state88=0x40000001 gfx=4
DP-HIT 2 state84=0x40000001 state88=0x40000001 gfx=16
DP-HIT 3 state84=0x00000002 state88=0x40000001 gfx=16
```

O terceiro display list era processado pelo RT64 mesmo com o bit sintético
`DPC_STATUS_FREEZE`; no N64 real ele aguardaria o VI liberar a RDP. O hook
específico de Tooie agora mascara esse bit contábil ao consultar o status.

Smoke de 20 segundos após o ajuste:

```text
syscalls finais: 32395 dispatch / 32395 return
overlays observados no trecho detalhado: 76
scheduler ack: 4
RSP audio: 560
queue samples: 844192
nonzero samples: 841117
fatal/assert/LOOKUP 0: nenhum
```

A captura visual confirmou que a transição termina e o RT64 segue renderizando
cenas 3D do jogo.

## Launcher e reinício da música

Com a ROM armazenada, o processo principal mantém o launcher no estilo do
BanjoRecomp e cria um segundo processo com janela SDL oculta. Esse processo faz
o boot nativo e toca a música usando a ROM do usuário. O clique em **Start Game**
inicia o runtime principal; um monitor encerra o preview antes do boot visível,
portanto música e sequência inicial recomeçam juntas.

Comando automatizado:

```sh
cd TooieRecomp/build
TOOIE_LAUNCHER_MUSIC_SMOKE=1 TOOIE_DIAG=1 ./TooieRecompiled
```

Marcadores observados:

```text
[tooie] launcher music: hidden ROM-backed preview started
[tooie] TOOIE_AUTO_START: starting game
[tooie] TOOIE_AUTO_START: starting game
[tooie] launcher music: restarting with visible game boot
```

Os dois boots produziram amostras não nulas. Nessa captura inicial apareceu
`Failed to load controller mappings: Invalid RWops`; o banco ausente foi
corrigido na validação de controles abaixo.

## Segurança e empacotamento

- Nenhuma faixa de música ou asset do jogo foi adicionado ao repositório.
- O processo auxiliar lê a mesma ROM validada pelo RecompFrontend.
- No fluxo normal, `SIGTERM` + `waitpid` encerra e coleta o preview; se o
  launcher morrer, `PR_SET_PDEATHSIG/SIGKILL` impede processo oculto órfão.
- `TOOIE_DISABLE_LAUNCHER_MUSIC=1` desativa o preview para diagnóstico.
- `TOOIE_AUTO_START=1` também desativa o preview e preserva o smoke direto.

## Paridade de controles com BanjoRecomp

A comparação por símbolos confirmou hashes idênticos entre BK e Tooie para:

- `set_control_defaults`;
- `set_control_descriptions`;
- `banjo::init_config`;
- `get_connected_device_info`;
- `recomp_update_inputs`;
- `recomp_get_right_analog_inputs`;
- `recomp_set_right_analog_suppressed`.

Os callbacks registrados em `main` também são os mesmos: `poll_inputs`, perfil
N64, `set_rumble`, device info e `update_rumble` no VI.

O único desvio encontrado era o banco SDL ausente no diretório do build. Após o
target CMake copiar diretamente a fonte canônica do BanjoRecomp:

```text
sha256 BanjoRecomp/recompcontrollerdb.txt:
a0659f8fb5c1398365d1280034f39350259cd169c79c0e12002da0df92adcc7c

sha256 TooieRecomp/build/recompcontrollerdb.txt:
a0659f8fb5c1398365d1280034f39350259cd169c79c0e12002da0df92adcc7c
```

O smoke de inicialização terminou com `controller_messages=0`, sem
`Invalid RWops` e sem erro fatal.

As chamadas libultra do jogo eram indiretas (`LOOKUP_FUNC`) e ainda não tinham
registro host próprio. Foram adicionados os seis símbolos presentes no binário
Tooie: `osContInit`, `osContStartReadData`, `osContGetReadData`, `osContSetCh`,
`__osMotorAccess` e `osMotorInit`. O hook `__osSiRawStartDma` continua dedicado
ao CIC-6105.

O novo smoke registrou `17 ultra LOOKUP hooks + 10 gap stubs`, preservou o
handshake CIC e passou sem erro fatal. Dois breakpoints comprovam o caminho de
entrada em execução:

```text
Thread "[Game] SCHEDULE" hit osContStartReadData_recomp
#1 func_80016810
#2 func_80014A88

Thread "[Game] JOY" hit osContGetReadData_recomp
#1 func_800165F0
#2 func_8001662C
#3 func_800166DC
```

Log compacto: [[p08-controller-host-path]].

## Auditoria do ciclo do preview

Após endurecer o encerramento do auxiliar Linux:

```text
launcher normal: CHILD_BEFORE_START=<pid>
após Start Game: CHILD_AFTER_START=gone
log: hidden ROM-backed preview started
log: restarting with visible game boot

launcher morto com SIGKILL: CHILD_AFTER_PARENT=gone
```

O fluxo normal usa `SIGTERM`, espera até um segundo e faz `waitpid`; se o
auxiliar não terminar, usa `SIGKILL` e coleta o PID. Para crash/morte do pai,
`PR_SET_PDEATHSIG/SIGKILL` encerra o processo oculto pelo kernel.
