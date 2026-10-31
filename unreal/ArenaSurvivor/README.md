# Arena Survivor — Unreal Engine 5.6 version

The same top-down arena survivor as the Unity project at the repository root, ported to **Unreal Engine 5.6** as a **C++ project**. It shares the Blender-generated models, the palette texture and the sound effects.

Everything here is plain text: C++ gameplay, `.uproject`, config `.ini` files, build rules, and an **editor Python bootstrap** that imports the raw assets, builds the material and the map the first time you open the project. Binary `.uasset` files are never committed (they are generated on your machine).

## Controls

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move | WASD or arrow keys | Left stick / d-pad |
| Aim | Mouse | Right stick |
| Fire | Left mouse button or Space | Right trigger or A |
| Pause | Esc or P | Start |
| Pick upgrade | Click, or keys 1 / 2 / 3 | D-pad to a card, A |
| Start / retry | Enter or the PLAY button | A on the focused button |

## Requirements

- **Unreal Engine 5.6** from the Epic Games Launcher.
- **Windows:** Visual Studio 2022 (17.8 or newer) with the workloads *Game development with C++* (including the Windows 10/11 SDK) and *.NET desktop development*.
- **Mac:** Xcode 16.
- Nothing else. Python is built into the editor.

## Quick start

1. Clone the repository. The Unreal project is `unreal/ArenaSurvivor/`.
2. **Windows:** right-click `ArenaSurvivor.uproject` → *Generate Visual Studio project files*. Open `ArenaSurvivor.sln`, choose *Development Editor* / *Win64*, and build (Ctrl+Shift+B).
   **Mac:** run `"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="<full path>/ArenaSurvivor.uproject" -game`, open the generated `.xcworkspace`, select the *ArenaSurvivorEditor* scheme and build.
   Alternatively, double-click the `.uproject` and answer **Yes** when asked to rebuild the missing modules.
3. Open `ArenaSurvivor.uproject`. The first launch may warn that the startup map is missing; that is expected. Within a few seconds the Output Log shows `[Arena] step 1/5 …` through `[Arena] Bootstrap complete` and the map `/Game/Maps/Arena` opens.
4. Press **Play** (Alt+P), click **PLAY** in the menu and survive.
5. Commit nothing from `Content/` except the `Python/` folder, unless you want to ship the generated assets.

If the bootstrap did not run (no `[Arena]` lines in the Output Log): open the Output Log, switch the command prompt from *Cmd* to *Python*, and run

```
import arena_bootstrap; arena_bootstrap.run(force=True)
```

## What the bootstrap does

`Content/Python/init_unreal.py` runs on editor start and defers to `arena_bootstrap.run()` on the first tick. The steps, each logged as `[Arena] step N/5`:

1. Imports the 10 FBX models from `RawAssets/Models` into `/Game/Models` with the legacy FBX importer (Interchange is disabled for FBX because it ignores the import options): static mesh, combine meshes, no materials, import normals, *Convert Scene*, *Force Front XAxis*, scale 1.
2. Imports `palette.png` (nearest filtering, no mipmaps) and the nine `.wav` files.
3. Creates `/Game/Materials/M_Palette` (texture sample → Base Color, roughness 0.85) and assigns it to every mesh.
4. Creates `/Game/Maps/Arena`: floor, 16 pillars, 14 crates (seed 42), directional light, sky atmosphere, sky light, player start. World actors are tagged `ArenaWorld`.
5. Loads the map.

Re-running is safe; existing assets are kept unless `force=True`.

## Manual fallback

If Python is unavailable or the import failed:

1. In the Output Log console run `Interchange.FeatureFlags.Import.FBX 0`.
2. Drag the files from `RawAssets/Models` into the Content Browser folder `Content/Models`. In the FBX Import Options: Import as Static Mesh, *Combine Meshes* on, *Generate Lightmap UVs* off, *Import Uniform Scale* 1.0, Material Import Method *Do Not Create Material*, *Import Textures* off, Advanced: *Convert Scene* on, *Force Front XAxis* on, *Convert Scene Unit* on, Normal Import Method *Import Normals*.
3. Drag `palette.png` into `Content/Textures` and the `.wav` files into `Content/Audio`.
4. Create a material `M_Palette` in `Content/Materials` with a Texture Sample of `palette` connected to Base Color.
5. File → New Level → Basic, save as `Content/Maps/Arena`. Press Play: the game builds the arena props itself when the level has no `ArenaWorld` actors and applies `M_Palette` to every mesh at runtime.

