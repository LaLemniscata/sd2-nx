# Fluxo de trabalho e economia de contexto

Referência para o LaLemniscata e para o assistente. O resumo operacional está no as regras do projeto.

## Por que importa
O as regras do projeto e todo o histórico da sessão são reenviados a cada mensagem. Contexto parado
custa tokens em todo turno e piora a qualidade das respostas. Neste projeto, as maiores fontes de
desperdício são dumps do Il2CppDumper/jadx, logs do loader e crash reports.

## Hábitos de sessão
| Situação | Ação |
|---|---|
| Começar uma sessão | Pedir: "leia `docs/STATUS.md` e continue". Não colar histórico antigo. |
| Trocar de assunto (ex.: de JNI para controles) | `/rename <nome>` e depois `/clear`. Retomar com `/resume`. |
| Tarefa longa ficando pesada | `/compact` dizendo o que manter (ver as regras do projeto, "When compacting"). |
| Fim da sessão | Pedir ao assistente para atualizar `docs/STATUS.md` antes de sair. |
| Mudança grande ou nova fase | Plan mode: o assistente propõe, o LaLemniscata aprova, depois edita. |

## Modelo por tarefa
- **Opus:** diagnosticar crash, decidir patches no `libunity`, desenhar a camada JNI, escolher a base.
- **Sonnet:** stubs repetitivos, Makefile, README, scripts em `tools/`, edições mecânicas.
- Na dúvida, começar no Sonnet e subir para o Opus se o problema exigir raciocínio sobre o binário.

## Dumps, logs e crash reports
- Nunca ler `dump.cs`, `script.json`, a saída do jadx ou logs inteiros.
- Buscar com `rg -n "NomeDaClasse|NomeDoMetodo" dumps/` e ler só as linhas encontradas.
- Crash report do Atmosphère: enviar o arquivo; o assistente extrai o backtrace e o PC, sem ler tudo.
- Varreduras amplas (ex.: "quais SDKs de anúncio existem no dump?") vão para um subagente Explore.
- Conclusões viram linhas em `docs/recon.md` ou `docs/jni_calls.md`, para não refazer a busca.

## Ciclo de teste no console
1. assistente compila e diz: o que mudou, o que testar, o que observar, quais arquivos enviar.
2. LaLemniscata copia o `.nro` (SD ou `nxlink`), abre por title override e testa.
3. LaLemniscata envia `log.txt` e, se houver, o crash report mais recente de `atmosphere/crash_reports/`.
4. assistente corrige e registra o resultado no `STATUS.md`.

## O que não usar (por enquanto)
- **Obsidian/MCP de notas:** a memória do projeto vive em `docs/`, versionada com o código.
  Uma segunda fonte de verdade cria divergência, e cada servidor MCP adiciona definições de
  ferramentas ao contexto.
- **Skills customizadas:** criar só depois de 3 ou 4 ciclos de debug, se a análise de crash report
  ficar repetitiva. Aí ela vira uma skill com o procedimento real, não um palpite.
