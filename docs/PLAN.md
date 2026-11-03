# sd2-nx — Soda Dungeon 2 para Nintendo Switch (homebrew)

> Plano mestre do projeto (referência). As regras que valem em toda sessão estão no as regras do projeto
> da raiz, que é curto de propósito. Leia aqui só a seção da fase em que está trabalhando.

## 0. Objetivo

Rodar a build **Android ARM64 original** do *Soda Dungeon 2* (Unity, IL2CPP — a confirmar na Fase 0)
no Nintendo Switch com CFW (Atmosphère), via **wrapper/loader nativo** no estilo dos ports da
linhagem so-loader (TheOfficialFloW → Andy Nguyen / fgsfds → ports Unity como Zookeeper DX,
Very Little Nightmares, The Room: Old Sins, CloverPit).

- Pacote Android: `com.armorgames.sodadungeon2` (Armor Games Studios / Afro-Ninja Productions).
- Resultado: `sd2_nx.nro` + instruções. O usuário fornece os arquivos do jogo a partir da **própria cópia**.
- Totalmente offline. Experiência pensada para Switch (portátil e dock), não "celular no Switch".

Equipe: **LaLemniscata** (decisões, hardware, testes no console) + **assistente** (código, pesquisa, documentação).

### Comunidade de referência: Switch Port Zone (Discord)
O projeto segue as regras do servidor, para poder ser apresentado e receber suporte lá:
- **Inglês** em toda comunicação pública: README, releases, issues, commits e posts no servidor.
  Docs internas (`docs/`) podem ficar em português.
- **Antipirataria:** arquivos do jogo **somente de fonte oficial**, extraídos da cópia instalada pela
  **Google Play** no aparelho do LaLemniscata. Nunca baixar, sugerir nem citar sites de APK de terceiros
  (APKMirror, mod APKs etc.), nem para pesquisa de versões.
- **Port nativo, não Carafe/Autorun:** wrapper so-loader rodando o binário ARM64 diretamente.
  Nada de Wine, Box64/FEX ou tradução x86. O desempenho tem que ser bom **sem overclock**.
- **Respeito à autoria:** nunca apresentar trabalho de outro Port Maker como nosso. Código herdado
  mantém licença, cabeçalhos e créditos. Partes reaproveitadas de outro port são citadas no README
  com link.
- **Canais certos (servidor reorganizado em 02/11/2026).** Três seções: **arm64-v8a** (64-bit),
  **armeabi-v7a** (32-bit) e **recomps / source / emulation**. Cada uma tem `guide`, `planned`,
  `completed`, `requests` e `discussion`. Os canais antigos (ex.: `#v7-requests`, `port-requests-old`)
  foram arquivados. O sd2-nx pertence à seção da ABI confirmada na Fase 0.
  - Dúvidas e progresso: `discussion` da seção. Pedidos de port: só em `requests`.
  - **Nunca pedir port nem ajuda individual por DM ou fora dos canais de pedido** (regra nova de 02/11/2026).
    Perguntas técnicas a Port Makers vão em `discussion`, abertas a todos.
- **Antes de começar:** ler o `guide` da seção certa e conferir `planned` e `completed`, para não
  duplicar um port que alguém já faz ou já terminou.

---

## 1. Regras de trabalho

Ficam no as regras do projeto da raiz (carregado em toda sessão). Hábitos de sessão e economia de contexto
ficam em `docs/WORKFLOW.md`.

---

## 2. Escopo

### Dentro
- Boot do jogo, gráficos (GLES/EGL via mesa/nouveau), áudio, save local persistente.
- Controles: tela touch nativa + Joy-Con/Pro Controller (toques injetados + cursor), portátil e dock.
- Resolução configurável (720p/1080p) e framerate estável.
- Tratamento de suspend/sleep, foco (menu HOME) e saída limpa.
- `config.txt` documentado, gerado no primeiro boot.
- Toda funcionalidade online **cortada de forma graciosa** (seção 6).

### Fora
- Anúncios, compras, login, cloud save, analytics, notícias e configuração remotas, links externos.
- Distribuição de qualquer arquivo do jogo.
- Instalação como NSP/forwarder (opcional e futura; só com aprovação do LaLemniscata).

