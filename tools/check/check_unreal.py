"""Container-side sanity checks for the Unreal project (no engine available here).

    python3 -I tools/check/check_unreal.py unreal/ArenaSurvivor
"""
import json
import os
import py_compile
import re
import struct
import sys
import wave

FAILS = []


def fail(msg):
    FAILS.append(msg)
    print("FAIL", msg)


def ok(msg):
    print(" ok ", msg)


# Engine symbol -> header that must be included somewhere in the file's include chain.
SYMBOL_HEADERS = {
    "UCapsuleComponent": "Components/CapsuleComponent.h",
    "UStaticMeshComponent": "Components/StaticMeshComponent.h",
    "UGameplayStatics": "Kismet/GameplayStatics.h",
    "UKismetSystemLibrary": "Kismet/KismetSystemLibrary.h",
    "ACameraActor": "Camera/CameraActor.h",
    "UCameraComponent": "Camera/CameraComponent.h",
    "USaveGame": "GameFramework/SaveGame.h",
    "UWidgetTree": "Blueprint/WidgetTree.h",
    "UCanvasPanelSlot": "Components/CanvasPanelSlot.h",
    "UCanvasPanel": "Components/CanvasPanel.h",
    "UTextBlock": "Components/TextBlock.h",
    "UButton": "Components/Button.h",
    "UProgressBar": "Components/ProgressBar.h",
    "UBorder": "Components/Border.h",
    "UVerticalBox": "Components/VerticalBox.h",
    "UVerticalBoxSlot": "Components/VerticalBoxSlot.h",
    "UEnhancedInputComponent": "EnhancedInputComponent.h",
    "UEnhancedInputLocalPlayerSubsystem": "EnhancedInputSubsystems.h",
    "UInputMappingContext": "InputMappingContext.h",
    "UInputAction": "InputAction.h",
    "UInputModifierNegate": "InputModifiers.h",
    "UInputModifierSwizzleAxis": "InputModifiers.h",
    "UInputModifierDeadZone": "InputModifiers.h",
    "FInputActionValue": "InputActionValue.h",
    "AStaticMeshActor": "Engine/StaticMeshActor.h",
    "ADirectionalLight": "Engine/DirectionalLight.h",
    "ASkyLight": "Engine/SkyLight.h",
    "USkyLightComponent": "Components/SkyLightComponent.h",
    "ULightComponent": "Components/LightComponent.h",
    "TActorIterator": "EngineUtils.h",
    "ULocalPlayer": "Engine/LocalPlayer.h",
    "CreateWidget": "Blueprint/UserWidget.h",
    "FCollisionObjectQueryParams": "CollisionQueryParams.h",
    "UArenaHealthComponent": "ArenaHealthComponent.h",
    "AArenaGameMode": "ArenaGameMode.h",
    "UArenaAssets": "ArenaAssets.h",
    "AArenaPlayerCharacter": "ArenaPlayerCharacter.h",
    "AArenaEnemy": "ArenaEnemy.h",
    "UArenaWaveSpawner": "ArenaWaveSpawner.h",
    "UArenaUpgradeSystem": "ArenaUpgradeSystem.h",
    "AArenaProjectile": "ArenaProjectile.h",
    "AArenaPickup": "ArenaPickup.h",
    "AArenaCameraActor": "ArenaCameraActor.h",
    "AArenaPlayerController": "ArenaPlayerController.h",
    "UArenaSaveGame": "ArenaSaveGame.h",
    "ArenaMovement::": "ArenaMovement.h",
}
# Modules implied by headers (checked against Build.cs).
HEADER_MODULES = [
    (re.compile(r"EnhancedInput|InputMappingContext|InputAction|InputModifiers"), "EnhancedInput"),
    (re.compile(r"Blueprint/UserWidget|Blueprint/WidgetTree|Components/(CanvasPanel|TextBlock|Button|ProgressBar|Border|VerticalBox)"), "UMG"),
    (re.compile(r"InputCoreTypes|EKeys::"), "InputCore"),
]


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def includes_of(src):
    return re.findall(r'#include\s+"([^"]+)"', src)


def strip_comments(src):
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    return re.sub(r"//[^\n]*", "", src)


