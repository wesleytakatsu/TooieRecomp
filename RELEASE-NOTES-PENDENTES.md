---
title: "Changelog consolidado e continuidade da próxima release"
description: "Rascunho aprovado na conversa, escopo da comparação com main e instruções para preparar a release após o merge."
status: draft
type: release-notes
project: TooieRecomp
updated: 2026-11-02
tags: [release, changelog, handoff]
---

# Changelog consolidado e continuidade da próxima release

## Finalidade e ponto de retomada

O usuário aprovou o changelog abaixo e pediu que fosse preservado na raiz para sobreviver à compactação do contexto.
O usuário fará o merge da PR para `main`; depois será preparado um novo build para uma nova release.
Este documento é um rascunho editorial e de continuidade, não um plano de implementação nem uma release publicada.

- Repositório: `https://github.com/wesleytakatsu/TooieRecomp`.
- Checkout de trabalho desta entrega: `E:\projetos\TooieRecomp-win`.
- Branch da PR: `codex/tk64-launcher`; destino: `main`.
- Base da comparação: `3912fc24b1d0765c592adb410ec696b2ddb74dc2`.
- Último commit de implementação/manutenção comparado: `ab767398beb7d366c5415dfee0e4726aaaf4062e`.
- A comparação original contém 23 commits além de `main`; o commit deste documento é posterior a essa contagem.
- `origin/main` foi atualizado e conferido em 2026-11-02, ainda na base acima.
- Última release consultada no GitHub: `v0.1.3`, pré-release Windows x64. Não foi escolhida a próxima versão.
- Autorização atual: salvar esta nota, enviar a branch e abrir a PR com GitHub CLI. O merge será feito pelo usuário.
- O build/publicação ficam para a etapa posterior ao merge; esta tarefa não os executa.

**Atenção editorial:** este changelog compara com `main`, não com a última release pública. `v0.1.1`/`v0.1.2`/`v0.1.3`
já publicaram parte das novidades antigas, como Warp, otimizações, ZIPs de texturas e cheats. Antes de escrever as
notas finais, comparar o commit efetivamente mesclado com a tag da release anterior e distinguir novidades de
recapitulação. Não anunciar todo este texto como novidade desde `v0.1.3` sem essa revisão.

## Changelog aprovado: comparação com main

### 1. Modo desenvolvedor no menu ESC

- Nova aba **Dev Mode**, liberada após aceitar um aviso de uso.
- Desbloqueio válido até fechar o jogo.
- Ferramentas de teleporte, voo, Breegull Blaster, seleção de personagens e splitscreen experimental.

### 2. Teleporte para posições salvas

- Ferramenta **Warp** permite salvar a localização atual, dar um nome, renomear, excluir e retornar depois.
- Guarda mapa, entrada, posição e direção do personagem.
- Funciona dentro do mesmo mapa e entre mapas.
- Nas viagens entre mapas, aguarda o carregamento e a entrada do personagem antes de aplicar a posição salva.
- Posições permanecem disponíveis após fechar e reabrir o jogo.
- Teleportar não restaura inimigos, objetos coletados ou outros estados anteriores do cenário.

### 3. Atalhos de voo e Breegull Blaster

- Novo controle configurável **Debug Button**, com R3 no controle e T no teclado como padrões.
- Cada ferramenta possui chave de ativação e seletor de direção do D-pad.
- **Fly:** inicia o voo usando o comportamento de decolagem das plataformas do jogo.
- **Breegull Blaster:** permite entrar e sair desse modo, incluindo a restauração correta do modelo ao sair.
- No splitscreen, os atalhos atuam no personagem de quem os acionou, independentemente do outro jogador.

### 4. Seleção de personagens e transformações

- Menu com **17 opções**, utilizável separadamente por J1 e J2.
- Inclui Banjo/Kazooie, variantes com Dragon Kazooie, personagens separados, Mumbo, Snowball, Bee, Washing Machine,
  Stony, Breegull Blaster, Submarine, Golden Goliath, Detonator, Van e os dois tamanhos de T. rex.
- Abertura por atalho configurável ou pelos botões **Open for Player 1/2**.
- Cada jogador pode abrir e operar sua própria lista, dentro da respectiva área da tela.
- Corrigida a troca direta para Dragon Kazooie quando o modelo principal permanece o mesmo.
- Melhorada a preservação individual das formas durante transições entre mapas.
- Golden Goliath recebe os 120 segundos iniciais, mantendo a contagem nativa.
- Clockwork Kazooie foi excluído do seletor devido ao problema encontrado ao utilizá-lo dessa maneira.

### 5. Cooperativo experimental em tela dividida

- Opção no Dev Mode para adicionar J2 ao modo história.
- Dois personagens controláveis, com câmeras próprias, coleta de itens, ataques e inventário compartilhado.
- J2 é colocado próximo de J1 ao entrar no mapa e acompanha os teleportes da ferramenta Warp.
- Criação dos dois jogadores aplicada no próximo carregamento de mapa elegível.
- Corrigidas alterações e oscilações indevidas de iluminação entre os personagens.
- Corrigido o Glowbo que concedia itens repetidamente sem desaparecer quando coletado por J2 longe de J1.
- Adicionado reconhecimento de J2 como alvo em **oito famílias de inimigos**, permitindo perseguição e ataques nos
  comportamentos cobertos.
