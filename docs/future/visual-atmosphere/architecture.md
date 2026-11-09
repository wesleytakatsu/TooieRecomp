---
title: "Arquitetura proposta da atmosfera visual"
description: "Perfis por sala, relógio visual, integração raster, recursos por câmera e limites de compatibilidade."
status: proposed
type: design-plan
project: TooieRecomp
updated: 2026-11-07
tags: [future, atmosphere, architecture, compatibility]
---

# Arquitetura proposta da atmosfera visual

Estado: `HYPOTHESIS` / `NOT_IMPLEMENTED`. [Guia](README.md), [perfis](scenario-profiles.md) e
[aceite](validation.md) definem escopo e validação. Os nomes abaixo são conceitos, não APIs ou schemas existentes.

## Contexto e perfis

Manter um contexto cosmético do mundo com hora visual, vento, transição de clima e geração do mapa. Resolver o perfil
por identidade confirmada de mapa, sala e região, incluindo condições nativas relevantes. Um mundo tem diversos
interiores, subambientes e exceções; o nome da fase não basta para decidir exposição à chuva ou passagem solar.

O perfil propõe: efeitos permitidos, volumes de emissão/exclusão, exposição ao céu, fontes de luz, materiais,
vegetação elegível, áudio e limites de qualidade. Regras específicas prevalecem sobre o perfil geral. Mapa, região
ou modo sem cadastro ficam com aparência nativa. Os IDs e volumes reais precisam ser inventariados, sem inventar
endereços ou adotar nomes de fase como identificadores técnicos definitivos.

## Prioridade e composição do estado

Resolver nesta ordem: desligamento explícito; capacidade técnica e modo elegível; estado nativo/roteirizado;
região específica; perfil geral; variação cosmética de hora/clima; intensidade escolhida. A intensidade nunca
reativa efeito vetado por teto, modo, material ou capacidade. Estado nativo e save são lidos quando necessário,
sem gravações para fabricar um perfil compatível.

Primeiro inventariar clima, partículas, luz e sons nativos por região. Decidir explicitamente entre preservar,
complementar ou substituir a apresentação de um efeito equivalente. Se não houver forma segura de separar o efeito
nativo de seus eventos de jogo, preservar o nativo e adiar a substituição. Não adicionar chuva/poeira/sons duplicados.

## Relógio e estado

- Um relógio cosmético compartilhado; câmeras não avançam o mundo separadamente.
- Atualização em passo controlado, com interpolação para apresentação; 30, 60 e 120 FPS não mudam duração dos ciclos.
- Usar gerador aleatório próprio para efeitos. Não consumir o RNG do jogo nem alterar temporizadores, IA ou eventos.
- Proposta inicial: meio-dia ao iniciar sessão, sem sincronização com relógio real e sem gravação no save nativo.
- Pausa, menu e cenas roteirizadas congelam relógio cosmético e emissão. Retomada não recupera tempo acumulado.
- Troca de sala preserva hora da sessão, encerra emissores e invalida recursos da geração anterior. Interior ignora
  mudanças solares locais, mas não cria um segundo relógio.
- Clima e hora podem ser fixados durante validação. Seed e ponto de câmera devem permitir comparação reproduzível.

Congelamento cosmético não modifica a política de pausa do guest. Identificar quais menus realmente pausam o jogo;
congelar somente a camada nova não autoriza pausar física/áudio nativos. Ao retomar, reiniciar a medição do intervalo,
sem emitir lote de partículas atrasadas. Teleporte ou mudança grande da câmera renova volumes móveis de chuva
sem esticar rastros nem interpretar deslocamento como rajada.

O campo de vento é dado compartilhado da base; deformação de árvores, chuva e folhas são consumidores independentes.
Um consumidor pode usar vetor constante no piloto. Nenhum efeito precisa esperar todos os demais estarem implementados.

## Camadas de renderização a investigar

| Camada candidata | Responsabilidade proposta | Dependência ainda não comprovada |
| --- | --- | --- |
| Contexto do jogo | Identidade de mapa, sala, condições e tempo de atualização | Fronteira segura entre update nativo e apresentação |
| Classificação de desenhos | Separar água, vegetação, emissores, céu, mundo e HUD | Tags estáveis por recurso/instância/passe; um hash sozinho não identifica todo uso |
| Geometria e materiais raster | Vento, luz complementar e relevo aparente | Normais, UVs, pivôs, máscaras e buffers próprios quando necessários |
| Partículas | Chuva, poeira, folhas e outros efeitos com profundidade | Integração, clipping, transparência e exposição por região |
| Composição | Cores, bloom, neblina e sombras de contato | Cor/profundidade corretas, ordenação e separação do HUD |
| Áudio ambiente | Emissores, loops e transições coerentes | Mixagem e propriedade independentes da substituição musical |

O arquivo `PostProcessPS.hlsl` existente não basta para afirmar que há uma cadeia geral pronta para esses efeitos.
O encaixe raster deve ser confirmado no código ativo de cada backend. Não usar a rota de ray tracing como dependência.
Não modificar cores/profundidade destinadas a leituras do guest sem demonstrar preservação de sua semântica.