## Project layout

```
ArenaSurvivor.uproject
Config/                 DefaultEngine.ini (maps, game mode, collision channels/profiles, renderer),
                        DefaultGame.ini, DefaultInput.ini (Enhanced Input)
Source/ArenaSurvivor/   C++ module
  ArenaTypes.h          enums, FEnemyDef, FUpgradeDef, FArenaPlayerStats
  ArenaAssets           loads meshes/material/sounds by path; enemy + upgrade tables; mesh orientation fix
  ArenaGameMode         state machine (Menu/Playing/Upgrading/Paused/GameOver), score/wave/xp, fallback arena builder
  ArenaPlayerController runtime Enhanced Input, mouse aim on the ground plane, camera, pause
  ArenaPlayerCharacter  kinematic pawn: capsule + mesh + health, movement, firing
  ArenaEnemy            chaser / dasher / tank behaviours, contact damage, knockback
  ArenaWaveSpawner      formula waves, enemy separation
  ArenaProjectile       line-trace projectile
  ArenaPickup           XP / health drops with magnet
  ArenaUpgradeSystem    three-card offers with stack limits
  ArenaCameraActor      top-down chase camera with aim lead
  ArenaHealthComponent, ArenaMovement, ArenaSaveGame
  UI/                   UMG widgets built in C++: HUD, main menu, pause, upgrade, game over
Content/Python/         init_unreal.py, arena_bootstrap.py
RawAssets/              FBX models, palette.png, .wav files (sources for the import)
```

## How it maps to the Unity version

| Unity | Unreal |
|---|---|
| 1 unit = 1 m, Y up, Z forward | 1 unit = 1 cm, Z up, X forward |
| `GameManager` + events | `AArenaGameMode` + multicast delegates |
| Dynamic rigidbodies with frozen Y | Capsules moved with swept offsets; enemies separated in code |
| Prefabs built by an editor bootstrap | Actors built from C++ at runtime; meshes loaded by path |
| uGUI built by `UIFactory` | UMG widget trees built in `RebuildWidget` |
| Input System actions built in code | Enhanced Input actions built in code |

## Regenerating the raw models

```
python3 -m venv .venv-bpy && .venv-bpy/bin/pip install bpy==5.2.2
.venv-bpy/bin/python -I tools/blender/build_models_unreal.py --out unreal/ArenaSurvivor/RawAssets/Models
python3 -I tools/check/check_unreal.py unreal/ArenaSurvivor
```
Then re-run the bootstrap with `force=True`.

## Troubleshooting

- **Compile error mentioning `EngineIncludeOrderVersion.Unreal5_6`.** Replace it with `EngineIncludeOrderVersion.Latest` in both `Source/*.Target.cs`.
- **Models face sideways.** The log line `[Arena] Player mesh front is not +X … applying yaw` shows the automatic correction. If it is wrong, re-import with *Force Front XAxis* toggled, or set `MeshYaw` in `ArenaAssets.cpp`.
- **Models are 100x too small or large.** Re-import with *Convert Scene Unit* on and *Import Uniform Scale* 1.0.
- **Everything is grey.** `M_Palette` is missing or not assigned; re-run the bootstrap with `force=True`.
- **Nothing happens on key presses.** Project Settings → Input → Default Classes must be `EnhancedPlayerInput` / `EnhancedInputComponent` (they are set in `Config/DefaultInput.ini`).
- **No sound while paused** is possible on some audio setups; gameplay sounds are unaffected.
- **Dark arena when using the manual fallback level.** The runtime-built arena uses a plain directional light and sky light; the bootstrap's map also adds a sky atmosphere.
