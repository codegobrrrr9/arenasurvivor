"""Container-side sanity checks for a Unity project that cannot be opened here.

    python3 -I tools/check/check_project.py /path/to/arena-survivor
"""
import json
import os
import re
import struct
import sys
import wave
import zlib

FAILS = []


def fail(msg):
    FAILS.append(msg)
    print("FAIL", msg)


def ok(msg):
    print(" ok ", msg)


def check_json(path):
    try:
        with open(path) as f:
            json.load(f)
        ok(path)
    except Exception as e:
        fail(f"{path}: {e}")


def check_yaml_header(path):
    with open(path) as f:
        head = f.read(200)
    if head.startswith("%YAML 1.1\n%TAG !u! tag:unity3d.com,2011:\n--- !u!"):
        ok(path)
    else:
        fail(f"{path}: missing Unity YAML header")


def check_wav(path):
    try:
        with wave.open(path, "rb") as w:
            if w.getnchannels() != 1 or w.getsampwidth() != 2:
                fail(f"{path}: expected mono 16-bit")
            elif w.getnframes() == 0:
                fail(f"{path}: empty")
            else:
                ok(f"{path} ({w.getnframes() / w.getframerate():.2f}s)")
    except Exception as e:
        fail(f"{path}: {e}")


def check_png(path):
    with open(path, "rb") as f:
        data = f.read()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        return fail(f"{path}: bad signature")
    pos = 8
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        crc = struct.unpack(">I", data[pos + 8 + length:pos + 12 + length])[0]
        if zlib.crc32(tag + body) & 0xFFFFFFFF != crc:
            return fail(f"{path}: CRC mismatch in {tag}")
        pos += 12 + length
        if tag == b"IEND":
            break
    ok(path)


def check_fbx(path, entry):
    with open(path, "rb") as f:
        head = f.read(23)
    if not head.startswith(b"Kaydara FBX Binary"):
        return fail(f"{path}: not a binary FBX")
    with open(path, "rb") as f:
        blob = f.read()
    if b"LayerElementUV" not in blob:
        return fail(f"{path}: no UV layer")
    if entry["tris"] > entry["budget"]:
        return fail(f"{path}: {entry['tris']} tris over budget {entry['budget']}")
    ok(f"{path} ({entry['tris']} tris, size {entry['size']})")


CS_RULES = [
    (re.compile(r"\bInput\.(GetKey|GetAxis|GetButton|GetMouseButton|mousePosition)\b"), "legacy Input API"),
    (re.compile(r"\busing TMPro;"), "TextMeshPro reference"),
    (re.compile(r"\brigidbody\.velocity\b|\.velocity\s*="), "Rigidbody.velocity (use linearVelocity)"),
    (re.compile(r"\bFindObjectOfType\b"), "FindObjectOfType (use FindFirstObjectByType)"),
]


def check_cs(path, is_editor):
    with open(path) as f:
        src = f.read()
    name = os.path.splitext(os.path.basename(path))[0]
    problems = []
    if src.count("{") != src.count("}"):
        problems.append("unbalanced braces")
    if src.count("(") != src.count(")"):
        problems.append("unbalanced parentheses")
    if not re.search(rf"\b(class|interface|enum|struct)\s+{re.escape(name)}\b", src):
        problems.append(f"no type named {name}")
    if "using UnityEditor" in src and not is_editor:
        # allowed only inside #if UNITY_EDITOR
        stripped = re.sub(r"#if UNITY_EDITOR.*?#(else|endif)", "", src, flags=re.S)
        if "UnityEditor" in stripped:
            problems.append("UnityEditor used outside Editor folder / #if UNITY_EDITOR")
    for rx, label in CS_RULES:
        if rx.search(src):
            problems.append(label)
    if problems:
        fail(f"{path}: {', '.join(problems)}")
    else:
        ok(path)


def main(root):
    os.chdir(root)
    check_json("Packages/manifest.json")
    check_yaml_header("ProjectSettings/ProjectSettings.asset")
    if not os.path.exists("ProjectSettings/ProjectVersion.txt"):
        fail("ProjectVersion.txt missing")
    check_png("Assets/Art/Textures/palette.png")
    for f in sorted(os.listdir("Assets/Audio")):
        if f.endswith(".wav"):
            check_wav(os.path.join("Assets/Audio", f))
    with open("Assets/Art/Models/models_manifest.json") as f:
        manifest = {m["name"]: m for m in json.load(f)["models"]}
    for name, entry in manifest.items():
        check_fbx(f"Assets/Art/Models/{name}.fbx", entry)
    for dirpath, _, files in os.walk("Assets/Scripts"):
        for f in sorted(files):
            if f.endswith(".cs"):
                check_cs(os.path.join(dirpath, f), "/Editor" in dirpath)
    for dirpath, _, files in os.walk("Assets"):
        for f in files:
            if f.endswith(".meta"):
                fail(f"unexpected .meta shipped: {os.path.join(dirpath, f)}")
    print()
    if FAILS:
        print(f"{len(FAILS)} problem(s)")
        sys.exit(1)
    print("all checks passed")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else ".")
