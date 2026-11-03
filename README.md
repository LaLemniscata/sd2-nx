# sd2_nx

Work-in-progress native wrapper for running the original Android ARM64 build of
*Soda Dungeon 2* on Nintendo Switch homebrew (Atmosphère).

**Status: playable on an OLED Switch, handheld and docked. Testing is not finished.**
Offline only: ads, purchases, login and cloud saves are unavailable.

This repository contains no game code or assets. Game files must come from your own
copy of the game, installed from Google Play. Supported version target: 1.2.21 (140).

## Building

Requires devkitPro with devkitA64 and:

```
sudo dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau switch-sdl2 switch-zlib switch-libpng switch-ffmpeg
```

Then run `make`. The output is `sd2_nx.nro`.

## Settings

`config.txt` is created next to the `.nro` on first boot: `log`, `fps` (60 or 30),
`cursor_speed` and `game_cursor`.

## Credits

This project is based on [cloverpit_nx](https://github.com/ChanseyIsTheBest/cloverpit_nx)
by ChanseyIsTheBest (MIT), including its Unity 6 / IL2CPP loader work.

cloverpit_nx credits the so-loader lineage this code inherits: badpiggies_nx,
colorsheep_nx, laytonbmr_nx and [vln_nx](https://github.com/NaGaa95/vln_nx) by naga,
the TheOfficialFloW / Rinnegatamante so-loader tradition, and Andy Nguyen and fgsfds.

## License

MIT. See `LICENSE`. Inherited notices are kept as they were in the base project.

Soda Dungeon 2 belongs to its respective owners and is not covered by this license.
Running homebrew requires custom firmware, which violates Nintendo terms of service.
