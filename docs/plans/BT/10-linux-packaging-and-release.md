---
title: "BT P10 — Empacotamento e release Linux"
description: "Produz, testa e publica um artefato Linux reproduzível sem conteúdo proprietário do jogo."
status: pending
type: implementation-stage
project: TooieRecomp
phase: P10
order: 10
depends_on: P09
tags: [banjo-tooie-Recomp, linux, packaging, release]
---

# BT P10 — Empacotamento e release Linux

## Resultado esperado

Um pacote instalável/portátil para Linux x86-64 que pede a ROM do usuário,
valida sua revisão e executa sem toolchain de desenvolvimento.

## Tarefas

- [ ] `P10-T01` Definir artefato primário Linux e um formato secundário apenas se necessário.
- [ ] `P10-T02` Instalar executável, bibliotecas permitidas, shaders, fontes, controller DB e metadados.
- [ ] `P10-T03` Implementar seleção inicial e cache seguro da ROM com validação por hash.
- [ ] `P10-T04` Garantir que ROM e assets extraídos nunca entrem no pacote ou cache de CI.
- [ ] `P10-T05` Adicionar desktop file, ícone, licença, créditos e notas de compatibilidade.
- [ ] `P10-T06` Construir em ambiente Linux limpo e testar em GPU/driver diferente quando disponível.
- [ ] `P10-T07` Executar smoke test automatizado do pacote instalado e do modo portátil.
- [ ] `P10-T08` Gerar SBOM/lista de dependências e checksums do release.
- [ ] `P10-T09` Publicar instruções de diagnóstico e coleta de logs sem dados pessoais.
- [ ] `P10-T10` Fechar todos os gates de [[../../plan/definition-of-done]].

## Gate de saída

- Build reproduzível a partir de checkout limpo.
- Pacote inicia em máquina sem dependências de desenvolvimento.
- Auditoria confirma ausência de ROM e assets do jogo.
- Checksum, licenças, instruções e matriz de compatibilidade acompanham o release.
- Todos os critérios da definição de pronto estão atendidos.

## Encerramento

Atualizar [[../../plan/index]] para `done`, congelar as versões do marco M6 e
preservar a matriz de evidências da release.
