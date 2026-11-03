# Controles

Mapa aprovado pelo LaLemniscata em 2026-11-02. O toque funciona em paralelo no modo portátil.

| Botão | Ação | Evidência |
|---|---|---|
| Analógico esquerdo | Move o cursor, que aparece sozinho | `nx_pointer.c` |
| L ou R | Centraliza o cursor | `nx_pointer.c` |
| A ou ZR | Clique; se o cursor estiver oculto, só o mostra | `nx_pointer.c` |
| Toque na tela | Clique direto; esconde o cursor | Testado no Switch (teste 5) |
| + | Tecla `Escape`: abre as opções quando não há outra ação | `Main.Update`; a testar |
| B | Fecha a janela do topo (`UIManager.ForceCloseIfPossible`) | `sd2_game.c`; a testar |
| ZL (segurar) | `LeftShift` | `Utils.PlayerIsHoldingShift`; efeito a confirmar |
| Analógico direito | Rolagem (`mouseScrollDelta`) | `CustomInputModule`; a confirmar |

Livres: X, D-pad e Minus.

## O que o `Escape` faz no jogo
`Main.Update` tenta, nesta ordem de código: dar OK no diálogo ativo, escolher o padrão da caixa ativa, aceitar a missão aberta, tratar o menu do topo e, por fim, `UIHUD.ForceOpenOptionsIfPossible`. Também aparece `UIHUD.ForceToggleAutoIfPossible`; a condição exata não foi lida.

## Decisões de implementação
- `Submit`, `Cancel`, `Horizontal`, `Vertical` e `JoystickButton1` ficam silenciosos, para o A dar um clique só e o analógico não mexer em duas coisas.
- `[input] game read key N down` no log mostra quais teclas o jogo leu em cada teste.
- O giroscópio fica desligado (`NXP_GYRO_ENABLED`).
