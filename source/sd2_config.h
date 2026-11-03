/* sd2_config.h -- user settings read from config.txt next to the .nro.
 *
 * MIT.
 */
#ifndef SD2_CONFIG_H
#define SD2_CONFIG_H

typedef struct {
  int   log;            /* 1 writes debug.log and stall.log */
  int   fps;            /* 60 or 30 */
  float cursor_speed;   /* px/frame at full stick deflection, at 720p */
  int   game_cursor;    /* 1 uses the game's cursor, 0 the port's arrow */
} Sd2Config;

extern Sd2Config sd2_config;

/* Reads <root>/config.txt, writing a commented default when it is missing.
 * Call after the game root is resolved and before anything logs. */
void sd2_config_load(const char *root);

#endif /* SD2_CONFIG_H */
