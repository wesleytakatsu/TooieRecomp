import { existsSync } from "node:fs"
import { join } from "node:path"
import type { Plugin } from "@opencode-ai/plugin"

export const ProjectCompaction: Plugin = async ({ worktree }) => {
  const enabledFile = join(worktree, ".opencode", "strict-compaction.enabled")

  return {
    "experimental.session.compacting": async (_input, output) => {
      if (!existsSync(enabledFile)) return

      output.prompt = `
Gere um resumo de continuacao da sessao.

DESCARTE:
- raciocinio intermediario
- hipoteses ja refutadas, exceto na secao REFUTED
- tentativas de comandos
- outputs completos
- explicacoes repetidas

PRESERVE SOMENTE estas secoes, exatamente nesta ordem:

CURRENT_HEAD
OBJECTIVE
PROVEN
REFUTED
UNKNOWN
CURRENT_RUNTIME_IDENTITIES
CURRENT_BREAKPOINTS
IMPORTANT_ADDRESSES
LATEST_RAW_EVIDENCE
FILES_CHANGED
TEST_RESULTS
CURRENT_HYPOTHESIS
NEXT_SINGLE_ACTION
DO_NOT_REOPEN

Regras:
- Registre apenas o estado atual e evidencias relevantes.
- Nao invente informacoes.
- Use NONE quando uma secao nao tiver conteudo.
- NEXT_SINGLE_ACTION deve conter somente uma acao concreta.
- Nao escreva texto fora dessas secoes.
`.trim()
    },
  }
}
