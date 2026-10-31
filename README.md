# Arena Survivor

A small, complete top-down arena survival game for **Unity 6.3 LTS** (Universal Render Pipeline, WebGL-first) with low-poly art generated procedurally in **Blender**.

Move, aim and shoot. Waves of enemies pour in from the edge of the arena. Clear a wave or fill the XP bar and choose one of three upgrades. Survive as long as you can and beat your high score.

Everything in this repository was authored as plain files: C# scripts, Blender/Python scripts, generated FBX models, a palette texture and generated sound effects. The Unity scene, prefabs, materials and render-pipeline settings are **generated inside the editor by a bootstrap script** the first time you open the project, so nothing fragile is hand-written.

## Controls

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move | WASD or arrow keys | Left stick / d-pad |
| Aim | Mouse | Right stick |
| Fire | Left mouse button or Space | Right trigger or A |
| Pause | Esc or P | Start |
| Pick upgrade | Click, or keys 1 / 2 / 3 | A on a card |

## Requirements

- **Unity Hub** with **Unity 6000.3.x LTS** installed, including the **WebGL Build Support** module (and whatever desktop module you want for local testing).
- Nothing else is needed to open, play or build the game.
- Optional, only for regenerating the art or sounds: Python 3.13 and the `bpy` package (Blender 5.2 as a Python module).

## Quick start

1. Clone this repository.
2. In Unity Hub choose **Add → Add project from disk** and pick the cloned folder. Open it with Unity 6000.3.x. The first open resolves packages and imports assets; expect a few minutes.
3. If the editor asks to enable the new Input System backends and restart, answer **Yes**.
4. A dialog titled *Arena Survivor* offers to build the generated assets. Click **Build now**. (You can run it any time from the menu **Arena → Build Project**; it is safe to re-run.)
5. Watch the Console for `[Arena] Bootstrap complete`. The generated scene `Assets/Generated/Scenes/Arena.unity` opens automatically.
6. Press **Play**. Click **PLAY** in the menu and survive.
7. Commit the files Unity generated: `Assets/Generated/`, every `.meta` file, `Packages/packages-lock.json` and `ProjectSettings/`. After that, clones open without the bootstrap step.

## Building for the web

1. **Arena → Build WebGL** (or File → Build Profiles → Web → Build). Output goes to `Build/WebGL`.
2. WebGL builds do not run from `file://`. Serve the folder, for example:
   ```
   cd Build/WebGL
   python -m http.server 8000
   ```
   then open <http://localhost:8000>.
3. The build uses gzip compression with the decompression fallback enabled, so it works on static hosts such as itch.io or GitHub Pages without special server headers.

Browser notes: audio starts after the first click (the PLAY button). A gamepad appears only after you press a button on it.

## Project layout

```
Assets/
  Art/Models/        FBX models exported from Blender + models_manifest.json
  Art/Textures/      palette.png (4x4 flat colours, one material for every model)
  Audio/             generated .wav sound effects
  Scripts/Runtime/   gameplay code (Core, Input, Player, Combat, Enemies, Waves, Pickups, Upgrades, UI)
  Scripts/Editor/    bootstrap that generates scene, prefabs, materials, URP settings; WebGL build menu
  Generated/         created by the bootstrap on your machine (scene, prefabs, materials, data, settings)
Packages/manifest.json
ProjectSettings/
unreal/ArenaSurvivor/  Unreal Engine 5.6 C++ port (see its README)
tools/
  blender/           build_models.py (procedural models), build_models_unreal.py (UE export profile), palette.py (colours + PNG writer)
  audio/             gen_sfx.py (procedural sound effects)
  check/             check_project.py, check_unreal.py (sanity checks that run without the engines)
```

## How it works

**State machine.** `GameManager` owns the state (`Menu`, `Playing`, `Upgrading`, `Paused`, `GameOver`), score, wave and XP. UI panels subscribe to its events and show themselves only in their states. `Time.timeScale` is zero outside `Playing`.

