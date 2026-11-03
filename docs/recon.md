# Recon (Fase 0 e Fase 2)

Preencher com resultados verificados. Nada aqui é suposição.

## Fase 0 — Viabilidade

| Item | Resultado | Como foi verificado |
|---|---|---|
| Pacote | `com.armorgames.sodadungeon2`; instalador `com.android.vending` (Google Play) | `pm path`, `dumpsys package` |
| versionName / versionCode | `1.2.21` / `140` (minSdk 23, targetSdk 35) | `dumpsys package` |
| Split APKs | `base.apk` + `split_config.arm64_v8a.apk`; SHA-256 local igual ao do celular | `pm path`, `sha256sum` nos dois lados |
| ABIs presentes (`lib/*/`) | só `arm64-v8a` | listagem dos APKs com `zipfile` do Python (sem `unzip` no WSL) |
| Backend (IL2CPP / Mono) | IL2CPP: `libil2cpp.so` + `assets/bin/Data/Managed/Metadata/global-metadata.dat` | listagem dos APKs |
| Versão do Unity | `6000.0.73f1` (`a166abc3bf0e`) | `strings libunity.so` e cabeçalho do `data.unity3d` |
| Outras `.so` | só `libmain.so`, `libunity.so`, `libil2cpp.so`; sem FMOD nem Burst | listagem dos APKs |
| Metadata ofuscada? | Sem sinal de ofuscação. **Il2CppDumper ainda não foi rodado** | leitura própria: magic `0xFAB11BAF`, versão 31, 31 pares offset/size válidos, 85.134 strings (99,9% legíveis), `Assembly-CSharp.dll` presente |
| SHA-256 de cada `.so` | ver abaixo | `sha256sum` |
| Firmware do Switch / Atmosphère | Firmware 23.0.1 e Atmosphère 1.12.1; modelo OLED. Versões deduzidas, não lidas no console | LaLemniscata diz estar na última; página de updates da Nintendo e releases do Atmosphère, consultadas em 2026-11-02 |
| Port do SD2 já em `planned`/`completed`? | Não aparece em nenhum dos dois | LaLemniscata, no Switch Port Zone |

### Versão suportada (candidata)
```
4f79947311560099b70cee6fab95cd9f08d6c6014fae4f822b33155bda25f0dc  base.apk
bc3ece4fd9fb6445942ede7ade0a036f36174cf9a8ae32197acc776a2b7dd151  split_config.arm64_v8a.apk
dfd30a66af009029f1b96c9886d9656730a3cb120eb51b96fdc049f58a3f6148  libmain.so
1a2904c469bb8bffbe737e183ec965992cdf4e52c47d5c23ec2af83d4d28d2d0  libunity.so
68cafaa2672ce58c502b1479071c50c3e161549ec228032ceb0fe54e9cdae100  libil2cpp.so
eb38b988a1ef1d025c14a87b3f9f45398742e45382d06d3c50468c31854882b1  global-metadata.dat
```

### Dependências declaradas (`readelf -d`)
- `libmain.so`: liblog, libm, libdl, libc
- `libil2cpp.so`: liblog, libm, libdl, libc
- `libunity.so`: libmain, libandroid, liblog, libz, libEGL, libmediandk, libm, libdl, libc

**Decisão GO/NO-GO:** GO, confirmado pelo LaLemniscata em 2026-11-02.
Ressalva: rodar o Il2CppDumper no início da Fase 2 (metadata v31, Unity 6).
Nota: as notas do Atmosphère 1.12.0 citam suporte a 23.0.0; não foi vista menção explícita a 23.0.1 nas notas do 1.12.1, que é posterior a ele.

## Fase 1 — Base

Base: `cloverpit_nx`, commit `1785b26bc28680063dbcbb15199d090c276b89ef` (MIT, Unity 6000.2.6f2, CloverPit 1.4.8).
Build sem alterações com devkitA64 16.1.0 e libnx 4.12.0: 0 erros, 23 avisos.

