"""Build all low-poly game models procedurally with Blender's bmesh API and
export each one as FBX for Unity.

Run headless with the bpy module (Blender 5.x):
    /home/user/.venv-bpy/bin/python -I tools/blender/build_models.py --out Assets/Art/Models

Conventions
- Metric scene, 1 Blender unit = 1 metre.
- Models face Blender -Y, which the exporter maps to Unity +Z.
- Characters/props have their origin at the feet (z = 0); projectile and
  pickups are centred on the origin.
- Every face is flat shaded and its UVs are collapsed to one palette cell so a
  single URP/Lit material with palette.png colours everything.
"""
import argparse
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))  # -I drops it

import bpy  # noqa: E402
import bmesh  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

from palette import uv_for  # noqa: E402

BUDGETS = {
    "player": 320, "enemy_blob": 200, "enemy_spiky": 400, "enemy_tank": 300,
    "projectile": 20, "pickup_xp": 24, "pickup_health": 48,
    "arena_floor": 320, "pillar": 60, "crate": 60,
}


class Builder:
    """Accumulates primitives into one BMesh, colouring each by palette cell."""

    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new("UVMap")

    def _paint(self, verts, color):
        faces = {f for v in verts for f in v.link_faces}
        u, v = uv_for(color)
        for f in faces:
            f.smooth = False
            for loop in f.loops:
                loop[self.uv].uv = (u, v)
        return list(faces)

    def _place(self, verts, center, scale=(1, 1, 1), rot=None):
        bmesh.ops.scale(self.bm, vec=Vector(scale), verts=verts)
        if rot is not None:
            bmesh.ops.rotate(self.bm, cent=Vector((0, 0, 0)), matrix=rot, verts=verts)
        bmesh.ops.translate(self.bm, vec=Vector(center), verts=verts)

    def box(self, size, center, color, rot=None):
        r = bmesh.ops.create_cube(self.bm, size=1.0)
        self._place(r["verts"], center, size, rot)
        return self._paint(r["verts"], color)

    def cylinder(self, radius, depth, center, color, segments=12, scale=(1, 1, 1), rot=None):
        r = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segments,
                                  radius1=radius, radius2=radius, depth=depth)
        self._place(r["verts"], center, scale, rot)
        return self._paint(r["verts"], color)

    def cone(self, radius, depth, center, color, segments=6, cap=True, rot=None):
        r = bmesh.ops.create_cone(self.bm, cap_ends=cap, cap_tris=False, segments=segments,
                                  radius1=radius, radius2=0.0, depth=depth)
        self._place(r["verts"], center, (1, 1, 1), rot)
        return self._paint(r["verts"], color)

    def icosphere(self, radius, center, color, subdivisions=1, scale=(1, 1, 1)):
        r = bmesh.ops.create_icosphere(self.bm, subdivisions=subdivisions, radius=radius)
        self._place(r["verts"], center, scale)
        return self._paint(r["verts"], color)

    def finish(self, name):
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces[:])
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        mesh.update()
        tris = sum(len(f.verts) - 2 for f in self.bm.faces)
        xs = [v.co.x for v in self.bm.verts]
        ys = [v.co.y for v in self.bm.verts]
        zs = [v.co.z for v in self.bm.verts]
        self.bm.free()
        # Unity axes: x, y(up)=blender z, z(forward)=blender -y
        size = [max(xs) - min(xs), max(zs) - min(zs), max(ys) - min(ys)]
        return mesh, tris, size


def rot_z_to(direction):
    """Matrix rotating local +Z onto `direction`."""
    return Vector((0, 0, 1)).rotation_difference(Vector(direction).normalized()).to_matrix()


# ---------------------------------------------------------------- models

def build_player(b):
    b.cylinder(0.35, 0.9, (0, 0, 0.75), "player_blue", segments=12)       # torso 0.3..1.2
    b.icosphere(0.35, (0, 0, 1.2), "player_blue")                            # shoulders
    b.icosphere(0.3, (0, 0, 1.5), "skin")                                    # head 1.2..1.8
    b.box((0.08, 0.06, 0.08), (-0.11, -0.27, 1.55), "eye_black")
    b.box((0.08, 0.06, 0.08), (0.11, -0.27, 1.55), "eye_black")
    b.box((0.12, 0.55, 0.12), (0.3, -0.35, 1.0), "gun_metal")               # gun points -Y
    b.box((0.14, 0.14, 0.14), (0.3, -0.05, 1.0), "player_dark")              # grip
    b.box((0.22, 0.32, 0.3), (-0.17, 0.0, 0.15), "player_dark")              # feet
    b.box((0.22, 0.32, 0.3), (0.17, 0.0, 0.15), "player_dark")
    return "feet"


def build_enemy_blob(b):
    b.icosphere(0.4, (0, 0, 0.3), "blob_green", scale=(1, 1, 0.75))         # 0..0.6
    for sx in (-1, 1):
        b.box((0.11, 0.08, 0.11), (sx * 0.14, -0.33, 0.38), "eye_white")
        b.box((0.06, 0.05, 0.06), (sx * 0.14, -0.38, 0.38), "eye_black")
    return "feet"