**Waves.** `WaveSpawner` builds each wave from a formula: more blobs each wave, dashers from wave 3, tanks from wave 5, and a boss-sized tank every fifth wave. Enemy HP scales up 12 % per wave. Enemies are spawned on the arena edge from an object pool.

**Enemies.** One `Enemy` class with three behaviours defined by `EnemyDefinition` assets: chasers run at you, dashers orbit and lunge, tanks are slow and shrug off knockback. Enemies are dynamic rigidbodies so they jostle each other and collide with pillars and crates.

**Upgrades.** Clearing a wave or filling the XP bar offers three `UpgradeDefinition` cards: fire rate, damage, move speed, max HP, multishot, projectile speed. Each can stack a limited number of times.

**Input.** All actions are built in code in `GameInput` (no `.inputactions` asset). Keyboard + mouse and gamepad work side by side.

**Rendering.** URP with one directional light, no post-processing, 2x MSAA, one `URP/Lit` material whose texture is the 16-colour palette. Each model face's UVs are collapsed onto one palette cell.

**Bootstrap.** `Assets/Scripts/Editor/ProjectBootstrap.cs` runs the steps in order: folders → player settings → layers and collision matrix → URP asset → importer settings → model scale check → material → definitions and prefabs → scene → build list → save. Each step is logged as `[Arena] step N/11`. Everything it creates lives under `Assets/Generated`; **Arena → Delete Generated Assets** wipes it so you can rebuild from scratch.

## Unreal Engine 5.6 version

The same game is also available as an Unreal Engine 5.6 C++ project in [`unreal/ArenaSurvivor/`](unreal/ArenaSurvivor/README.md). Unity and Unreal cannot be connected to each other; what the two versions share is the source of the art and sound: the Blender scripts in `tools/blender/`, the palette and the generated `.wav` files. The Unreal project keeps its own copies under `RawAssets/` (exported with `tools/blender/build_models_unreal.py`) and imports them with an editor Python script on first launch. See its README for setup.

## Regenerating the art and sounds

The models are built with Blender's `bmesh` API from primitives (no manual modelling) and exported as FBX with the axis conversion baked in, so they import into Unity at the right scale and orientation.

```
python3 -m venv .venv-bpy
.venv-bpy/bin/pip install bpy==5.2.2          # Blender 5.2 as a Python module (~400 MB)
.venv-bpy/bin/python -I tools/blender/build_models.py --out Assets/Art/Models
python3 -I tools/blender/palette.py Assets/Art/Textures/palette.png
python3 -I tools/audio/gen_sfx.py Assets/Audio
python3 -I tools/check/check_project.py .
```

Edit `tools/blender/palette.py` to change colours (the PNG and the model UVs stay in sync because both read the same table). Edit the `build_*` functions in `build_models.py` to change shapes. Triangle budgets are enforced per model.

## Troubleshooting

- **Everything is pink.** URP is not active or the material lost its shader. Run **Arena → Build Project** again.
- **`InvalidOperationException: You are trying to read Input using the UnityEngine.Input class...`** Active Input Handling is set to the old Input Manager. Edit → Project Settings → Player → Other Settings → Active Input Handling → *Input System Package (New)*, then restart the editor.
- **Models are tiny, huge or lying on their side.** Check the Console for a `[Arena] <model>: imported height ...` warning. Select the FBX, and in the Model tab set *Scale Factor* 1, *Convert Units* on, and try toggling *Bake Axis Conversion*.
- **No sound in the browser.** Click PLAY first; browsers block audio until a user gesture.
- **Gamepad not detected in the browser.** Press any button on the pad once.
- **Unity 6.0 instead of 6.3.** Change `com.unity.render-pipelines.universal` in `Packages/manifest.json` to `17.0.4` and `ProjectVersion.txt` to your installed version.
- **The project opened but nothing was generated.** The prompt only shows once per editor session. Use the menu **Arena → Build Project**.

## Licence

MIT. Do whatever you like with it.
