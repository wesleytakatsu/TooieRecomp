---
title: "Guia de uso do MCP markdown-vault"
status: "current"
area: "root"
updated: 2026-08-21
---

# Guia de uso do MCP markdown-vault

Este vault usa as tools do MCP `markdown-vault`
(`/home/wesley/projetos/markdown-vault-mcp-standalone`).

## Workspace e configs do projeto

Inicie Grok, Claude, Codex, OpenCode e VS Code em **`TooieRecomp/`**, não em
`N64-Recomp/`. O vault é só `docs/`.

| Cliente | Config no TooieRecomp |
| --- | --- |
| Grok / Claude Code | `.mcp.json` |
| Codex | `.codex/config.toml` |
| OpenCode | `opencode.json` |
| VS Code | `.vscode/mcp.json` |
| outros agentes | `.agents/mcp_config.json` |

Todos apontam:

```text
cwd:                 TooieRecomp/docs
MARKDOWN_VAULT_PATH: TooieRecomp/docs
```

O servidor Node continua fora do repo
(`markdown-vault-mcp-standalone`). Cópias antigas no pai `N64-Recomp/`
(`cwd` no monorepo) ficam só para sessões velhas; não as edite.

Sob a arquitetura standalone da fase P10, todas as dependências (`lib/`,
`decomp/`, `assets/`, `recompcontrollerdb.txt`, `tools/wheelhouse/`) vivem
vendorizadas diretamente em `TooieRecomp/`. O CMake e o build normal não usam
checkouts irmãos externos nem o BanjoRecomp. A ROM canônica (`Banjo-Tooie (USA).z64`,
git-ignored) fica na raiz do repositório ou indicada por `TOOIE_ROM`. Nenhuma ROM,
dado proprietário ou diretório vendorizado entra no vault de documentação.

## list_files

Descobre notas sem carregar conteúdo grande. Use `includeMetadata=true` para
ver tamanho e data.

## markdown_vault_diagnose_docs

Gate de saúde estrutural: `broken_links`, `missing_titles`, `duplicate_titles`,
`empty_files`, `orphan_notes`, `missing_frontmatter`, `large_files`.

## markdown_vault_lint

Mantém notas amigáveis para agentes: uma única H1 por nota, hierarquia sem
saltos bruscos, limite de ~30 KB.

## markdown_vault_generate_index

Atualiza o [VAULT.md](VAULT.md) após criar, dividir ou renomear notas.

## markdown_vault_get_backlinks

Antes de mover, dividir, renomear ou remover notas.

## Fluxo de manutenção

1. `list_files` para mapear a área.
2. Crie/edite notas com frontmatter e uma única H1.
3. Notas > 30 KB viram hub + partes.
4. Atualize [VAULT.md](VAULT.md) com `markdown_vault_generate_index`.
5. Rode `markdown_vault_diagnose_docs`.
6. Rode `markdown_vault_lint`.

## Modelo adotado pelos planos BT

Cada etapa em [plans/BT](plans/BT/index.md) mantém frontmatter com `title`,
`description`, `status`, `type`, `project`, `phase`, `order`, `depends_on` e
`tags`. Os títulos são únicos, cada arquivo contém somente uma H1 e os links
internos usam paths relativos ou wikilinks resolvíveis.

As ações usam checkboxes e IDs estáveis como `P03-T04`. Assim,
`markdown_vault_extract_tasks` produz uma fila de trabalho que continua
compreensível fora do arquivo original. A mudança de fase exige atualizar o
`status` do frontmatter, o gate no [plano mestre](plan/index.md) e anexar
evidências no documento da etapa.

Para agentes, o fluxo recomendado é:

1. `markdown_vault_generate_agent_briefing` com a tarefa atual.
2. `markdown_vault_build_context_pack` limitado à fase e às specs relacionadas.
3. `markdown_vault_extract_tasks` para selecionar o próximo ID pendente.
4. Implementação e registro da evidência na mesma nota.
5. `markdown_vault_audit` antes de fechar a fase.

## Critério de pronto

- `markdown_vault_diagnose_docs` retorna `issuesFound: 0`.
- `markdown_vault_lint` retorna `issuesFound: 0`.
- Nenhum `.md` em `docs/` passa de 30 KB.
