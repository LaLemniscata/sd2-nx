/* sd2_game.h -- calls into Soda Dungeon 2's managed code, resolved by name.
 *
 * MIT.
 */
#ifndef SD2_GAME_H
#define SD2_GAME_H

#include "so_util.h"

/* Resolves the il2cpp runtime API. The managed classes are looked up on first use. */
int sd2_game_bind(so_module *il2cpp);

/* All of these run on the engine main thread, between frames. */
int  sd2_game_close_top_menu(void);   /* UIManager.ForceCloseIfPossible; 0 if no menu */
void sd2_game_save(void);             /* SaveManager.SaveGameData       */
void sd2_game_cursor_show(int show);  /* CursorController graphic       */
/* Answers Backend.RequestAd(placement) as if no ad could be shown. */
void sd2_game_ad_unavailable(void *backend, int placement);
/* Delivers a pending answer. Call once per frame. */
void sd2_game_tick(void);

#endif /* SD2_GAME_H */
