"""Export the same procedural models for Unreal Engine.

Identical geometry to build_models.py (1 BU = 1 m, front -Y, origin at feet);
only the export call differs: Blender's default Y-up FBX frame, centimetre
units baked into the geometry, face smoothing groups so UE does not warn.

    /home/user/.venv-bpy/bin/python -I tools/blender/build_models_unreal.py --out unreal/ArenaSurvivor/RawAssets/Models
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bpy  # noqa: E402

from build_models import BUDGETS, MODELS, Builder, ensure_fbx_exporter  # noqa: E402


def export_fbx_unreal(obj, path):
    for o in bpy.context.scene.objects:
        o.select_set(o is obj)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_NONE",
        global_scale=1.0,
        axis_forward="-Z",
        axis_up="Y",
        use_space_transform=True,
        bake_space_transform=True,
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_triangles=True,
        path_mode="COPY",
        embed_textures=False,
        add_leaf_bones=False,
        bake_anim=False,
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    ensure_fbx_exporter()

    manifest = {}
    for name, build in MODELS.items():
        b = Builder()
        origin = build(b)
        mesh, tris, size = b.finish(name)
        obj = bpy.data.objects.new(name, mesh)
        scene.collection.objects.link(obj)
        export_fbx_unreal(obj, os.path.join(args.out, f"{name}.fbx"))
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(mesh)
        manifest[name] = {"tris": tris, "size_cm": [round(s * 100, 2) for s in size], "origin": origin, "budget": BUDGETS[name]}
        print(f"{name:14s} tris={tris:4d} size_cm={manifest[name]['size_cm']}")
    with open(os.path.join(args.out, "models_manifest.json"), "w") as f:
        json.dump({"models": [dict(name=k, **v) for k, v in manifest.items()]}, f, indent=2)


if __name__ == "__main__":
    main()