---

## 3. Ambiente

- Host: WSL2 Ubuntu 24.04 (máquina do LaLemniscata).
- Toolchain: **devkitPro (devkitA64 + libnx)**, instalado pelo `dkp-pacman` seguindo a documentação oficial
  (devkitpro.org, página do devkitPro pacman). Conferir o procedimento atual na doc antes de instalar.
  - Grupo base: `switch-dev`. Pacotes extras: exatamente os listados no README do repositório-base
    escolhido na Fase 1 (tipicamente `switch-mesa`, `switch-libdrm_nouveau`, `switch-sdl2` etc.).
    Não adivinhar.
  - Em todo shell de build: `source $DEVKITPRO/switchvars.sh` quando o Makefile-base exigir.
- Ferramentas de análise (só no host): `adb`, `unzip`, `binutils` (`readelf`, `nm`, `objdump`),
  **jadx** (DEX → Java), **Il2CppDumper** (libil2cpp.so + global-metadata.dat → tipos/métodos),
  opcionalmente Ghidra.
- Deploy: copiar via SD/MTP ou `nxlink` (hbmenu com netloader). Logs também via `nxlink -s`, quando a base suportar.
- Execução: **sempre por title override** (segurar R ao abrir um jogo instalado) ou forwarder.
  Applet/álbum não tem memória suficiente para Unity.

Estrutura do repositório:
```
sd2-nx/
├── regras do projeto            # regras curtas + ponteiros (carregado em toda sessão)
├── source/              # loader + shims (herdado + novo)
├── docs/
│   ├── PLAN.md          # este plano completo
│   ├── WORKFLOW.md      # hábitos de sessão e economia de contexto
│   ├── STATUS.md
│   ├── DECISIONS.md
│   ├── recon.md         # resultados da Fase 0/2
│   ├── jni_calls.md     # inventário de chamadas Java↔nativo
│   └── test_matrix.md
├── tools/               # scripts de extração/verificação (não incluem dados do jogo)
├── game/                # (gitignored) arquivos extraídos do APK do LaLemniscata
├── dumps/               # (gitignored) saídas do Il2CppDumper/jadx
├── Makefile
├── README.md
└── LICENSE              # MIT, herdada + créditos
```

---

## 4. Fases

### Fase 0 — Viabilidade (GATE: nada avança sem isto)

Executado pelo LaLemniscata com apoio do assistente:
```bash
adb shell pm list packages | grep -i soda          # confirmar com.armorgames.sodadungeon2
adb shell pm path com.armorgames.sodadungeon2      # pode retornar várias linhas (split APKs)
adb shell dumpsys package com.armorgames.sodadungeon2 | grep -E "versionName|versionCode"
mkdir -p game/apk && adb pull <cada caminho retornado> game/apk/
for f in game/apk/*.apk; do echo "== $f"; unzip -l "$f" | grep -E "lib/|global-metadata|data.unity3d|\.obb"; done
```

Verificar e registrar em `docs/recon.md`:
| Pergunta | Como verificar | Resultado necessário |
|---|---|---|
| Existe `lib/arm64-v8a/`? | listagem acima (base ou `split_config.arm64_v8a.apk`) | **arm64-v8a:** segue este plano. **Só armeabi-v7a:** não mata o projeto (portes 32-bit nativos passaram a existir em 02/11/2026), mas muda base e bibliotecas: pausar, ler o `v7-guide` e replanejar as Fases 1–3 com o LaLemniscata antes de seguir |
| Backend IL2CPP? | `libil2cpp.so` + `assets/bin/Data/Managed/Metadata/global-metadata.dat` | sim (Mono = replanejar) |
| Versão do Unity | `strings libunity.so \| grep -E "^20[0-9]{2}\.[0-9]+\.[0-9]+"` e/ou cabeçalho do `data.unity3d` | anotar exata |
| Outras `.so` | lista completa de `lib/arm64-v8a/` | anotar (FMOD? Burst? SDKs de anúncio?) |
| Metadata ofuscada? | Il2CppDumper roda sem erro? | sim (ofuscação = replanejar) |
| Versão do jogo | `versionName`/`versionCode` + SHA-256 de cada `.so` | fixar como **versão suportada** |

