using System;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace Arena.EditorTools
{
    public static class BootstrapAssets
    {
        public const string ModelsDir = "Assets/Art/Models";
        public const string TexturePath = "Assets/Art/Textures/palette.png";
        public const string AudioDir = "Assets/Audio";
        public const string MaterialPath = ProjectBootstrap.MaterialsDir + "/Palette.mat";

        public static string ModelPath(string name) => $"{ModelsDir}/{name}.fbx";

        public static void ConfigureImporters()
        {
            if (AssetImporter.GetAtPath(TexturePath) is TextureImporter ti)
            {
                ti.textureType = TextureImporterType.Default;
                ti.sRGBTexture = true;
                ti.filterMode = FilterMode.Point;
                ti.mipmapEnabled = false;
                ti.wrapMode = TextureWrapMode.Clamp;
                ti.textureCompression = TextureImporterCompression.Uncompressed;
                ti.SaveAndReimport();
            }
            else
            {
                throw new Exception("palette.png not found at " + TexturePath);
            }

            foreach (var file in Directory.GetFiles(ModelsDir, "*.fbx"))
            {
                var path = file.Replace('\\', '/');
                if (!(AssetImporter.GetAtPath(path) is ModelImporter mi)) continue;
                mi.materialImportMode = ModelImporterMaterialImportMode.None;
                mi.importAnimation = false;
                mi.animationType = ModelImporterAnimationType.None;
                mi.importCameras = false;
                mi.importLights = false;
                mi.isReadable = false;
                mi.importNormals = ModelImporterNormals.Import;
                mi.bakeAxisConversion = false;   // the Blender exporter already baked the axis conversion
                mi.useFileScale = true;
                mi.globalScale = 1f;
                mi.SaveAndReimport();
            }

            foreach (var file in Directory.GetFiles(AudioDir, "*.wav"))
            {
                var path = file.Replace('\\', '/');
                if (!(AssetImporter.GetAtPath(path) is AudioImporter ai)) continue;
                var settings = ai.defaultSampleSettings;
                settings.loadType = AudioClipLoadType.DecompressOnLoad;
                settings.compressionFormat = AudioCompressionFormat.PCM;
                ai.defaultSampleSettings = settings;
                ai.SaveAndReimport();
            }
        }

        [Serializable] private class ModelEntry { public string name; public int tris; public float[] size; public string origin; public int budget; }
        [Serializable] private class Manifest { public ModelEntry[] models; }

        /// <summary>Compares imported mesh bounds against the sizes recorded by the Blender script.</summary>
        public static void CheckModelScale()
        {
            var manifestPath = ModelsDir + "/models_manifest.json";
            if (!File.Exists(manifestPath))
            {
                Debug.LogWarning("[Arena] models_manifest.json missing; skipping scale check");
                return;
            }
            var manifest = JsonUtility.FromJson<Manifest>(File.ReadAllText(manifestPath));
            if (manifest?.models == null) return;

            foreach (var entry in manifest.models)
            {
                var go = AssetDatabase.LoadAssetAtPath<GameObject>(ModelPath(entry.name));
                if (go == null)
                {
                    Debug.LogWarning($"[Arena] model {entry.name}.fbx not found");
                    continue;
                }
                var size = MeasureModel(go);
                float expectedHeight = entry.size[1];
                float ratio = expectedHeight > 0.001f ? size.y / expectedHeight : 1f;
                if (ratio < 0.9f || ratio > 1.1f)
                {
                    Debug.LogWarning($"[Arena] {entry.name}: imported height {size.y:F2} m, expected {expectedHeight:F2} m. " +
                                     "Check the FBX importer's Scale Factor / Convert Units, or set Bake Axis Conversion.");
                }
            }
        }

        private static Vector3 MeasureModel(GameObject root)
        {
            var filters = root.GetComponentsInChildren<MeshFilter>();
            if (filters.Length == 0) return Vector3.zero;
            var min = Vector3.positiveInfinity;
            var max = Vector3.negativeInfinity;
            foreach (var mf in filters)
            {
                if (mf.sharedMesh == null) continue;
                var b = mf.sharedMesh.bounds;
                var m = root.transform.worldToLocalMatrix * mf.transform.localToWorldMatrix;
                var corners = new[]
                {
                    new Vector3(b.min.x, b.min.y, b.min.z), new Vector3(b.max.x, b.min.y, b.min.z),
                    new Vector3(b.min.x, b.max.y, b.min.z), new Vector3(b.max.x, b.max.y, b.min.z),
                    new Vector3(b.min.x, b.min.y, b.max.z), new Vector3(b.max.x, b.min.y, b.max.z),
                    new Vector3(b.min.x, b.max.y, b.max.z), new Vector3(b.max.x, b.max.y, b.max.z),
                };
                foreach (var c in corners)
                {
                    var p = m.MultiplyPoint3x4(c);
                    min = Vector3.Min(min, p);
                    max = Vector3.Max(max, p);
                }
            }
            return max - min;
        }

        public static Material CreateMaterials()
        {
            var shader = Shader.Find("Universal Render Pipeline/Lit");
            if (shader == null) throw new Exception("URP Lit shader not found. Is the URP package installed and active?");
            var palette = AssetDatabase.LoadAssetAtPath<Texture2D>(TexturePath);

            var mat = AssetDatabase.LoadAssetAtPath<Material>(MaterialPath);
            if (mat == null)
            {
                mat = new Material(shader);
                AssetDatabase.CreateAsset(mat, MaterialPath);
            }
            mat.shader = shader;
            mat.SetTexture("_BaseMap", palette);
            mat.SetColor("_BaseColor", Color.white);
            mat.SetFloat("_Smoothness", 0.15f);
            mat.SetFloat("_Metallic", 0f);
            EditorUtility.SetDirty(mat);
            return mat;
        }

        public static GameAssets CreateGameAssets()
        {
            var assets = ProjectBootstrap.LoadOrCreate<GameAssets>(ProjectBootstrap.MarkerAsset);
            assets.arenaRadius = 20f;
            assets.paletteMaterial = AssetDatabase.LoadAssetAtPath<Material>(MaterialPath);

            assets.enemies = new[]
            {
                EnemyDef("blob", "Blob", EnemyBehaviour.Chaser, hp: 30f, speed: 3.6f, damage: 10f, mass: 1f, resist: 0f, xp: 3, score: 10, heal: 0.04f, firstWave: 1),
                EnemyDef("spiky", "Spiky", EnemyBehaviour.Dasher, hp: 45f, speed: 4.2f, damage: 15f, mass: 1.2f, resist: 0.2f, xp: 5, score: 25, heal: 0.06f, firstWave: 3),
                EnemyDef("tank", "Tank", EnemyBehaviour.Tank, hp: 220f, speed: 1.8f, damage: 25f, mass: 12f, resist: 0.9f, xp: 12, score: 80, heal: 0.25f, firstWave: 5),
            };

            assets.upgrades = new[]
            {
                UpgradeDef("FireRate", "Rapid Fire", "+15% fire rate", StatType.FireRate, 1.15f, true, 8),
                UpgradeDef("Damage", "Heavy Rounds", "+20% damage", StatType.Damage, 1.2f, true, 8),
                UpgradeDef("MoveSpeed", "Swift Boots", "+10% move speed", StatType.MoveSpeed, 1.1f, true, 6),
                UpgradeDef("MaxHp", "Vitality", "+20 max HP and heal 25%", StatType.MaxHp, 20f, false, 8, healOnPick: true),
                UpgradeDef("Multishot", "Multishot", "+1 projectile per shot", StatType.ProjectileCount, 1f, false, 4),
                UpgradeDef("BulletSpeed", "Velocity", "+20% projectile speed", StatType.ProjectileSpeed, 1.2f, true, 4),
            };

            assets.shoot = Clip("shoot");
            assets.hit = Clip("hit");
            assets.enemyDie = Clip("enemy_die");
            assets.pickup = Clip("pickup");
            assets.upgrade = Clip("upgrade");
            assets.playerHurt = Clip("player_hurt");
            assets.gameOver = Clip("game_over");
            assets.uiClick = Clip("ui_click");
            assets.waveStart = Clip("wave_start");
            EditorUtility.SetDirty(assets);
            return assets;
        }

        /// <summary>Called after prefabs exist so the registry points at them.</summary>
        public static void FinaliseGameAssets(GameAssets assets)
        {
            assets.playerPrefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("Player"));
            assets.projectilePrefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("Projectile"));
            assets.xpPickupPrefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("PickupXp"));
            assets.healthPickupPrefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("PickupHealth"));
            assets.pillarPrefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("Pillar"));
            assets.cratePrefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("Crate"));
            foreach (var def in assets.enemies)
            {
                def.prefab = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapPrefabs.PrefabPath("Enemy_" + def.id));
                EditorUtility.SetDirty(def);
            }
            EditorUtility.SetDirty(assets);
        }

        private static AudioClip Clip(string name) => AssetDatabase.LoadAssetAtPath<AudioClip>($"{AudioDir}/{name}.wav");

        private static EnemyDefinition EnemyDef(string id, string display, EnemyBehaviour behaviour, float hp, float speed, float damage,
            float mass, float resist, int xp, int score, float heal, int firstWave)
        {
            return ProjectBootstrap.LoadOrCreate<EnemyDefinition>($"{ProjectBootstrap.DataDir}/Enemy_{id}.asset", d =>
            {
                d.id = id;
                d.displayName = display;
                d.behaviour = behaviour;
                d.hp = hp;
                d.moveSpeed = speed;
                d.contactDamage = damage;
                d.contactCooldown = 0.8f;
                d.mass = mass;
                d.knockbackResistance = resist;
                d.xpValue = xp;
                d.scoreValue = score;
                d.dropHealthChance = heal;
                d.firstWave = firstWave;
            });
        }

        private static UpgradeDefinition UpgradeDef(string file, string title, string description, StatType stat, float value, bool multiplier, int maxStacks, bool healOnPick = false)
        {
            return ProjectBootstrap.LoadOrCreate<UpgradeDefinition>($"{ProjectBootstrap.DataDir}/Upgrade_{file}.asset", u =>
            {
                u.title = title;
                u.description = description;
                u.stat = stat;
                u.value = value;
                u.isMultiplier = multiplier;
                u.maxStacks = maxStacks;
                u.healOnPick = healOnPick;
            });
        }
    }
}
