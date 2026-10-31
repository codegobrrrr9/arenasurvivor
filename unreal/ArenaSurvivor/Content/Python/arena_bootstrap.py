"""Editor bootstrap for Arena Survivor (Unreal Engine 5.6).

Imports the FBX models, palette texture and sounds from RawAssets/, builds the
palette material and the Arena map. Idempotent: existing assets are kept unless
``run(force=True)``.

Run manually from the Output Log (switch the prompt from Cmd to Python):
    import arena_bootstrap; arena_bootstrap.run(force=True)
or Tools > Execute Python Script > this file.
"""
import math
import os
import random

import unreal

MODELS = ["player", "enemy_blob", "enemy_spiky", "enemy_tank", "projectile",
          "pickup_xp", "pickup_health", "arena_floor", "pillar", "crate"]
SOUNDS = ["shoot", "hit", "enemy_die", "pickup", "upgrade", "player_hurt",
          "game_over", "ui_click", "wave_start"]
MODELS_DIR = "/Game/Models"
TEXTURES_DIR = "/Game/Textures"
AUDIO_DIR = "/Game/Audio"
MATERIALS_DIR = "/Game/Materials"
MATERIAL = MATERIALS_DIR + "/M_Palette"
MAP = "/Game/Maps/Arena"
ARENA_RADIUS = 2000.0

TOTAL_STEPS = 5


def log(msg):
    unreal.log(f"[Arena] {msg}")


def warn(msg):
    unreal.log_warning(f"[Arena] {msg}")


def raw_dir(*parts):
    return os.path.join(unreal.Paths.project_dir(), "RawAssets", *parts)


def set_prop(obj, name, value):
    """set_editor_property that logs instead of raising, so one bad name cannot abort the import."""
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        warn(f"could not set {type(obj).__name__}.{name}: {exc}")
        return False


def editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def force_legacy_fbx():
    # From 5.5 FBX goes through Interchange, which ignores FbxImportUI options.
    try:
        unreal.SystemLibrary.execute_console_command(editor_world(), "Interchange.FeatureFlags.Import.FBX 0")
    except Exception as exc:
        warn(f"could not disable Interchange FBX import: {exc}")


def asset_exists(path):
    return unreal.EditorAssetLibrary.does_asset_exist(path)


# ------------------------------------------------------------------ import