def check_header(path, src, module_name):
    name = os.path.splitext(os.path.basename(path))[0]
    code = strip_comments(src)
    problems = []
    incs = includes_of(src)
    reflected = re.search(r"\b(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(", code) is not None
    if reflected:
        gen = f"{name}.generated.h"
        if gen not in incs:
            problems.append(f"missing #include \"{gen}\"")
        elif incs[-1] != gen:
            problems.append("generated.h must be the last #include")
    for m in re.finditer(r"\bUCLASS\s*\([^)]*\)\s*class\s+(?:\w+_API\s+)?(\w+)\s*:[^{]*\{\s*([^\n]*)", code):
        cls, first = m.group(1), m.group(2).strip()
        if not first.startswith("GENERATED_BODY()") and not first.startswith("GENERATED_UCLASS_BODY()"):
            problems.append(f"{cls}: GENERATED_BODY() must be the first line of the class body")
        if f"{module_name.upper()}_API" not in m.group(0):
            problems.append(f"{cls}: missing {module_name.upper()}_API")
    for m in re.finditer(r"\bUSTRUCT\s*\([^)]*\)\s*struct\s+(?:\w+_API\s+)?(\w+)[^{]*\{\s*([^\n]*)", code):
        if not m.group(2).strip().startswith("GENERATED_BODY()"):
            problems.append(f"{m.group(1)}: GENERATED_BODY() missing")
    for m in re.finditer(r"\bUENUM\s*\([^)]*\)\s*enum\s+(\w+)", code):
        if m.group(1) != "class":
            problems.append(f"UENUM {m.group(1)} should be an enum class")
    # TObjectPtr / raw UObject pointer members inside UCLASS bodies need UPROPERTY.
    if "UCLASS" in code:
        for line in code.splitlines():
            s = line.strip()
            if re.match(r"^(TObjectPtr<|TArray<TObjectPtr<|[AU]\w+\s*\*\s*\w+\s*(=|;))", s) and "UPROPERTY" not in s and not s.startswith("static"):
                if not re.search(r"\(|\)", s.split("=")[0]) or "TObjectPtr" in s:
                    problems.append(f"member without UPROPERTY: {s}")
    return problems


def check_cpp(path, src, headers_by_name):
    name = os.path.splitext(os.path.basename(path))[0]
    incs = includes_of(src)
    problems = []
    if not incs or os.path.basename(incs[0]) != f"{name}.h":
        problems.append("first #include must be its own header")
    # Transitive include walk over project headers.
    seen = set()
    todo = list(incs)
    while todo:
        inc = todo.pop()
        if inc in seen:
            continue
        seen.add(inc)
        base = os.path.basename(inc)
        if base in headers_by_name:
            todo.extend(includes_of(headers_by_name[base]))
    code = strip_comments(src)
    for symbol, header in SYMBOL_HEADERS.items():
        if symbol in code and header not in seen and os.path.basename(header) not in {os.path.basename(s) for s in seen}:
            if symbol.rstrip(":") + ".h" == os.path.basename(path).replace(".cpp", ".h"):
                continue
            problems.append(f"uses {symbol} but never includes {header}")
    for m in re.finditer(r"AddDynamic\(this,\s*&(\w+)::(\w+)\)", code):
        cls, fn = m.groups()
        hdr = headers_by_name.get(f"{cls[1:]}.h") or headers_by_name.get(f"{cls}.h")
        if hdr is None:
            # strip U/A prefix
            hdr = headers_by_name.get(f"{cls[1:]}.h")
        if hdr is not None and not re.search(r"UFUNCTION\(\)\s*(?:virtual\s+)?void\s+" + fn + r"\s*\(", hdr):
            problems.append(f"{cls}::{fn} bound with AddDynamic but not declared UFUNCTION()")
    if re.search(r"\bUE_LOG\(LogArena", code) and "ArenaSurvivor.h" not in seen:
        problems.append("UE_LOG(LogArena...) without including ArenaSurvivor.h")
    return problems


def check_cs(path, src, module):
    problems = []
    if path.endswith("Target.cs"):
        if "BuildSettingsVersion.V5" not in src:
            problems.append("expected BuildSettingsVersion.V5")
        if not re.search(r"EngineIncludeOrderVersion\.(Unreal5_6|Latest)", src):
            problems.append("expected EngineIncludeOrderVersion.Unreal5_6 or .Latest")
        if f'ExtraModuleNames.Add("{module}")' not in src:
            problems.append("module name mismatch in ExtraModuleNames")
        if "ArenaSurvivorEditor" in os.path.basename(path) and "TargetType.Editor" not in src:
            problems.append("editor target must be TargetType.Editor")
    return problems


def check_python(path):
    try:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            py_compile.compile(path, doraise=True, cfile=os.path.join(tmp, "out.pyc"))
        ok(path)
    except Exception as exc:
        fail(f"{path}: {exc}")