Texturas fornecem identidade de imagem; um mesmo recurso pode aparecer em contextos diferentes. Classificação
precisa combinar contexto e uso quando necessário. Os efeitos não podem depender exclusivamente da cor dos pixels.

## Coerência entre atualização e quadro apresentado

Associar contexto a geração de mapa, identidade de câmera/instância e amostra temporal válida. Cor, profundidade,
projeção, pose e máscaras usadas juntos precisam representar o mesmo quadro/passe apresentado. Profundidade lida
do guest, potencialmente atrasada ou de outra viewport, não deve ser tratada como profundidade moderna pronta
para refração, oclusão ou partículas. Ver contratos de [gráficos](../../specs/Tooie/graphics-and-presentation.md).

Identidades temporárias, ordem de desenho, ponteiros reciclados e hashes de textura isolados não são chaves
universais de material/instância. Confirmar o conjunto mínimo de identificadores e invalidar caches por geração.
Informação ausente implica efeito desligado no recorte. Não tentar corrigir classificações mudando o pareamento
ou enfraquecendo guardas de interpolação já existentes.

Consumo na thread gráfica usa snapshot coerente, com vida útil explícita; mutações de perfil/configuração entram
em fronteira segura. Não consultar ou executar funções guest da thread gráfica sem contrato demonstrado, nem
introduzir espera sincronizada da GPU para classificar cada desenho/partícula.

## Câmeras, memória e transições

No split-screen, hora e clima são compartilhados; exposição, névoa submersa, buffers temporais, reflexos, clipping,
partículas próximas e pós-processamento pertencem à respectiva viewport. Um jogador sob teto pode ver chuva do lado
de fora enquanto o outro está exposto. Não alternar foco global para resolver esses efeitos.

Emissores de mundo têm identidade estável e não devem emitir duas vezes porque existem duas câmeras. Volumes móveis
de partículas por câmera precisam evitar duplicação visual na interseção. Luz, deformação e cores não podem escrever
no mesmo buffer mutável de duas instâncias. Histórico temporal é invalidado em corte de câmera, resize, Warp,
mudança de mapa e mudança de configuração incompatível.

## Contratos propostos

| ID | Requisito |
| --- | --- |
| ATM-01 | Configuração mestre e controles por efeito, inicialmente desligados até aceite do respectivo piloto; desligado mantém o caminho nativo, sem trabalho gráfico residual relevante. |
| ATM-02 | Perfil explícito por sala/região; contexto desconhecido retorna ao nativo. |
| ATM-03 | Tempo cosmético e aleatoriedade independentes de FPS e simulação nativa. |
| ATM-04 | Dono explícito para recursos de mundo, instância e câmera, com limpeza por geração. |
| ATM-05 | Não alterar colisão, progressão, timers, flags, navegação de NPCs ou eventos por causa do clima. |
| ATM-06 | Separar capacidades por backend e oferecer fallback por efeito, sem falha total por recurso opcional. |
| ATM-07 | Fontes temporárias respeitam teto, salas e profundidade; ausência de máscara não autoriza emitir em todo lugar. |
| ATM-08 | Preservar HUD, legendas e legibilidade; disponibilizar redução de partículas, flashes e movimento ambiental. |
| ATM-09 | Prioridade entre capacidades, modos, estados nativos e perfis explícita; sem duplicação de apresentação equivalente. |
| ATM-10 | Classificação e caches distinguem contexto, instância e geração, sem adotar endereço/hash/ordem isolados como identidade universal. |
| ATM-11 | Cor, profundidade, pose e máscaras coerentes com viewport e tempo do quadro apresentado, inclusive interpolado. |
| ATM-12 | Publicação e descarte seguros entre threads; nenhum novo bloqueio de GPU/guest por consulta cosmética. |

## Plataformas e propriedade da implementação

Prototipar primeiro em um backend desktop comprovadamente funcional na máquina de teste. Testar os demais
[backends suportados](../../specs/Tooie/startup-and-backends.md) separadamente. Para
[portáteis](../../specs/Tooie/handheld-portmaster.md), prever menos partículas, texturas menores e efeitos desligados
quando faltarem recursos ou orçamento. DDS/BC7 de desktop não é garantia de suporte em OpenGL ES.

Integração de Tooie fica no código-fonte do projeto; alterações em RT64/RecompFrontend seguem patches e manifesto
de `vendor/`. Nunca editar `RecompiledFuncs/**` manualmente. Não relaxar culling nativo para obter reflexos ou
partículas; uma passagem adicional exige desenho próprio e validação, não reabertura de soluções refutadas.

Próxima investigação técnica: identificar fronteiras de frame, submissão de mundo/HUD, identidade de sala e
descartes de recursos antes de escolher hooks ou publicar um formato de perfil.

A implementação segue provas delimitadas do [roteiro](implementation-roadmap.md) e orçamento de
[qualidade](quality-presets.md). Relações artísticas entre páginas não são automaticamente pré-requisitos técnicos.