**Saída da fase:** `docs/recon.md` preenchido e decisão GO/NO-GO registrada com o LaLemniscata.

### Fase 1 — Escolha e preparo da base

1. Candidatos (todos MIT, wrappers Unity/IL2CPP ARM64 para Switch):
   - Zookeeper DX (ChanseyIsTheBest): Unity 2022.3.x. Tem JNI Unity, bridges GLES/EGL e de áudio,
     backend de PlayerPrefs, input com cursor e patcher do libunity.
   - Very Little Nightmares e The Room: Old Sins: Unity 2021.3.
   - CloverPit: Unity 6 (GC bridge, patches de engine, FMOD).
2. Critério: **a versão de Unity mais próxima da do SD2** e, em segundo lugar, as mesmas libs auxiliares
   (ex.: FMOD).
3. Achar o repositório **pelo link "Source" da página do port no GameBrew/GitHub**. Confirmar com o
   LaLemniscata a URL antes de clonar, e conferir a licença (MIT) e a lista de dependências no README.
   Antes, o LaLemniscata lê o `guide` da seção da ABI e confere `planned`/`completed` (o SD2 já pode estar
   sendo feito). Na dúvida entre bases, ele pergunta no `discussion` da seção (em inglês), nunca por DM.
4. Fork → renomear para `sd2_nx` (título, autor, caminho `sdmc:/switch/sd2_nx/`, ícone **original**,
   não usar arte do jogo no repositório).
5. Compilar a base sem modificações (prova de toolchain) e só então começar a adaptar.

**Saída:** repositório compila, `.nro` gerado, créditos e LICENSE no lugar.

### Fase 2 — Reconhecimento do binário

1. **Il2CppDumper** em `libil2cpp.so` + `global-metadata.dat` → `dumps/`. Mapear classes relevantes:
   ads (ex.: Unity Ads, AdMob, AppLovin, ironSource; descobrir quais, não supor), IAP (Unity IAP / Google
   Billing), analytics, login/cloud, `PlayerPrefs`/arquivos de save, `Application.OpenURL`,
   `Application.internetReachability`, rede (`UnityWebRequest`).
2. **jadx** no DEX: listar plugins Java e todos os métodos `native` e chamadas que o lado Java faz,
   para prever o JNI que o nativo vai pedir.
3. `readelf -d` / `nm -D` em cada `.so`: dependências e imports que o loader precisa resolver
   (libc, libm, liblog, libandroid, libEGL, libGLESv2/3, libOpenSLES, libz etc.).
4. Gerar `docs/jni_calls.md`: tabela `classe.método(assinatura) → comportamento no port`.
5. **Os dumps são enormes.** Nunca abrir `dump.cs` nem a saída do jadx por inteiro: consultar com
   `rg`/`grep` por classe ou método. Varreduras amplas vão para um subagente Explore, que devolve só a
   conclusão (ver `docs/WORKFLOW.md`).

**Saída:** inventário completo, sem chute. Pontos ambíguos viram perguntas ao LaLemniscata.

### Fase 3 — Primeiro boot

1. Ajustar o loader: carregar `libmain.so`, `libunity.so`, `libil2cpp.so` (+ extras) de `sdmc:/switch/sd2_nx/`.
2. Implementar ou adaptar o JNI falso para as classes do Unity (`UnityPlayer` etc.) e stubs de **todos** os
   plugins do inventário (seção 6).
3. Resolver imports faltantes com shims, um por vez, guiado por log. Nunca usar stub "silencioso" que
   retorna lixo: todo stub loga a primeira chamada.
4. Patches no `libunity.so` só quando necessários, identificados por padrão de bytes (fingerprint), nunca por
   offset fixo sem verificação, e falhando com mensagem clara se o padrão não for achado.
5. Ciclo: build → LaLemniscata testa → logs → correção. Meta: chegar à tela título.

**Saída:** jogo abre e chega ao menu principal no console.