def check_fbx(path):
    with open(path, "rb") as f:
        blob = f.read()
    if not blob.startswith(b"Kaydara FBX Binary"):
        return fail(f"{path}: not binary FBX")
    i = blob.find(b"UnitScaleFactor")
    if i < 0:
        return fail(f"{path}: no UnitScaleFactor")
    # The property record: name, then type 'D' and 8-byte double shortly after.
    seg = blob[i:i + 120]
    j = seg.find(b"D")
    try:
        # find first double after the name whose value is plausible
        found = None
        for k in range(len(b"UnitScaleFactor"), len(seg) - 9):
            if seg[k:k + 1] == b"D":
                val = struct.unpack("<d", seg[k + 1:k + 9])[0]
                if 0.0001 < val < 10000:
                    found = val
                    break
        if found is None:
            return fail(f"{path}: could not read UnitScaleFactor")
        if abs(found - 1.0) > 1e-6:
            return fail(f"{path}: UnitScaleFactor {found} (expected 1.0 = centimetres)")
    except Exception as exc:
        return fail(f"{path}: {exc}")
    ok(f"{path} (cm units)")


def main(root):
    os.chdir(root)
    up = json.load(open("ArenaSurvivor.uproject"))
    module = up["Modules"][0]["Name"]
    if up.get("EngineAssociation") != "5.6":
        fail("EngineAssociation must be 5.6")
    plugins = {p["Name"] for p in up.get("Plugins", [])}
    for p in ("PythonScriptPlugin", "EditorScriptingUtilities", "EnhancedInput"):
        if p not in plugins:
            fail(f"plugin {p} not enabled in .uproject")
    ok("uproject")

    build_cs = read(f"Source/{module}/{module}.Build.cs")
    if f"public class {module} : ModuleRules" not in build_cs:
        fail("Build.cs class name mismatch")
    for p in ("Source/ArenaSurvivor.Target.cs", "Source/ArenaSurvivorEditor.Target.cs"):
        probs = check_cs(p, read(p), module)
        fail(f"{p}: {', '.join(probs)}") if probs else ok(p)

    sources = {}
    for dirpath, _, files in os.walk(f"Source/{module}"):
        for f in files:
            if f.endswith((".h", ".cpp")):
                p = os.path.join(dirpath, f)
                sources[p] = read(p)
    headers_by_name = {os.path.basename(p): s for p, s in sources.items() if p.endswith(".h")}

    all_src = "\n".join(sources.values())
    for rx, mod in HEADER_MODULES:
        if rx.search(all_src) and f'"{mod}"' not in build_cs:
            fail(f"Build.cs missing module {mod}")
    ok("Build.cs modules")

    for p, s in sorted(sources.items()):
        probs = check_header(p, s, module) if p.endswith(".h") else check_cpp(p, s, headers_by_name)
        if probs:
            fail(f"{p}: {'; '.join(probs)}")
        else:
            ok(p)

    # Every project include must exist.
    all_files = {os.path.relpath(os.path.join(d, f), f"Source/{module}").replace("\\", "/") for d, _, fs in os.walk(f"Source/{module}") for f in fs}
    for p, s in sources.items():
        for inc in includes_of(s):
            if inc.endswith(".generated.h") or "/" in inc and not inc.startswith("UI/"):
                continue
            if inc.startswith("UI/") or inc.startswith("Arena"):
                if inc not in all_files and os.path.basename(inc) not in {os.path.basename(a) for a in all_files}:
                    fail(f"{p}: include {inc} not found in project")

    for f in ("Content/Python/init_unreal.py", "Content/Python/arena_bootstrap.py"):
        check_python(f)

    eng = read("Config/DefaultEngine.ini")
    gm = re.search(r"GlobalDefaultGameMode=/Script/(\w+)\.(\w+)", eng)
    if not gm or gm.group(1) != module or f"class {module.upper()}_API A{gm.group(2)}" not in all_src:
        fail("GlobalDefaultGameMode does not match a C++ class")
    else:
        ok("ini game mode")
    for prof in ("ArenaPlayer", "ArenaEnemy"):
        if f'Profiles=(Name="{prof}"' not in eng:
            fail(f"collision profile {prof} missing in DefaultEngine.ini")
    py = read("Content/Python/arena_bootstrap.py")
    if 'MAP = "/Game/Maps/Arena"' not in py or "GameDefaultMap=/Game/Maps/Arena.Arena" not in eng:
        fail("map path mismatch between ini and Python")
    else:
        ok("map paths")

    for m in ["player", "enemy_blob", "enemy_spiky", "enemy_tank", "projectile", "pickup_xp", "pickup_health", "arena_floor", "pillar", "crate"]:
        check_fbx(f"RawAssets/Models/{m}.fbx")
    for s in ["shoot", "hit", "enemy_die", "pickup", "upgrade", "player_hurt", "game_over", "ui_click", "wave_start"]:
        with wave.open(f"RawAssets/Audio/{s}.wav", "rb") as w:
            if w.getsampwidth() != 2:
                fail(f"{s}.wav not 16-bit")
    if not os.path.exists("RawAssets/Textures/palette.png"):
        fail("palette.png missing")
    ok("raw assets")

    print()
    if FAILS:
        print(f"{len(FAILS)} problem(s)")
        sys.exit(1)
    print("all checks passed")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "unreal/ArenaSurvivor")
