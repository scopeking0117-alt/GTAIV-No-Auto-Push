# GTA IV — No Auto Push Peds

A lightweight gameplay mod for **Grand Theft Auto IV** that prevents Niko Bellic from automatically shoving pedestrians during incidental contact.

## Why I made this

In the unmodified game, Niko can automatically push pedestrians when moving into them. I found this behavior distracting and immersion-breaking, especially when simply navigating crowded sidewalks. This mod was created to remove that unwanted automatic reaction and make pedestrian encounters feel more natural.

## Features

- Disables Niko's automatic pedestrian-shoving behavior.
- Focuses on a small, targeted gameplay change.
- Intended for single-player GTA IV on PC.

## Requirements

- **Grand Theft Auto IV** (PC)
- [Priler's ScriptHook](https://www.nexusmods.com/gta4/mods/1217)
- An ASI loader, such as the one included with [FusionFix](https://github.com/ThirteenAG/GTAIV.EFLC.FusionFix), or a compatible standalone ASI loader

## Installation

1. Install [Priler's ScriptHook](https://www.nexusmods.com/gta4/mods/1217).
2. Install an ASI loader. You can use [FusionFix](https://github.com/ThirteenAG/GTAIV.EFLC.FusionFix), which includes an ASI loader (`dinput8.dll`), **or** install a compatible standalone Ultimate ASI Loader. You do not need to install a second ASI loader if FusionFix already provides one.
3. Download the ZIP from [Releases](../../releases) and extract it.
4. Open the directory containing `GTAIV.exe`. The default Steam location is:

   ```text
   C:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto IV\GTAIV
   ```

5. Copy **`NoAutoPushHelper.asi`** into the main `GTAIV` directory, alongside `GTAIV.exe`.
6. Copy **`NikoNoAutoPush.net.dll`** into `GTAIV\scripts`. If the archive contains a `scripts` folder, merge it with your existing `scripts` folder; do not create `scripts\scripts`.
7. Launch GTA IV.

Your installed files should look like this:

```text
GTAIV/
├── GTAIV.exe
├── NoAutoPushHelper.asi
└── scripts/
    └── NikoNoAutoPush.net.dll
```

**Important:** The `.asi` and `.net.dll` files go in **different locations**. Do not put both files in `scripts` or both beside `GTAIV.exe`.

## Download

Use the [GitHub Releases](../../releases) page for precompiled downloads. The repository itself is intended primarily for source code and development files.

## Source code

The project includes C# source code and its project configuration. Build output such as `bin/` and `obj/` is excluded from version control. Some parts of the project may also include native code; consult the repository files for details.

## Credits

Created by **Saul** (`scopeking0117-alt`).

Grand Theft Auto IV is developed by Rockstar North and published by Rockstar Games. This project is an independent, unofficial fan modification and is not affiliated with or endorsed by Rockstar Games or Take-Two Interactive.

## Additional Notes

- **Pedestrians Only:** This mod specifically removes Niko Bellic's automatic pedestrian-shoving animation. It does not affect doors or Niko's interactions with them.
- **Episodes from Liberty City:** The mod is intended to work with Grand Theft Auto IV, The Lost and Damned (TLAD), and The Ballad of Gay Tony (TBoGT).
- **Compatibility:** The mod should work with TLAD and TBoGT but has not yet been independently verified.

## Gameplay Demonstration

[![GTA IV — No Auto Push Peds Gameplay Demonstration](https://img.youtube.com/vi/HekvSrZJoPw/hqdefault.jpg)](https://youtu.be/HekvSrZJoPw)
