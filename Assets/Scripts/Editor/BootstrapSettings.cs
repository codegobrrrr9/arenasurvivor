using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace Arena.EditorTools
{
    public static class BootstrapSettings
    {
        public static readonly string[] Layers = { "Player", "Enemy", "Projectile", "Pickup", "Obstacle" };

        public const string RendererPath = ProjectBootstrap.SettingsDir + "/ArenaRenderer.asset";
        public const string PipelinePath = ProjectBootstrap.SettingsDir + "/ArenaURP.asset";

        public static void ConfigurePlayerSettings()
        {
            PlayerSettings.productName = "Arena Survivor";
            PlayerSettings.companyName = "ArenaSurvivor";
            PlayerSettings.colorSpace = ColorSpace.Linear;
            PlayerSettings.runInBackground = true;
            PlayerSettings.defaultScreenWidth = 1280;
            PlayerSettings.defaultScreenHeight = 720;

            // WebGL: gzip + decompression fallback works on any static host (itch.io, GitHub Pages)
            // without special server headers.
            PlayerSettings.WebGL.compressionFormat = WebGLCompressionFormat.Gzip;
            PlayerSettings.WebGL.decompressionFallback = true;
            PlayerSettings.WebGL.dataCaching = true;
            PlayerSettings.WebGL.template = "APPLICATION:Default";

            EnsureInputSystemBackend();
        }

        private static void EnsureInputSystemBackend()
        {
            var objs = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/ProjectSettings.asset");
            if (objs == null || objs.Length == 0) return;
            var so = new SerializedObject(objs[0]);
            var prop = so.FindProperty("activeInputHandler");
            if (prop == null) return;
            if (prop.intValue != 1)
            {
                prop.intValue = 1; // Input System Package (New)
                so.ApplyModifiedPropertiesWithoutUndo();
                Debug.LogWarning("[Arena] Active Input Handling was switched to 'Input System Package (New)'. Restart the editor if input does not respond.");
            }
        }

        public static void ConfigureLayers()
        {
            var objs = AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/TagManager.asset");
            if (objs == null || objs.Length == 0) throw new System.Exception("TagManager.asset not found");
            var tm = new SerializedObject(objs[0]);
            var layers = tm.FindProperty("layers");

            foreach (var name in Layers)
            {
                if (FindLayer(layers, name) >= 0) continue;
                int slot = -1;
                for (int i = 8; i < layers.arraySize; i++)
                {
                    if (string.IsNullOrEmpty(layers.GetArrayElementAtIndex(i).stringValue)) { slot = i; break; }
                }
                if (slot < 0) throw new System.Exception("No free user layer for " + name);
                layers.GetArrayElementAtIndex(slot).stringValue = name;
            }
            tm.ApplyModifiedPropertiesWithoutUndo();

            int player = LayerMask.NameToLayer("Player");
            int enemy = LayerMask.NameToLayer("Enemy");
            int projectile = LayerMask.NameToLayer("Projectile");
            int pickup = LayerMask.NameToLayer("Pickup");
            int obstacle = LayerMask.NameToLayer("Obstacle");

            // Pickups only ever interact with the player.
            Physics.IgnoreLayerCollision(pickup, enemy, true);
            Physics.IgnoreLayerCollision(pickup, pickup, true);
            Physics.IgnoreLayerCollision(pickup, obstacle, true);
            Physics.IgnoreLayerCollision(pickup, projectile, true);
            Physics.IgnoreLayerCollision(pickup, player, false);
            // Projectiles are raycast-based and carry no collider, but keep the matrix tidy.
            Physics.IgnoreLayerCollision(projectile, player, true);
            Physics.IgnoreLayerCollision(projectile, projectile, true);
            Physics.IgnoreLayerCollision(projectile, enemy, true);
            Physics.IgnoreLayerCollision(projectile, obstacle, true);
        }

        private static int FindLayer(SerializedProperty layers, string name)
        {
            for (int i = 0; i < layers.arraySize; i++)
            {
                if (layers.GetArrayElementAtIndex(i).stringValue == name) return i;
            }
            return -1;
        }

        public static UniversalRenderPipelineAsset ConfigureUrp()
        {
            var rendererData = AssetDatabase.LoadAssetAtPath<UniversalRendererData>(RendererPath);
            if (rendererData == null)
            {
                rendererData = ScriptableObject.CreateInstance<UniversalRendererData>();
                AssetDatabase.CreateAsset(rendererData, RendererPath);
            }
            if (rendererData.postProcessData == null)
            {
                rendererData.postProcessData = AssetDatabase.LoadAssetAtPath<PostProcessData>(
                    "Packages/com.unity.render-pipelines.universal/Runtime/Data/PostProcessData.asset");
            }
            EditorUtility.SetDirty(rendererData);

            var pipeline = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(PipelinePath);
            if (pipeline == null)
            {
                pipeline = UniversalRenderPipelineAsset.Create(rendererData);
                AssetDatabase.CreateAsset(pipeline, PipelinePath);
            }
            pipeline.supportsHDR = false;
            pipeline.msaaSampleCount = 2;
            pipeline.shadowDistance = 45f;
            pipeline.mainLightShadowmapResolution = 1024;
            pipeline.supportsCameraDepthTexture = false;
            pipeline.supportsCameraOpaqueTexture = false;
            EditorUtility.SetDirty(pipeline);

            GraphicsSettings.defaultRenderPipeline = pipeline;
            int current = QualitySettings.GetQualityLevel();
            int count = QualitySettings.names.Length;
            for (int i = 0; i < count; i++)
            {
                QualitySettings.SetQualityLevel(i, false);
                QualitySettings.renderPipeline = pipeline;
            }
            QualitySettings.SetQualityLevel(current, false);
            AssetDatabase.SaveAssets();
            return pipeline;
        }
    }
}
