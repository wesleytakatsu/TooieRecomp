---
title: "BT — Input e controles nativos"
description: "Contrato de controles do TooieRecomp, deliberadamente alinhado ao BanjoRecomp."
status: current
area: "specs/BT"
updated: 2026-08-09
tags: [banjo-tooie-Recomp, tooie-recomp, input, controller, rumble]
---

# BT — Input e controles nativos

O TooieRecomp deve seguir o mesmo padrão do BanjoRecomp para reconhecimento de
dispositivos, perfis, defaults, remapeamento, persistência e rumble. Não existe
um segundo sistema de input específico de Tooie.

## Fonte de verdade

- Framework: `BanjoRecomp/lib/RecompFrontend/recompinput`.
- Referência de integração: `BanjoRecomp/src/main/main.cpp` e
  `BanjoRecomp/src/game/config.cpp`.
- Integração Tooie: `TooieRecomp/src/main/main.cpp`,
  `TooieRecomp/src/game/config.cpp` e `TooieRecomp/src/game/recomp_api.cpp`.
- Banco SDL: `BanjoRecomp/recompcontrollerdb.txt`.

## Contrato de runtime

Tooie registra exatamente os callbacks usados pelo BanjoRecomp:

| Callback | Implementação compartilhada |
| --- | --- |
| `poll_input` | `recompinput::poll_inputs` |
| `get_input` | `recompinput::profiles::get_n64_input` |
| `set_rumble` | `recompinput::set_rumble` |
| `get_connected_device_info` | `Controller` + `RumblePak` no perfil single-player |
| callback VI | `recompinput::update_rumble` |

`update_gfx()` chama `recompinput::handle_events()` a cada frame e o projeto usa
`recompinput::players::set_single_player_mode(true)`, como o BanjoRecomp.

Os exports `recomp_update_inputs`, `recomp_get_right_analog_inputs` e
`recomp_set_right_analog_suppressed` também têm fontes/hash idênticos nos dois
projetos.

### Roteamento libultra e SI/PIF

As chamadas recompiladas de Tooie chegam por `LOOKUP_FUNC`, portanto os símbolos
de controle precisam apontar explicitamente para os wrappers host do
N64ModernRuntime. `register_boot_lookup_stubs()` instala:

| VRAM Tooie | Wrapper host |
| --- | --- |
| `0x8002FF90` | `osContStartReadData_recomp` |
| `0x80030014` | `osContGetReadData_recomp` |
| `0x800301E0` | `osContInit_recomp` |
| `0x80030550` | `osContSetCh_recomp` |
| `0x80030640` | `__osMotorAccess_recomp` |
| `0x800308B4` | `osMotorInit_recomp` |

Assim, polling, leitura de botões/analógicos e rumble seguem o mesmo caminho
host do BanjoRecomp. O hook bruto `__osSiRawStartDma` permanece separado para o
tráfego PIF privado do desafio CIC-6105; ele não deve interpretar controles.

Breakpoints de runtime confirmaram `osContStartReadData_recomp` na thread
`[Game] SCHEDULE` e `osContGetReadData_recomp` na thread `[Game] JOY`.

## Defaults do controle moderno

Os defaults adicionais são os mesmos de `BanjoRecomp::set_control_defaults`:

| Entrada N64 | Binding moderno |
| --- | --- |
| `C_DOWN` | eixo Y do analógico direito (`true`) + shoulder esquerdo |
| `C_UP` | eixo Y do analógico direito (`false`) + shoulder direito |
| `C_LEFT` | eixo X do analógico direito (`false`) + botão norte |
| `C_RIGHT` | eixo X do analógico direito (`true`) + botão leste |
| `L` | clique do analógico direito (R3) |

Os demais bindings-base vêm do perfil N64 do RecompFrontend. A aba Controls usa
as mesmas descrições e o mesmo editor do BanjoRecomp.

## Persistência

`banjo::init_config()` chama, na mesma ordem:

1. `set_control_defaults()`;
2. `set_control_descriptions()`;
3. `recompui::config::create_controls_tab()`;
4. `recompui::config::finalize()`.

`controls.json` é lazy: ele pode não existir enquanto o usuário mantiver os
defaults. O arquivo é criado/atualizado pelo RecompFrontend quando um binding é
alterado. Não se deve criar um arquivo vazio artificialmente.

## Banco SDL

O TooieRecomp não mantém uma cópia independente do banco de gamepads. O target
CMake copia `BanjoRecomp/recompcontrollerdb.txt` para a pasta do executável em
todo build, usando `copy_if_different`.

Hash verificado na origem e no build:

```text
a0659f8fb5c1398365d1280034f39350259cd169c79c0e12002da0df92adcc7c
```

Isso eliminou o aviso `Failed to load controller mappings: Invalid RWops` no
smoke executado a partir de `TooieRecomp/build`.

## Critérios de paridade

- Não alterar defaults de Tooie sem alterar conscientemente o contrato com BK.
- Não duplicar `recompcontrollerdb.txt`.
- Remapeamento deve sobreviver ao reinício quando houver `controls.json`.
- Reconexão e rumble devem usar somente os callbacks do RecompFrontend.
- O caminho host de polling/leitura está comprovado, mas o title/file select do
  jogo ainda não reage ao input validado pelo usuário. Esse bloqueio é uma
  lacuna de runtime separada dos testes físicos de hotplug e rumble.
- Os hooks EEPROM usam os VRAMs de `tooie.us.syms.toml` e `SaveType::Eep16k`;
  são corretos para probe/save, mas não resolveram sozinhos o title/file select.
