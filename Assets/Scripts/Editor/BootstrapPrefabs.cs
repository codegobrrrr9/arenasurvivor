using UnityEditor;
using UnityEngine;

namespace Arena.EditorTools
{
    public static class BootstrapPrefabs
    {
        public static string PrefabPath(string name) => $"{ProjectBootstrap.PrefabsDir}/{name}.prefab";

        public static void BuildAll(GameAssets assets, Material material)
        {
            BuildPlayer(material);
            BuildProjectile(material);
            BuildPickup("PickupXp", "pickup_xp", PickupType.Xp, material);
            BuildPickup("PickupHealth", "pickup_health", PickupType.Health, material);
            BuildProp("Pillar", "pillar", material, root =>
            {
                var c = root.AddComponent<CapsuleCollider>();
                c.center = new Vector3(0f, 1.6f, 0f);
                c.height = 3.2f;
                c.radius = 0.62f;
            });
            BuildProp("Crate", "crate", material, root =>
            {
                var c = root.AddComponent<BoxCollider>();
                c.center = new Vector3(0f, 0.4f, 0f);
                c.size = new Vector3(0.84f, 0.8f, 0.84f);
            });
            foreach (var def in assets.enemies) BuildEnemy(def, material);
        }

        private static GameObject BuildPlayer(Material material)
        {
            var root = new GameObject("Player");
            root.layer = LayerMask.NameToLayer("Player");

            var col = root.AddComponent<CapsuleCollider>();
            col.center = new Vector3(0f, 0.9f, 0f);
            col.height = 1.8f;
            col.radius = 0.4f;

            var rb = root.AddComponent<Rigidbody>();
            rb.mass = 3f;
            rb.useGravity = false;
            rb.linearDamping = 0f;
            rb.angularDamping = 0f;
            rb.constraints = RigidbodyConstraints.FreezeRotation | RigidbodyConstraints.FreezePositionY;
            rb.interpolation = RigidbodyInterpolation.Interpolate;
            rb.collisionDetectionMode = CollisionDetectionMode.Continuous;

            root.AddComponent<PlayerStats>();
            var health = root.AddComponent<Health>();
            health.Configure(100f);
            health.invulnerabilityAfterHit = 0.25f;
            root.AddComponent<PlayerAim>();
            root.AddComponent<PlayerController>();
            var shooter = root.AddComponent<PlayerShooter>();

            AttachModel(root, "player", material);

            // Blender +X becomes Unity -X on import, so the gun (modelled at +X) sits at -X here.
            var muzzle = new GameObject("Muzzle").transform;
            muzzle.SetParent(root.transform, false);
            muzzle.localPosition = new Vector3(-0.3f, 1.0f, 0.7f);
            shooter.muzzle = muzzle;

            return Save(root, "Player");
        }

        private static GameObject BuildProjectile(Material material)
        {
            var root = new GameObject("Projectile");
            root.layer = LayerMask.NameToLayer("Projectile");
            root.AddComponent<Projectile>();
            var model = AttachModel(root, "projectile", material);
            foreach (var r in model.GetComponentsInChildren<MeshRenderer>())
            {
                r.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            }
            return Save(root, "Projectile");
        }

        private static GameObject BuildPickup(string name, string model, PickupType type, Material material)
        {
            var root = new GameObject(name);
            root.layer = LayerMask.NameToLayer("Pickup");
            var col = root.AddComponent<SphereCollider>();
            col.isTrigger = true;
            col.radius = 0.45f;
            var rb = root.AddComponent<Rigidbody>();
            rb.isKinematic = true;
            rb.useGravity = false;
            var pickup = root.AddComponent<Pickup>();
            pickup.type = type;
            AttachModel(root, model, material);
            return Save(root, name);
        }

        private static GameObject BuildProp(string name, string model, Material material, System.Action<GameObject> addCollider)
        {
            var root = new GameObject(name);
            root.layer = LayerMask.NameToLayer("Obstacle");
            root.isStatic = true;
            addCollider(root);
            var m = AttachModel(root, model, material);
            m.isStatic = true;
            return Save(root, name);
        }

        private static GameObject BuildEnemy(EnemyDefinition def, Material material)
        {
            var root = new GameObject("Enemy_" + def.id);
            root.layer = LayerMask.NameToLayer("Enemy");

            switch (def.id)
            {
                case "tank":
                    var box = root.AddComponent<BoxCollider>();
                    box.center = new Vector3(0f, 0.72f, 0f);
                    box.size = new Vector3(2.0f, 1.4f, 1.7f);
                    break;
                case "spiky":
                    var s = root.AddComponent<SphereCollider>();
                    s.center = new Vector3(0f, 0.5f, 0f);
                    s.radius = 0.5f;
                    break;
                default:
                    var b = root.AddComponent<SphereCollider>();
                    b.center = new Vector3(0f, 0.32f, 0f);
                    b.radius = 0.4f;
                    break;
            }

            var rb = root.AddComponent<Rigidbody>();
            rb.mass = def.mass;
            rb.useGravity = false;
            rb.linearDamping = 1.5f;
            rb.constraints = RigidbodyConstraints.FreezeRotation | RigidbodyConstraints.FreezePositionY;
            rb.interpolation = RigidbodyInterpolation.Interpolate;

            var health = root.AddComponent<Health>();
            health.Configure(def.hp);
            var enemy = root.AddComponent<Enemy>();
            enemy.definition = def;

            AttachModel(root, "enemy_" + def.id, material);
            return Save(root, "Enemy_" + def.id);
        }

        /// <summary>Instantiates the imported FBX as a nested prefab under <paramref name="root"/> and applies the palette material.</summary>
        public static GameObject AttachModel(GameObject root, string modelName, Material material)
        {
            var asset = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapAssets.ModelPath(modelName));
            if (asset == null) throw new System.Exception($"Model {modelName}.fbx not found in {BootstrapAssets.ModelsDir}");
            var instance = (GameObject)PrefabUtility.InstantiatePrefab(asset);
            instance.name = "Model";
            instance.transform.SetParent(root.transform, false);
            instance.transform.localPosition = Vector3.zero;
            instance.transform.localRotation = Quaternion.identity;
            ApplyMaterial(instance, material);
            foreach (var t in instance.GetComponentsInChildren<Transform>(true)) t.gameObject.layer = root.layer;
            return instance;
        }

        public static void ApplyMaterial(GameObject go, Material material)
        {
            if (material == null) return;
            foreach (var r in go.GetComponentsInChildren<MeshRenderer>(true))
            {
                int n = Mathf.Max(1, r.sharedMaterials.Length);
                var mats = new Material[n];
                for (int i = 0; i < n; i++) mats[i] = material;
                r.sharedMaterials = mats;
            }
        }

        private static GameObject Save(GameObject root, string name)
        {
            var path = PrefabPath(name);
            var prefab = PrefabUtility.SaveAsPrefabAsset(root, path);
            Object.DestroyImmediate(root);
            return prefab;
        }
    }
}