### Fase 4 — Jogo jogável de ponta a ponta

- Iniciar save novo, completar tutorial, jogar dungeon, voltar à cidade, salvar, fechar e reabrir com o
  progresso mantido.
- Áudio (música + SFX) sem estalos nem dessincronia.
- Progresso offline (AFK / Battle Credits) funcionando com o relógio do sistema.

### Fase 5 — Adaptação para Switch (qualidade)

- **Controles:**
  - Touch: repasse direto (multitouch, se o jogo usar).
  - Controle: cursor virtual (analógico esquerdo) + A = toque; atalhos de botões injetando toques em
    posições fixas da UI **somente depois de levantar o layout real das telas** com o LaLemniscata.
    O mapa fica em `docs/controls.md` e é aprovado por ele antes de implementar.
  - Cursor: visível por padrão no dock e oculto no portátil, alternável (padrão dos ports Unity). PNG próprio opcional.
  - B = "voltar do Android" se o jogo tratar `KEYCODE_BACK`. Confirmar o comportamento antes.
- **Vídeo:** resolução por modo (portátil 720p, dock 1080p por padrão, configurável), troca dock/portátil ao
  vivo, proporção correta (confirmar se o jogo usa paisagem ou retrato e decidir com o LaLemniscata como tratar no
  dock), vsync, meta de 60 fps se o jogo suportar; senão 30 fps estável.
- **Sistema:** pausar ou retomar o Unity no foco/suspend (menu HOME, sleep), salvar antes de sair,
  saída limpa sem crash.
- **Performance:** medir no console em modo portátil, 720p e clocks padrão, que é a referência para o Switch Lite; documentar se algum overclock ajuda, mas o padrão deve rodar bem sem OC.
  O Lite não tem dock: tudo precisa funcionar só em portátil. Sem um Lite para testar, o resultado vale como aproximação feita no OLED e é documentado assim.
- **config.txt** gerado no primeiro boot com comentários: resoluções, fps, cursor, sensibilidade, logging.

### Fase 6 — QA

`docs/test_matrix.md` com resultado por item (portátil/dock):
- boot frio, boot com save existente, save corrompido ou ausente (o jogo não deve travar);
- 1 h de sessão contínua; sleep/wake várias vezes; HOME e volta; dock ↔ portátil durante o jogo;
- todos os menus acessíveis só com controle; tudo acessível só com touch;
- nenhum caminho que trave esperando rede (abrir loja, botão de anúncio, links, conquistas).

### Fase 7 — Release

- README: requisitos (CFW Atmosphère, title override), **versão suportada exata do jogo**, como
  extrair os arquivos da própria cópia, estrutura de pastas, controles, config, problemas conhecidos e créditos.
- Script em `tools/` que valida os arquivos extraídos pelo usuário (SHA-256 da versão suportada) e avisa se
  a versão for diferente.
- README, notas de release e o post no Switch Port Zone **em inglês**. A instrução de obtenção dos
  arquivos cita **apenas a Google Play** como fonte.
- Seção de créditos com cada port e projeto da linhagem usado (nome, autor, link, licença).
- Publicação (GitHub Releases / GameBrew / Switch Port Zone) **só com decisão explícita do LaLemniscata.**

---

## 5. Estrutura no cartão SD (alvo)

```
sdmc:/switch/sd2_nx/
├── sd2_nx.nro
├── libmain.so          # da cópia do usuário (lib/arm64-v8a/)
├── libunity.so         # idem
├── libil2cpp.so        # idem
├── <outras .so>        # conforme Fase 0
├── assets/             # do APK base
├── config.txt          # gerado no 1º boot
├── save/               # PlayerPrefs + arquivos de persistentDataPath
└── log.txt             # quando logging ativo
```
(Ajustar à realidade da Fase 0; se o APK vier dividido, documentar de qual split sai cada arquivo.)

---

## 6. Corte de conectividade (comportamento definido)