| Item | Situação na base | Ação |
|---|---|---|
| Identidade (`CS_*`, `GAME_HOME`, `LOG_NAME`) | fixa em `config.h` | Trocada na Fase 1 para os valores do SD2 |
| Pastas procuradas no SD | `sd2_root.c` | Trocada na Fase 1 |
| Hook de DLC e constantes de IAP | `sd2_il2cpp.c/.h`, `config.h` | Removidos na Fase 1 |
| `lib_burst_generated.so` | carregamento opcional em `main.c` | Removido na Fase 1 (não existe no SD2) |
| RVAs, guards e fingerprints | cerca de 20 arquivos, derivados do CloverPit 1.4.8 | **Refazer para o SD2** (Fases 2 e 3) |
| Unity 6000.2.6f2 x 6000.0.73f1 | a base avisa que até a mesma versão pode gerar código diferente | Verificar na Fase 3 |
| FMOD (`CP_PATCH_FMOD_BUFFER_GEOMETRY` e outros) | vários arquivos | Manter: o FMOD está dentro do `libunity.so` do SD2 |
| Vídeo de abertura (`CP_VIDEO`, ffmpeg) | `sd2_video.c` | Desligado: o SD2 não tem `VideoPlayer` |
| `libmediandk` | funções `AMedia` em `sd2_imports.c` e citação em `imports.c` | Conferir os imports do SD2 (Fase 2) |
| `CP_FORCE_SCREEN_W/H` | ligado à memória de GPU | 1280x720 fixo; troca dock/portátil na Fase 5 |
| Firebase | `firebase_stub.c` | Conferir uso no SD2 (Fase 2) |
| Nomes `cp_*`/`CP_*` e comentários sobre CloverPit | mantidos | Comentários seguem corretos até os valores serem refeitos |

## Fase 3 — Valores do SD2 no código

Conferência: `tools/check_engine.py` (50 verificações, todas passando em 2026-11-02). É análise estática; nada disso foi testado no console.

- **Engine:** `TM_FIELD_*`, `TM_BODY_DELTA` e offsets do FMOD são iguais aos da base. Endereços trocados em `sd2_offsets.h`.
- **Patch de memória:** uma varredura por regra achou um único grupo com os 21 sites da tabela da base, nas mesmas instruções e na mesma ordem. `laytonbmr_nx` (6000.0.58f2) lista 18 sites e não inclui os de `lsr #40`; mantidos os 21 da base.
- **Natives:** 66 achados no `RegisterNatives`. `nativeUnityPlayerSetRunning` existe; `nativeViewPaddingChanged` não, e o boot não o usa. Offsets em `sd2_entrypoints.h` só servem para o log.
- **Hooks removidos, sem método no SD2:** `Input.GetJoystickNames`, `Screen.get_currentResolution_Injected`, `Time.get_fixedDeltaTime`, `Time.get_fixedUnscaledDeltaTime`, `Time.get_renderedFrameCount`.
- **Removidos, só do CloverPit:** `PlatformAPI`, `Master::get__ApiKind`, `RenderingMaster`, `VideoPlayer`.
- **Boot:** `check_data` exigia `sharedassets0.resource`, que o SD2 não tem. Removido.

### Testes no console
- **Teste 1 (2026-11-02):** tela preta, HOME funcionando, sem crash report. `DEBUG_LOG` estava desligado, logo sem log.
- **Teste 2 (2026-11-02):** tela preta. Log mostra 5/5 patches de engine, 21/21 sites de memória, hooks aplicados, Unity iniciado, EGL criado e FMOD produzindo áudio. Ficou no frame 0: `[gc] BRIDGE DISABLED`, com a thread principal na parada do GC (`libil2cpp+0x1537c50`).
- **Correção:** offsets e guards da ponte do GC refeitos para o SD2 em `libc_shim.c`.
- **Teste 3 (2026-11-02):** ponte do GC validada (30/24). Log com 3180 frames, cerca de 59,5 fps, `swap` EGL ok e FMOD com áudio não silencioso. LaLemniscata confirmou no Switch: tela título com imagem e áudio; só o seletor de idioma respondeu ao toque e os Joy-Con não responderam.

