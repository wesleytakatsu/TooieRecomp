---
title: "Definição de pronto — TooieRecomp"
description: "Critérios objetivos para considerar Banjo-Tooie concluído como port nativo no Linux."
status: active
type: acceptance-criteria
project: TooieRecomp
tags:
  - banjo-tooie-Recomp
  - quality
  - acceptance
  - linux
---

# Definição de pronto — TooieRecomp

Esta nota transforma “rodar perfeitamente” em gates verificáveis. Nenhuma
melhoria visual compensa uma regressão de comportamento do jogo original.

## Gate funcional

- Boot frio chega ao menu principal repetidamente.
- Nova campanha pode ser concluída sem crash, softlock ou comando de debug.
- Todas as áreas, chefes, minigames, transformações e cutscenes são acessíveis.
- Trocas de overlay e carregamentos não corrompem estado ou memória.
- Saves criam, carregam, sobrescrevem e preservam progresso entre versões.

## Gate audiovisual

- Display lists usadas pelo jogo renderizam sem erros fatais no RT64.
- Áudio, música, vozes e efeitos permanecem sincronizados em sessões longas.
- O modo padrão reproduz aspect ratio e timing originais.
- Widescreen e FPS alto são opcionais e não aceleram física, animação, áudio,
  timers ou cutscenes.

## Gate de entrada e sistema

- Teclado e ao menos um controle SDL funcionam, incluindo rumble quando
  suportado.
- Conectar ou remover controles não encerra o jogo.
- Pause, foco da janela, fullscreen e troca de resolução preservam estado.
- Encerramento normal grava dados e libera recursos sem erro.

## Gate técnico

- Build reproduzível em instalação Linux limpa e documentada.
- Compilação sem símbolos, relocations, syscalls ou tarefas RSP desconhecidas.
- Sanitizers e ferramentas de memória não encontram erro reproduzível no smoke
  test definido pelo projeto.
- Matriz de regressão associa cada falha a uma versão e a uma evidência.
- Versões de toolchain, dependências e commits estão fixadas.

## Gate de distribuição

- O pacote não contém ROM, texturas, modelos, músicas ou outros assets do jogo.
- A ROM válida é verificada por hash e carregada pelo usuário.
- Uma ROM inválida produz mensagem clara e não continua silenciosamente.
- Licenças, créditos e fontes dos componentes redistribuídos acompanham o
  pacote.
- Executável e pacote têm checksum publicado.

## Evidência mínima de release

| Evidência | Formato esperado |
| --- | --- |
| Build limpo | log com commit, compilador e exit code zero |
| Smoke test | log do boot até gameplay |
| Campanha | checklist da matriz funcional |
| Saves | hashes antes/depois e teste de recarga |
| Gráficos/áudio | capturas e lista de tarefas RSP/display lists observadas |
| Memória | relatório de sanitizer/Valgrind equivalente |
| Pacote | listagem de conteúdo e SHA-256 |

## Relações

- Plano mestre: [[index]]
- Última etapa: [[../plans/BT/10-linux-packaging-and-release]]
- Paridade: [[../plans/BT/09-gameplay-parity-and-enhancements]]