| Subsistema | Comportamento no port |
|---|---|
| Rede em geral | `internetReachability` = NotReachable; `UnityWebRequest` falha rápido com erro de rede (nunca pendura) |
| IAP | Billing "indisponível"; nenhuma compra jamais concluída |
| Anúncios (inclusive premiados) | SDK nunca inicializa; "anúncio indisponível" no caminho de erro do próprio jogo. **⚠ Decisão pendente do LaLemniscata** sobre o que fazer se algum fluxo exigir anúncio para continuar |
| Login / cloud save / cross-save | Desativado; apenas save local |
| Analytics / crash reporting | No-op |
| Notícias, remote config, eventos online | Jogo usa os valores padrão locais; se travar o boot, aplicar patch mínimo documentado |
| Abrir links (Discord, loja, site) | No-op (opcional: toast/log) |
| Notificações push / locais | No-op |

Regra: cada linha só é marcada como feita depois de testada no console (Fase 6).

---

## 7. Perguntas abertas para o LaLemniscata

Responder antes ou durante as fases indicadas:
1. ~~(Fase 0) Firmware do Switch e versão do Atmosphère?~~ Respondida: OLED, firmware 23.0.1, Atmosphère 1.12.1 (ver `docs/recon.md`).
2. (Fase 0) Versão do SD2 instalada no celular (`versionName`/`versionCode`).
3. ~~(Fase 1) Aprovar o repositório-base escolhido e a URL.~~ Respondida: `cloverpit_nx` (ver `docs/DECISIONS.md`).
4. ~~(Fase 5) Preferência de controles.~~ Respondida: mapa completo em `docs/controls.md`.
5. (Fase 5) Resolução e fps padrão por modo.
6. (Fase 6) Política para fluxos com anúncio premiado (ver seção 6).
7. (Fase 7) Publicar ou manter privado?
8. Tem sys-clk instalado? (Só para testes de performance; o padrão não deve depender dele.)

---

## 8. Riscos conhecidos

| Risco | Impacto | Mitigação |
|---|---|---|
| Sem `arm64-v8a` (só v7) | Base e libs diferentes, replanejamento | Gate da Fase 0; seguir o `v7-guide` |
| Alguém já está portando o SD2 | Trabalho duplicado | Conferir `planned`/`completed` antes da Fase 1 |
| Pedir ajuda fora dos canais/por DM | Violação de regra do servidor | Só `discussion` e `requests` da seção certa |
| Instalar como NSP/forwarder | Risco de ban online da Nintendo | Padrão: `.nro` via title override; forwarder só com aprovação |
| Port visto como "não real" ou sem crédito | Recusa/ban na comunidade | Seguir as regras do Switch Port Zone (seção 0) |
| Metadata IL2CPP ofuscada/criptografada | Recon muito mais difícil | Avaliar na Fase 0 e decidir com o LaLemniscata |
| Versão de Unity sem base próxima | Mais patches de engine | Escolher a base mais próxima; documentar diferenças |
| Jogo exige servidor para bootar | Patch de lógica necessário | Identificar na Fase 2; patch mínimo e documentado |
| Atualização do jogo muda binários | Port quebra | Fixar a versão suportada + verificação SHA-256 |
| Memória insuficiente | Crash | Somente title override; monitorar uso |

---

## 9. Depuração no console

- Log do loader: `sdmc:/switch/sd2_nx/log.txt` (ativado pelo `config.txt`) e/ou `nxlink -s`.
- Crash do sistema: relatórios do Atmosphère em `sdmc:/atmosphere/crash_reports/` (enviar o mais recente).
- Ao pedir um teste, o assistente informa: o que mudou, o que testar, o que observar e quais arquivos enviar.
- Logs e crash reports grandes: ler só o trecho relevante (backtrace, últimas ~100 linhas, linhas com
  `error`/`missing`/`not found`), nunca o arquivo inteiro de uma vez.

---

## 10. Definição de "pronto"

O jogo roda offline do boot ao fim de uma sessão longa, em portátil e dock, jogável inteiro só com
controle ou só com touch, sem travar em nenhum fluxo de rede, com save persistente, 60 fps (ou 30 estável,
se for o limite do jogo), sem nenhum arquivo do jogo no repositório, com README completo e todos os itens
da `test_matrix.md` aprovados no console.