def build_enemy_spiky(b):
    c = (0, 0, 0.5)
    b.icosphere(0.35, c, "spiky_purple")                                      # 0.15..0.85
    for i in range(10):
        az = math.radians(i * 36)
        el = math.radians(35 if i % 2 == 0 else -5)
        d = Vector((math.cos(az) * math.cos(el), math.sin(az) * math.cos(el), math.sin(el)))
        pos = Vector(c) + d * (0.35 + 0.15)
        b.cone(0.1, 0.36, pos, "spiky_purple", segments=6, rot=rot_z_to(d))
    for sx in (-1, 1):
        b.box((0.1, 0.07, 0.1), (sx * 0.13, -0.33, 0.58), "eye_white")
        b.box((0.05, 0.05, 0.05), (sx * 0.13, -0.37, 0.58), "eye_black")
    return "feet"


def build_enemy_tank(b):
    b.box((1.6, 1.2, 0.8), (0, 0, 0.6), "tank_rust")                         # body 0.2..1.0
    b.box((0.4, 1.4, 0.5), (-0.9, 0, 0.3), "player_dark")                    # treads
    b.box((0.4, 1.4, 0.5), (0.9, 0, 0.3), "player_dark")
    b.cylinder(0.42, 0.4, (0, 0, 1.2), "tank_rust", segments=8)              # turret 1.0..1.4
    b.box((0.16, 0.9, 0.16), (0, -0.7, 1.2), "gun_metal")                    # barrel -Y
    b.box((0.2, 0.2, 0.1), (0, 0, 1.45), "eye_black")                        # hatch
    return "feet"


def build_projectile(b):
    b.icosphere(0.15, (0, 0, 0), "projectile_yellow", subdivisions=0, scale=(1, 2.6, 1))
    return "center"


def build_pickup_xp(b):
    b.cone(0.2, 0.22, (0, 0, 0.11), "gem_cyan", segments=6, cap=False)
    b.cone(0.2, 0.22, (0, 0, -0.11), "gem_cyan", segments=6, cap=False,
           rot=Matrix.Rotation(math.pi, 3, "X"))
    return "center"


def build_pickup_health(b):
    b.box((0.4, 0.12, 0.13), (0, 0, 0), "heart_red")
    b.box((0.13, 0.12, 0.4), (0, 0, 0), "heart_red")
    return "center"


def build_arena_floor(b):
    b.cylinder(22.0, 0.3, (0, 0, -0.15), "floor_light", segments=32)        # top at z=0
    b.cylinder(14.0, 0.32, (0, 0, -0.15), "floor_dark", segments=24)        # ring top z=0.01
    b.cylinder(3.0, 0.34, (0, 0, -0.15), "floor_dark", segments=16)         # centre mark
    return "top"


def build_pillar(b):
    b.cylinder(0.5, 3.0, (0, 0, 1.5), "pillar_stone", segments=8)
    b.box((1.3, 1.3, 0.25), (0, 0, 3.08), "pillar_stone")
    b.box((1.3, 1.3, 0.2), (0, 0, 0.1), "floor_dark")
    return "feet"


def build_crate(b):
    b.box((0.8, 0.8, 0.8), (0, 0, 0.4), "crate_wood")
    b.box((0.84, 0.84, 0.1), (0, 0, 0.15), "player_dark")
    b.box((0.84, 0.84, 0.1), (0, 0, 0.65), "player_dark")
    return "feet"


MODELS = {
    "player": build_player,
    "enemy_blob": build_enemy_blob,
    "enemy_spiky": build_enemy_spiky,
    "enemy_tank": build_enemy_tank,
    "projectile": build_projectile,
    "pickup_xp": build_pickup_xp,
    "pickup_health": build_pickup_health,
    "arena_floor": build_arena_floor,
    "pillar": build_pillar,
    "crate": build_crate,
}


# ---------------------------------------------------------------- export

def ensure_fbx_exporter():
    if hasattr(bpy.ops.export_scene, "fbx") and bpy.ops.export_scene.fbx.poll() is not None:
        return
    bpy.ops.preferences.addon_enable(module="io_scene_fbx")


def export_fbx(obj, path):
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
        mesh_smooth_type="OFF",
        use_triangles=True,
        path_mode="COPY",
        embed_textures=False,
        add_leaf_bones=False,
        bake_anim=False,
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True, help="output directory for .fbx files")
    ap.add_argument("--only", nargs="*", help="subset of model names")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    ensure_fbx_exporter()

    manifest = {}
    names = args.only or list(MODELS)
    failed = []
    for name in names:
        b = Builder()
        origin = MODELS[name](b)
        mesh, tris, size = b.finish(name)
        obj = bpy.data.objects.new(name, mesh)
        scene.collection.objects.link(obj)
        path = os.path.join(args.out, f"{name}.fbx")
        export_fbx(obj, path)
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(mesh)
        manifest[name] = {
            "tris": tris,
            "size": [round(s, 4) for s in size],
            "origin": origin,
            "budget": BUDGETS[name],
        }
        ok = tris <= BUDGETS[name]
        if not ok:
            failed.append(name)
        print(f"{name:14s} tris={tris:4d}/{BUDGETS[name]:<4d} size={manifest[name]['size']} {'OK' if ok else 'OVER BUDGET'}")

    # JsonUtility-friendly layout (list, not dict) for the Unity bootstrap check.
    with open(os.path.join(args.out, "models_manifest.json"), "w") as f:
        json.dump({"models": [dict(name=k, **v) for k, v in manifest.items()]}, f, indent=2)
    if failed:
        print("over budget:", failed)
        sys.exit(1)


if __name__ == "__main__":
    main()