### Input do SD2 (leitura do dump, 2026-11-02)
- O jogo usa `CustomInputModule` e `CursorController`, com os eixos `Horizontal`/`Vertical` e os botões `Submit`/`Cancel`. Não usa Rewired.
- O clique do cursor por controle usa `GetKeyDown/GetKeyUp(331)`, `JoystickButton1`, mapeado hoje no A.
- Bug corrigido: o estado anterior dos botões era gravado antes do jogo ler, então `GetKeyDown/Up` nunca disparavam.
- Toque: `android_native_unity.c` escalava para 1920x1080 com o jogo em 1280x720, então os toques caíam fora do alvo. Corrigido; falta testar.
- Teste 4 (2026-11-02, LaLemniscata): com o cursor por giroscópio e A passou da tela título e chegou à batalha de abertura (Level 98), onde o toque não respondeu. Giroscópio desligado depois disso.

### Teste 5 (2026-11-02, LaLemniscata)
Toque correto, com 268 eventos no log e coordenadas plausíveis. Jogo passou pelo tutorial, cidade, taverna, missão e loja. Sessão registrada de cerca de 4,5 minutos, sem stall, com 59,4 fps no final; 9 de 134 amostras abaixo de 55 fps (mínimo 0,7, a confirmar se só em carregamentos). Loja: `Could not contact the purchasing server`, sem travar. Joy-Con ainda sem efeito: o cursor fica oculto em portátil até apertar +, e o analógico só atua com ele visível.

### Save (leitura do dump e log do teste 6, 2026-11-02)
- `SaveManager.SaveGameData` é chamado em `VisualAdventureObserver.TriggerAutosave` e `OnBattleStarted`, `UIMenuAdventureResults.ConfirmExit`, `UIMenuOptions.OnExitToTitleYes` e outros eventos. Não há chamada direta a partir de `SetGlobalVolume`.
- `Main.OnApplicationPause` é vazio, então o jogo não salva ao pausar.
- O log não mostra `[prefs]` além do load, e o `prefs.kv` nunca foi criado: nada indica uso de `PlayerPrefs` pelo jogo nesse fluxo.
- O loader grava o SD a cada 120 frames. Sair pelo HOME não chega a `[boot] shutting down`, porque não há `appletLockExit`.
- Confirmado pelo LaLemniscata no Switch: após Exit to Title e reabrir, ficaram salvos idioma, mudo e progresso. Fechar pelo HOME volta ao último ponto salvo pelo jogo.

### Jogo offline (leitura do dump, 2026-11-03)
- Pontos de anúncio: `RequestAd` tem 6 chamadores, todos ligados a bônus ou atalhos conhecidos: reviver, resultado da aventura, arena, recargas, créditos, recrutador.
- Arena: `SaveFile.ArenaIsAvailable` usa o tempo desde a última entrada; o anúncio chama `UnlockArenaImmediately`, um atalho.
- Recrutador: existe `ClickBtnHirePartyWithAd`, além do caminho normal de contratar. Falta confirmar no jogo.
- Existe `GameContext.PlatformIsSteam`, e o jogo saiu no Steam: indício de fluxo completo sem esses anúncios, não prova.
- Ninguém zerou o jogo neste port. Afirmação ainda não verificada na prática.

## Fase 2 — Reconhecimento

### Imports contra o loader (primeira rodada)
Todos os símbolos indefinidos têm entrada `{ "nome", ... }` nas tabelas do loader: `libmain` 12, `libunity` 341, `libil2cpp` 278. Listas em `dumps/*.und`.
Limite: ter entrada não prova que funciona. Várias são stubs de recusa, como `z_stub0` e a parte de mídia em `sd2_imports.c`.

### Fingerprints da base no `libunity.so` do SD2 (análise estática, sem teste no console)
| Alvo | Resultado | Endereço no SD2 |
|---|---|---|
| `TimeManager::Update` | 16/16, uma ocorrência | `0x57d68c` |
| `AndroidAudio::GetAndroidAudioOutputType` | 16/16, uma ocorrência | `0x72f284` |
| FMOD OpenSL buffer geometry | 8/8, uma ocorrência | `0xeb9064` |
| `ChoreographerBase::Get` | 15/16; a palavra diferente é um `ldr` de global relativo ao `adrp` (`f9446268` x `f9470268`). Mesma distância até `WaitVSync` da base (`0x1d0`) | `0x71f7ac` |
| `WaitVSync` | Laço lido por disassembly: `adrp x20` / `mutex_lock` / `ldr x21,[x22,#0x898]` / `cmp` / `b.ge` / `add x0,x20,#0x28` / `cond_wait` | `0x71f5dc` |
| Contador de vsync | No segmento RW, além do `filesz` (`.bss`) | `0x12c7898` |

