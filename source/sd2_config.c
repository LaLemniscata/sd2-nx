/* sd2_config.c -- user settings read from config.txt next to the .nro.
 *
 * MIT.
 */
#include <stdio.h>
#include <string.h>

#include "sd2_config.h"

Sd2Config sd2_config = { 0, 60, 12.0f, 0 };

static const char DEFAULT_TEXT[] =
  "# sd2_nx settings. Lines starting with # are ignored.\n"
  "#\n"
  "# Write debug.log and stall.log. Costs some SD writes; turn on to report a bug.\n"
  "log=0\n"
  "# Frame rate: 60 or 30. 30 saves battery and heat.\n"
  "fps=60\n"
  "# Cursor speed with the left stick, in pixels per frame at 720p.\n"
  "cursor_speed=12\n"
  "# 1 uses the game's own cursor, 0 the port's arrow.\n"
  "game_cursor=0\n";

void sd2_config_load(const char *root) {
  char path[600];
  snprintf(path, sizeof path, "%s/config.txt", root);

  FILE *f = fopen(path, "r");
  if (!f) {
    f = fopen(path, "w");
    if (f) {
      fputs(DEFAULT_TEXT, f);
      fclose(f);
    }
    return;
  }

  char line[128];
  while (fgets(line, sizeof line, f)) {
    int i;
    float v;
    if (line[0] == '#')
      continue;
    if (sscanf(line, "log=%d", &i) == 1)
      sd2_config.log = i != 0;
    else if (sscanf(line, "fps=%d", &i) == 1)
      sd2_config.fps = (i == 30) ? 30 : 60;
    else if (sscanf(line, "cursor_speed=%f", &v) == 1 && v >= 2.0f && v <= 60.0f)
      sd2_config.cursor_speed = v;
    else if (sscanf(line, "game_cursor=%d", &i) == 1)
      sd2_config.game_cursor = i != 0;
  }
  fclose(f);
}
