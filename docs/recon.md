# Recon (Fase 0 e Fase 2)

Preencher com resultados verificados. Nada aqui é suposição.

## Fase 0 — Viabilidade

| Item | Resultado | Como foi verificado |
|---|---|---|
| Pacote | | `adb shell pm list packages` |
| versionName / versionCode | | `dumpsys package` |
| Split APKs | | `pm path` |
| ABIs presentes (`lib/*/`) | | `unzip -l` |
| Backend (IL2CPP / Mono) | | `libil2cpp.so` + `global-metadata.dat` |
| Versão do Unity | | `strings libunity.so` |
| Outras `.so` | | `unzip -l` |
| Metadata ofuscada? | | Il2CppDumper |
| SHA-256 de cada `.so` | | `sha256sum` |
| Firmware do Switch / Atmosphère | | LaLemniscata |
| Port do SD2 já em `planned`/`completed`? | | LaLemniscata, no Switch Port Zone |

**Decisão GO/NO-GO:** _(pendente)_

## Fase 2 — Reconhecimento

### SDKs encontrados
| SDK | Tipo (ads/IAP/analytics/login/rede) | Onde aparece | Tratamento |
|---|---|---|---|

### Imports por `.so`
| Biblioteca | Dependências (`readelf -d`) | Observações |
|---|---|---|
