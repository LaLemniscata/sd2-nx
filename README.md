# sd2_nx

Work-in-progress native wrapper for running the original Android ARM64 build of
*Soda Dungeon 2* on Nintendo Switch homebrew (Atmosphère).

**Status: playable on an OLED Switch, handheld and docked. Testing is not finished.**
Offline only: ads, purchases, login and cloud saves are unavailable.

This repository contains no game code or assets. Game files must come from your own
copy of the game, installed from Google Play. Supported version target: 1.2.21 (140).

## Installing

1. Install Soda Dungeon 2 version 1.2.21 from Google Play on an ARM64 Android device.
2. Copy its two APKs to your PC. With USB debugging on:

   ```
   adb shell pm path com.armorgames.sodadungeon2
   adb pull <each path shown above>
   ```

3. Put these three files in `sd:/switch/sd2_nx/`:

   ```
   sd2_nx.nro
   base.apk
   split_config.arm64_v8a.apk
   ```

4. Hold R while opening an installed game to reach the Homebrew Menu, then open sd2_nx.
   Do not launch it from the Album: applet mode lacks the memory.

The first start checks the APKs, unpacks about 200 MB and moves the APKs to `apk/`.
After that, `apk/` can be deleted. To update, replace only `sd2_nx.nro`.

## Controls

| Input | Action |
|---|---|
| Touchscreen | Tap |
| Left stick | Move the cursor |
| A or ZR | Click |
| B | Close the top menu or leave the tavern |
| + | Game menu |
| Right stick | Scroll |
| L or R | Center the cursor |
| ZL (hold) | Shift |

## Building

Requires devkitPro with devkitA64 and:

```
sudo dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau switch-sdl2 switch-zlib switch-libpng switch-ffmpeg
```

Then run `make`. The output is `sd2_nx.nro`.

`tools/stage_sd.py` builds the same SD folder on a PC, as an alternative to first-run setup.

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

This is an unofficial fan project, not affiliated with or endorsed by Nintendo,
Armor Games Studios or Afro-Ninja Productions. Soda Dungeon 2 and its assets belong
to their owners and are not covered by this license.
Running homebrew requires custom firmware, which violates Nintendo terms of service.
