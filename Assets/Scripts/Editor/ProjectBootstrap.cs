using System;
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace Arena.EditorTools
{
    /// <summary>
    /// Generates everything that is normally authored in the Unity editor: URP settings,
    /// importer settings, materials, ScriptableObject definitions, prefabs, the scene and
    /// the build list. Runs once automatically on first open (with a prompt) and can be
    /// re-run any time from Arena > Build Project. Every step is idempotent.
    /// </summary>
    public static class ProjectBootstrap
    {
        public const string GeneratedRoot = "Assets/Generated";
        public const string SettingsDir = GeneratedRoot + "/Settings";
        public const string MaterialsDir = GeneratedRoot + "/Materials";
        public const string DataDir = GeneratedRoot + "/Data";
        public const string PrefabsDir = GeneratedRoot + "/Prefabs";
        public const string ScenesDir = GeneratedRoot + "/Scenes";
        public const string MarkerAsset = DataDir + "/GameAssets.asset";
        public const string ScenePath = ScenesDir + "/Arena.unity";

        private const string SessionKey = "Arena.BootstrapPrompted";
        private static int _step;
        private const int TotalSteps = 11;

        [InitializeOnLoadMethod]
        private static void AutoRun()
        {
            EditorApplication.delayCall += TryPrompt;
        }

        private static void TryPrompt()
        {
            if (Application.isPlaying || BuildPipeline.isBuildingPlayer) return;
            if (EditorApplication.isCompiling || EditorApplication.isUpdating)
            {
                EditorApplication.delayCall += TryPrompt;
                return;
            }
            if (File.Exists(MarkerAsset)) return;
            if (SessionState.GetBool(SessionKey, false)) return;
            SessionState.SetBool(SessionKey, true);

            if (Application.isBatchMode)
            {
                Run();
                return;
            }
            bool go = EditorUtility.DisplayDialog(
                "Arena Survivor",
                "This project generates its scene, prefabs, materials and render settings from code.\n\n" +
                "Build them now? You can also run it later from the menu: Arena > Build Project.",
                "Build now", "Later");
            if (go) Run();
        }

        [MenuItem("Arena/Build Project", priority = 0)]
        public static void Run()
        {
            _step = 0;
            Debug.Log("[Arena] Bootstrap starting");
            bool allOk = true;

            allOk &= Step("Create folders", CreateFolders);
            allOk &= Step("Player settings", BootstrapSettings.ConfigurePlayerSettings);
            allOk &= Step("Layers and physics", BootstrapSettings.ConfigureLayers);
            allOk &= Step("URP pipeline", () => BootstrapSettings.ConfigureUrp());
            allOk &= Step("Importers", BootstrapAssets.ConfigureImporters);
            allOk &= Step("Model scale check", BootstrapAssets.CheckModelScale);

            Material material = null;
            allOk &= Step("Materials", () => material = BootstrapAssets.CreateMaterials());

            GameAssets assets = null;
            allOk &= Step("Definitions and prefabs", () =>
            {
                assets = BootstrapAssets.CreateGameAssets();
                BootstrapPrefabs.BuildAll(assets, material);
                BootstrapAssets.FinaliseGameAssets(assets);
            });
            allOk &= Step("Scene", () => BootstrapScene.Build(assets, material));
            allOk &= Step("Build settings", () =>
            {
                EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
            });
            allOk &= Step("Save", () =>
            {
                AssetDatabase.SaveAssets();
                AssetDatabase.Refresh();
            });

            if (allOk)
            {
                Debug.Log("[Arena] Bootstrap complete. Press Play to test, or Arena > Build WebGL to build.");
                if (!Application.isBatchMode) EditorSceneManager.OpenScene(ScenePath);
            }
            else
            {
                Debug.LogError("[Arena] Bootstrap finished with errors. Fix the issue above and re-run Arena > Build Project (it is safe to re-run).");
            }
        }

        [MenuItem("Arena/Delete Generated Assets", priority = 20)]
        public static void DeleteGenerated()
        {
            if (!EditorUtility.DisplayDialog("Arena Survivor", "Delete Assets/Generated (scene, prefabs, materials, settings)? Re-run Arena > Build Project afterwards.", "Delete", "Cancel"))
                return;
            EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);
            AssetDatabase.DeleteAsset(GeneratedRoot);
            AssetDatabase.Refresh();
            Debug.Log("[Arena] Deleted " + GeneratedRoot);
        }

        private static bool Step(string name, Action action)
        {
            _step++;
            try
            {
                action();
                Debug.Log($"[Arena] step {_step}/{TotalSteps}: {name} - ok");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[Arena] step {_step}/{TotalSteps}: {name} - FAILED\n{e}");
                return false;
            }
        }

        private static void CreateFolders()
        {
            EnsureFolder(GeneratedRoot);
            EnsureFolder(SettingsDir);
            EnsureFolder(MaterialsDir);
            EnsureFolder(DataDir);
            EnsureFolder(PrefabsDir);
            EnsureFolder(ScenesDir);
        }

        public static void EnsureFolder(string path)
        {
            if (AssetDatabase.IsValidFolder(path)) return;
            var parent = Path.GetDirectoryName(path)?.Replace('\\', '/');
            var name = Path.GetFileName(path);
            if (!string.IsNullOrEmpty(parent) && !AssetDatabase.IsValidFolder(parent)) EnsureFolder(parent);
            AssetDatabase.CreateFolder(parent, name);
        }

        /// <summary>Loads the asset at <paramref name="path"/> or creates it, then always applies <paramref name="init"/>.</summary>
        public static T LoadOrCreate<T>(string path, Action<T> init = null) where T : ScriptableObject
        {
            var asset = AssetDatabase.LoadAssetAtPath<T>(path);
            if (asset == null)
            {
                asset = ScriptableObject.CreateInstance<T>();
                AssetDatabase.CreateAsset(asset, path);
            }
            init?.Invoke(asset);
            EditorUtility.SetDirty(asset);
            return asset;
        }
    }
}