- Corrigido o desaparecimento das listas de destinos de Silos e Warp Pads quando J2 está distante. A operação
  desses transportes permanece exclusiva de J1.
- Reforçado o isolamento dos controles, seletores e ações de cada jogador durante carregamentos e recriação dos
  personagens.

### 6. Suspensão do splitscreen em minigames

- Implementada suspensão nas salas previstas para as **15 famílias de minigames e seus 23 estágios**, incluindo
  variantes relacionadas e trechos de Saucer of Peril.
- Nessas salas, preserva-se a criação original dos jogadores e os controles exigidos pelo evento.
- A opção de splitscreen continua salva; J2 retorna no próximo carregamento elegível.
- **Limitação conhecida:** a corrida de trenzinho com o pássaro continua permitindo acesso a J2. Esse ajuste foi
  adiado por decisão do usuário.
- A cobertura depende das salas e transições identificadas; não existe detecção universal de qualquer atividade
  iniciada no mapa.

### 7. Câmera analógica funcional

- Ativada a opção **General → Analog Camera**.
- Analógico direito movimenta a câmera continuamente, com velocidade proporcional à intensidade da alavanca e
  transição suave.
- Sensibilidade e inversão horizontal/vertical passam a funcionar para essa câmera.
- Movimento calculado pelo tempo da simulação, evitando variação de velocidade entre diferentes taxas de FPS.
- **Z + analógico direito preserva os comandos C originais**, inclusive quando Z foi remapeado.
- Na mira em primeira pessoa, o analógico volta a funcionar como C, preservando aumentar e diminuir o zoom.
- Modos especiais e câmeras não compatíveis mantêm os controles originais, incluindo Breegull Blaster e minigames.
- Entrada e suavização independentes para cada jogador no splitscreen.

### 8. Novas opções de compatibilidade gráfica

- Integração do **TK64**, acrescentando DirectX 11 e OpenGL às opções DirectX 12 e Vulkan.
- Seletor **Graphics API** na aba Graphics, com rótulos compactos: **Auto, DX12, Vulkan, DX11 e OpenGL**.
- Preferência salva para a próxima abertura; aplicar não troca o renderizador durante a sessão.
- No Windows, inicialização automática tenta a cadeia **DX12 → Vulkan → DX11 → OpenGL**, respeitando exceções de
  drivers e preferência do Wine.
- Falha de inicialização de uma candidata permite tentar a seguinte antes de mostrar o menu.
- Descrição informa a API efetivamente utilizada.
- Diagnóstico opcional registra tentativas, falhas e API selecionada.
- O caminho OpenGL exige **OpenGL desktop 4.3 ou superior**; não acrescenta suporte a OpenGL 3.x.

### 9. Inicialização, música do menu e modo de janela

- Removida a execução oculta do jogo usada anteriormente para produzir música no launcher.
- Em uso normal, o jogo começa efetivamente ao selecionar **Start Game**.
- Música do menu inicial reproduzida por um player independente, com suporte a loop e aos volumes Master/BGM.
- Reprodução encerrada com fade antes de começar o áudio do jogo.
- Removidos o botão de escolher música e o seletor de API sobre a arte do launcher.
- A música depende do MP3 privado configurado localmente; esse arquivo não acompanha o executável ou pacote público.
- Corrigida a persistência de janela/tela cheia ao usar F11 ou Alt+Enter.
- Corrigida a exposição indevida de janelas ocultas durante mudanças de modo.

### 10. Distância de objetos e correções visuais

- Nova opção **Graphics → Object Distance**: **Original, 2x, 4x e Unlimited**.
- Configuração persistente, aplicada sem reiniciar o jogo; padrão Original.
- Ampliação coordenada da seleção, atualização e desenho de objetos, inimigos e coletáveis, mantendo o aparecimento
  gradual.
- No splitscreen, a atualização ampliada considera a proximidade dos dois jogadores.
- Unlimited remove os limites de distância cobertos pela implementação; paredes, setores e condições próprias dos
  objetos continuam podendo ocultá-los.
- Corrigido o esticamento de cenas comuns, como explicações de habilidades do Jamjars, respeitando a proporção atual
  da janela.
- Corrigidos cortes parciais de ovos, penas, inimigos e sombras nas áreas laterais do widescreen, inclusive em
  determinados ângulos de câmera.
- Corrigida também uma sobrescrita de memória nos comandos de recarga das matrizes de projeção.

### 11. Desempenho, mods e manutenção

- Otimizado o processamento gráfico de cenas com muitos objetos e desenhos pequenos.
- No ponto medido de Glitter Gulch Mine, houve melhora de aproximadamente **9–12 para 60 FPS** no computador Windows
  testado. Não é uma promessa de desempenho para outros computadores ou cenas.