def fbx_options():
    ui = unreal.FbxImportUI()
    set_prop(ui, "automated_import_should_detect_type", False)
    set_prop(ui, "import_mesh", True)
    set_prop(ui, "import_as_skeletal", False)
    set_prop(ui, "import_animations", False)
    set_prop(ui, "import_materials", False)
    set_prop(ui, "import_textures", False)
    set_prop(ui, "create_physics_asset", False)
    try:
        set_prop(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    except Exception as exc:
        warn(f"mesh_type_to_import: {exc}")
    sm = ui.static_mesh_import_data
    set_prop(sm, "combine_meshes", True)
    set_prop(sm, "auto_generate_collision", True)
    set_prop(sm, "generate_lightmap_u_vs", False)
    set_prop(sm, "build_nanite", False)
    set_prop(sm, "remove_degenerates", True)
    try:
        set_prop(sm, "normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    except Exception as exc:
        warn(f"normal_import_method: {exc}")
    # Models are centimetres, Y-up, front +Z (Maya convention). Convert Scene + Force Front X
    # lands them Z-up, 1:1 scale, facing +X. The C++ side also auto-detects the facing.
    set_prop(sm, "convert_scene", True)
    set_prop(sm, "convert_scene_unit", True)
    set_prop(sm, "force_front_x_axis", True)
    set_prop(sm, "import_uniform_scale", 1.0)
    return ui


def import_files(paths, destination, options=None):
    tasks = []
    for path in paths:
        if not os.path.exists(path):
            warn(f"missing source file {path}")
            continue
        task = unreal.AssetImportTask()
        set_prop(task, "filename", path)
        set_prop(task, "destination_path", destination)
        set_prop(task, "automated", True)
        set_prop(task, "save", True)
        set_prop(task, "replace_existing", True)
        if options is not None:
            set_prop(task, "options", options)
        tasks.append(task)
    if not tasks:
        return []
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        try:
            imported.extend(list(task.get_editor_property("imported_object_paths")))
        except Exception:
            pass
    return imported


def import_models(force):
    todo = [m for m in MODELS if force or not asset_exists(f"{MODELS_DIR}/{m}")]
    if not todo:
        log("models already imported")
        return
    force_legacy_fbx()
    imported = import_files([raw_dir("Models", f"{m}.fbx") for m in todo], MODELS_DIR, fbx_options())
    log(f"imported {len(imported)} model asset(s)")
    missing = [m for m in MODELS if not asset_exists(f"{MODELS_DIR}/{m}")]
    if missing:
        warn(f"models not imported: {missing}. See README 'Manual fallback'.")


def import_texture(force):
    path = f"{TEXTURES_DIR}/palette"
    if force or not asset_exists(path):
        import_files([raw_dir("Textures", "palette.png")], TEXTURES_DIR)
    tex = unreal.load_asset(path)
    if tex is None:
        warn("palette texture missing")
        return None
    try:
        set_prop(tex, "filter", unreal.TextureFilter.TF_NEAREST)
        set_prop(tex, "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        set_prop(tex, "srgb", True)
        set_prop(tex, "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
    except Exception as exc:
        warn(f"texture settings: {exc}")
    return tex


def import_sounds(force):
    todo = [s for s in SOUNDS if force or not asset_exists(f"{AUDIO_DIR}/{s}")]
    if todo:
        import_files([raw_dir("Audio", f"{s}.wav") for s in todo], AUDIO_DIR)
    missing = [s for s in SOUNDS if not asset_exists(f"{AUDIO_DIR}/{s}")]
    if missing:
        warn(f"sounds not imported: {missing}")
    else:
        log("sounds ready")


# ------------------------------------------------------------------ material

def make_material(tex, force):
    if asset_exists(MATERIAL) and not force:
        return unreal.load_asset(MATERIAL)
    if asset_exists(MATERIAL):
        unreal.EditorAssetLibrary.delete_asset(MATERIAL)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset("M_Palette", MATERIALS_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        warn("could not create M_Palette")
        return None
    mel = unreal.MaterialEditingLibrary
    sample = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, 0)
    if tex is not None:
        set_prop(sample, "texture", tex)
    try:
        set_prop(sample, "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    except Exception as exc:
        warn(f"sampler_type: {exc}")
    mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 250)
    set_prop(rough, "r", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    log("material M_Palette created")
    return mat


def assign_material(mat):
    if mat is None:
        return
    for name in MODELS:
        mesh = unreal.load_asset(f"{MODELS_DIR}/{name}")
        if mesh is None:
            continue
        try:
            mesh.set_material(0, mat)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        except Exception as exc:
            warn(f"could not assign material to {name}: {exc}")


# ------------------------------------------------------------------ level

def build_level(force):
    if asset_exists(MAP) and not force:
        log("map already exists")
        return
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not les.new_level(MAP):
        warn(f"could not create level {MAP}")
        return
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    def mesh(name):
        return unreal.load_asset(f"{MODELS_DIR}/{name}")

    def place(mesh_name, location, yaw=0.0, label="", profile="BlockAll"):
        actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, location, unreal.Rotator(0.0, 0.0, yaw))
        if actor is None:
            return None
        m = mesh(mesh_name)
        if m is not None:
            actor.static_mesh_component.set_static_mesh(m)
        actor.static_mesh_component.set_collision_profile_name(profile)
        if label:
            actor.set_actor_label(label)
        set_prop(actor, "tags", ["ArenaWorld"])
        return actor

    place("arena_floor", unreal.Vector(0, 0, 0), 0.0, "Floor", "NoCollision")
    pillars = 16
    for i in range(pillars):
        a = i * 2 * math.pi / pillars
        r = ARENA_RADIUS - 250.0
        place("pillar", unreal.Vector(math.cos(a) * r, math.sin(a) * r, 0), -math.degrees(a), f"Pillar{i}")
    rng = random.Random(42)
    for i in range(14):
        a = rng.random() * 2 * math.pi
        d = 500.0 + rng.random() * 900.0
        place("crate", unreal.Vector(math.cos(a) * d, math.sin(a) * d, 0), rng.random() * 90.0, f"Crate{i}")

    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(0.0, -55.0, -35.0))
    if sun is not None:
        sun.set_actor_label("Sun")
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if comp is not None:
            comp.set_light_color(unreal.LinearColor(1.0, 0.96, 0.9, 1.0))
            comp.set_intensity(4.0)
        set_prop(sun, "tags", ["ArenaWorld"])
    atmosphere = eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    if atmosphere is not None:
        atmosphere.set_actor_label("SkyAtmosphere")
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    if sky is not None:
        sky.set_actor_label("SkyLight")
        comp = sky.get_component_by_class(unreal.SkyLightComponent)
        if comp is not None:
            set_prop(comp, "real_time_capture", True)
    eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 92))

    les.save_current_level()
    log(f"level {MAP} created")


# ------------------------------------------------------------------ entry

def run(force=False):
    log(f"bootstrap starting (force={force})")
    steps = [
        ("import models", lambda: import_models(force)),
        ("import texture + sounds", lambda: (import_texture(force), import_sounds(force))),
        ("material", lambda: assign_material(make_material(unreal.load_asset(f"{TEXTURES_DIR}/palette"), force))),
        ("level", lambda: build_level(force)),
        ("load level", lambda: unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)),
    ]
    ok = True
    for i, (name, fn) in enumerate(steps, 1):
        try:
            fn()
            log(f"step {i}/{TOTAL_STEPS}: {name} - ok")
        except Exception as exc:
            ok = False
            unreal.log_error(f"[Arena] step {i}/{TOTAL_STEPS}: {name} - FAILED: {exc}")
    if ok:
        log("Bootstrap complete. Press Play.")
    else:
        unreal.log_error("[Arena] Bootstrap finished with errors; see README 'Manual fallback'. Re-run with arena_bootstrap.run(force=True).")


if __name__ == "__main__":
    run(force=True)