O FMOD está dentro do `libunity.so`, então o código de FMOD da base continua relevante mesmo sem `.so` própria.
`libunity.so` cita `libOpenSLES.so`, `libaaudio.so`, `libvulkan.so` e `libEGL.so`; o loader recusa aaudio e vulkan de propósito.

### Outras bases e ferramentas
- `NaGaa95/laytonbmr_nx`: Unity 6000.0.58f2, MIT, a mais próxima do SD2 (6000.0.73f1). A base já usa um port dele como validação do contador de vsync. Referência a consultar para `WaitVSync`.
- Il2CppDumper v6.7.46 (Perfare, MIT; `Il2CppDumper-win-v6.7.46.zip`, SHA-256 `f5fc60df...fa2dea2`): rodou no SD2. Metadata 31, 72.438 métodos com RVA, todos dentro do `.text`. O aviso `This file may be protected` veio junto com `find JNI_OnLoad`; nada no dump indica proteção. Saída em `dumps/il2cpp/`.

### Hooks do `libil2cpp`
22 de 29 métodos Input/Screen/Time da base têm RVA direto no SD2 (`dumps/rva_map.txt`). Sem RVA direto, a resolver na Fase 3: `Input.GetJoystickNames`, `Screen.get_currentResolution_Injected`, `Display.get_systemWidth/Height`, `Time.get_fixedUnscaledDeltaTime`, `Time.get_fixedDeltaTime`, `Time.get_renderedFrameCount`.
Hooks exclusivos do CloverPit (`PlatformAPI`, `RenderingMaster`, `VideoPlayer`) não se aplicam. O SD2 não tem o tipo `VideoPlayer`, então `CP_VIDEO` deve poder ser desligado.

### Comportamento do jogo (leitura do dump, sem teste)
- `Backend` concentra anúncios, IAP, login e serviços online: `InitAds`, `InitIAP`, `InitGameServices`, `LoginFile`, `OnLoginFailure`, `DownloadTitleData`, `DownloadTitleNews`, `EnactAdRequest`, `FailRewardedAdAttempt`, `AttemptIAP`.
- Save local: `SaveManager`, `SaveFile`, arquivo `soda_dungeon_2_save_data.dat`. O código usa `persistentDataPath`.
- Hosts no código do jogo: `api.armorgamesstudios.com`, `sandbox.api.armorgamesstudios.com`.
- `AdPlacement`: `INTERSTITIAL_AFTER_ADVENTURE`, `REWARDED_SODA_PACK`, `REWARDED_TEAM_REVIVE`, `REWARDED_BATTLE_CREDITS`, `REWARDED_ARENA_UNLOCK`, `REWARDED_RECRUITER_HIRE`.
- A questão em aberto: se arena e recrutador têm outro caminho além do anúncio (decisão do LaLemniscata, `PLAN.md` seção 6).


### SDKs encontrados
| SDK | Tipo (ads/IAP/analytics/login/rede) | Onde aparece | Tratamento |
|---|---|---|---|
| Unity LevelPlay | Anúncios | `Unity.LevelPlay`, `Backend` | Nunca inicializar; caminho de falha do jogo |
| Voxel Busters Essential Kit | IAP, cloud, game services, outros | `VoxelBusters.*`, `IAPManager`, `Backend.EK*` | Loja indisponível; no-op |
| PlayFab | Login, dados, notícias | namespace `PlayFab`, `Backend` | Falhar rápido |
| Unity Services Analytics | Analytics | `Unity.Services.*`, `UnityEngine.Unity*Analytics*` | No-op |
| Servidor Armor Games | Backup e importação de save, logs | `Backend`, `api.armorgamesstudios.com` | Falhar rápido |

### Imports por `.so`
| Biblioteca | Dependências (`readelf -d`) | Observações |
|---|---|---|