- Pacotes de texturas RT64 passam a aceitar **ZIP original**, inclusive com pasta interna, sem conversão para `.rtz`.
- Instalação pela interface ou cópia para a pasta de mods, preservando o arquivo original.
- Melhorada a organização do Dev Mode: descrições, seletores alinhados, botões de jogadores e rolagem.
- Removidos diagnósticos temporários, medições de desenvolvimento e código sem uso nos caminhos revisados.
- Mantidos diagnósticos úteis sob ativação explícita e testes automatizados separados do executável do jogo.
- Documentação reorganizada; planos obsoletos removidos e propostas futuras reunidas em documento próprio.

## Limitações a preservar nas notas públicas

Splitscreen continua experimental. Portas e mudanças de mapa permanecem dependentes de J1; a morte de J2 afeta a vida
compartilhada; dois Golden Goliaths ainda compartilham o temporizador. IA cooperativa não cobre todos os inimigos e
chefes. A corrida de trenzinho com o pássaro continua sem suspensão de J2, por decisão do usuário.

Compatibilidade com todas as GPUs, todos os eventos e funcionamento dessas novidades no Linux ainda não foram
integralmente validados. A recuperação automática entre APIs descrita acima foi implementada no Windows. Novos
backends têm limitações documentadas, incluindo MSAA indisponível nos caminhos novos; não prometer equivalência
universal entre APIs. Paredes do intro em 16:9 e resíduos ultrawide continuam pendências separadas.

A implementação de menus de viagem tem regressão automatizada, mas a documentação ainda marca o aceite visual
específico como pendente. Aprovações manuais dos demais recursos se limitam aos casos relatados, não a toda a campanha.

## Fora desta branch: mod de músicas

O mod de músicas MP3 em ZIP, com sincronização entre versões de superfície/subaquática e transições, está na branch
`feat/dev-mode`, nos commits `157b830` e `1ee45fb`. Foi conferido que esses commits **não pertencem à branch desta PR**.
Não o anunciar como incluído na próxima release sem verificar uma integração posterior. O player de música do
launcher presente nesta PR é um recurso separado. Não incluir músicas ou ZIPs privados na publicação.

## Validação já registrada antes desta nota

- Última revisão de código: commit `ab76739`; suíte obrigatória Windows **30/30**, build aprovado.
- Comandos registrados na mensagem daquele commit:
  `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Test -Jobs 12` e
  `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Build -Jobs 12`.
- Executável dessa revisão: `build-win/TooieRecompiled.exe`, SHA-256
  `4202a5b6672a5e5e6b9957b4e44722b2bec5e8e7305618abf8e478cc216c28b5`.
- Auditoria de documentação e `git diff --check` aprovados na revisão anterior.
- Esses resultados são anteriores ao merge e **não substituem a validação do novo build de release**.
- Esta nota altera somente documentação; não executa novamente o jogo, os testes de código ou o build.

## Retomada depois do merge

1. Confirmar pelo `gh` que a PR foi mesclada e registrar o SHA efetivo em `origin/main`. Não fazer o merge pelo usuário.
2. Atualizar o checkout de build de forma segura, preservando alterações locais. O sibling `E:\projetos\TooieRecomp`
   não deve ser confundido com o checkout Windows usado até aqui.
3. Consultar releases/tags atuais, escolher a versão e rever o diff desde a release anterior. Não inventar tag nem
   reutilizar um pacote antigo como build novo.
4. Seguir [o guia Windows](tools/windows/README.md) e
   [a spec de empacotamento](docs/specs/Tooie/packaging-and-dependencies.md). Gerar novo executável a partir do SHA
   mesclado, executar a suíte obrigatória e tratar qualquer falha antes de publicar.
5. Gerar o pacote de release a partir de worktree limpo, com
   `pwsh -NoProfile -File tools/windows/Build-Tooie.ps1 -Stage Package -ReleaseVersion <versão>`.
   `-Stage Test` não atualiza o executável: executar `-Stage Build` também.
6. Verificar o ZIP real com `tools/verify_package.py`, conferir `BUILD-INFO.txt`, versão, SHA do commit, manifestos,
   licenças, DLLs e SHA-256 do artefato. Excluir ROMs, saves, configs pessoais, músicas, texturas privadas e logs.
7. Revisar as notas finais a partir deste rascunho, mantendo as limitações e o limite Windows/Linux. Publicar os
   artefatos verificados somente na etapa de release combinada com o usuário. Não fabricar pacote Linux validado.
8. Depois da publicação, registrar URL, tag, SHA e hashes; retirar ou atualizar esta nota para não deixar um rascunho
   pendente enganoso. O texto publicado e o histórico Git preservam o changelog.

Referências atuais: [handoff](docs/HANDOFF.md), [testes e limites](docs/specs/Tooie/testing-and-oracle.md),
[inicialização e APIs](docs/specs/Tooie/startup-and-backends.md),
[cogitações futuras](docs/future/implementation-ideas.md).
