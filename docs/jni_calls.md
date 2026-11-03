# Inventário JNI (Fase 2)

Fonte: leitura dos DEX do APK, `stringliteral.json` do Il2CppDumper e `strings libunity.so`.
Listas brutas (fora do git): `dumps/dex_native.txt`, `dumps/jni_candidates.txt`.

**Limite:** isto mostra o que existe, não as chamadas feitas em execução. Parâmetros e valores exatos só aparecem no log da Fase 3.

## Como o loader responde
- Não há Java no port. `jni_fake.c` devolve uma classe interna para qualquer `FindClass`.
- O comportamento é decidido pelo nome do método, em `jni_fake.c`, `unity_jni.c` e `sd2_jni.c`.
- Slots JNI sem implementação são registrados no log (`jni_unimpl.h`).

## Métodos `native` no DEX (80 no total)
| Classe | Qtd. | Comportamento no port |
|---|---|---|
| `UnityPlayer` e `UnityPlayerForActivityOrService` | 29 | Recuperados por nome no `RegisterNatives`; os 29 nomes aparecem no loader. Confirmar na Fase 3 |
| Outras classes `com/unity3d/player` (acessibilidade, ARCore, câmera, `ReflectionHelper` etc.) | 37 | Sem Java para chamá-los. `ReflectionHelper` é citado no loader; demais a observar no log |
| `org/fmod/FMODAudioDevice` | 3 | Áudio pelo FMOD interno e OpenSL (`opensles.c`) |
| `bitter/jnibridge/JNIBridge` | 1 | A observar no log |
| `com/android/billingclient` | 9 | Sem Google Play Billing. IAP sempre indisponível |
| `com/google/android` (`NativeOnCompleteListener`) | 1 | Sem Play Services. A observar no log |

## Classes pedidas pelo código nativo
| Origem | Classes | Comportamento no port |
|---|---|---|
| `libunity.so` | 159 nomes (`android/*`, `java/*`, `com/unity3d/*`) | Muitos não têm nome no loader, o que é esperado com o `FindClass` genérico. Tratar só o que aparecer como problema no log |
| Código C# | `com.ironsource.unity.androidbridge.*` | Anúncios: não inicializar |
| Código C# | `com.voxelbusters.essentialkit.*` | IAP, cloud e game services: indisponível, no-op |
| Código C# | `com.unity3d.player.UnityPlayer`, `PlayAssetDeliveryUnityWrapper` | Já citados no loader |

## SDKs Java no APK
Google Play Services, Facebook Audience Network, Unity Ads, ironSource/LevelPlay, Voxel Busters, Google Play Billing, AndroidX.
Nenhum é executado no port, porque o DEX não roda.

## Ponto a vigiar na Fase 3
O maior risco é o C# chamar `AndroidJavaObject` em um desses SDKs durante o boot e travar ou falhar com um valor nulo. Cortar em `Backend.InitAds`, `InitIAP` e `InitGameServices`, com patch mínimo e documentado, só se o log mostrar necessidade.
