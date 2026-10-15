using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.UI;

namespace Arena.EditorTools
{
    public static class BootstrapScene
    {
        private const string DefaultUiActions = "Packages/com.unity.inputsystem/InputSystem/Plugins/PlayerInput/DefaultInputActions.inputactions";

        public static void Build(GameAssets assets, Material material)
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            RenderSettings.ambientMode = AmbientMode.Flat;
            RenderSettings.ambientLight = new Color(0.42f, 0.44f, 0.52f);
            RenderSettings.fog = false;
            RenderSettings.skybox = null;

            BuildCamera();
            BuildLight();
            BuildArena(assets, material);
            BuildSystems(assets);

            var player = (GameObject)PrefabUtility.InstantiatePrefab(assets.playerPrefab);
            player.transform.position = Vector3.zero;

            BuildEventSystem();
            BuildUi();

            EditorSceneManager.SaveScene(scene, ProjectBootstrap.ScenePath);
        }

        private static void BuildCamera()
        {
            var go = new GameObject("Main Camera");
            go.tag = "MainCamera";
            var cam = go.AddComponent<Camera>();
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = new Color(0.05f, 0.06f, 0.09f);
            cam.fieldOfView = 50f;
            cam.nearClipPlane = 0.3f;
            cam.farClipPlane = 150f;
            go.AddComponent<AudioListener>();
            var data = cam.GetUniversalAdditionalCameraData();
            data.renderPostProcessing = false;
            data.antialiasing = AntialiasingMode.None;
            data.renderShadows = true;
            var follow = go.AddComponent<CameraFollow>();
            follow.SnapTo(Vector3.zero);
        }

        private static void BuildLight()
        {
            var go = new GameObject("Directional Light");
            var light = go.AddComponent<Light>();
            light.type = LightType.Directional;
            light.color = new Color(1f, 0.96f, 0.9f);
            light.intensity = 1.3f;
            light.shadows = LightShadows.Soft;
            light.shadowStrength = 0.7f;
            go.transform.rotation = Quaternion.Euler(55f, -35f, 0f);
        }

        private static void BuildArena(GameAssets assets, Material material)
        {
            var world = new GameObject("Arena");

            var floorAsset = AssetDatabase.LoadAssetAtPath<GameObject>(BootstrapAssets.ModelPath("arena_floor"));
            if (floorAsset != null)
            {
                var floor = (GameObject)PrefabUtility.InstantiatePrefab(floorAsset);
                floor.name = "Floor";
                floor.transform.SetParent(world.transform, false);
                floor.isStatic = true;
                BootstrapPrefabs.ApplyMaterial(floor, material);
            }

            float r = assets.arenaRadius;
            var props = new GameObject("Props");
            props.transform.SetParent(world.transform, false);

            if (assets.pillarPrefab != null)
            {
                const int pillars = 16;
                for (int i = 0; i < pillars; i++)
                {
                    float a = i * Mathf.PI * 2f / pillars;
                    var p = (GameObject)PrefabUtility.InstantiatePrefab(assets.pillarPrefab);
                    p.transform.SetParent(props.transform, false);
                    p.transform.position = new Vector3(Mathf.Cos(a) * (r - 2.5f), 0f, Mathf.Sin(a) * (r - 2.5f));
                    p.transform.rotation = Quaternion.Euler(0f, -a * Mathf.Rad2Deg, 0f);
                }
            }

            if (assets.cratePrefab != null)
            {
                var rng = new System.Random(42);
                for (int i = 0; i < 14; i++)
                {
                    float a = (float)(rng.NextDouble() * Mathf.PI * 2f);
                    float d = 5f + (float)rng.NextDouble() * 9f;
                    var c = (GameObject)PrefabUtility.InstantiatePrefab(assets.cratePrefab);
                    c.transform.SetParent(props.transform, false);
                    c.transform.position = new Vector3(Mathf.Cos(a) * d, 0f, Mathf.Sin(a) * d);
                    c.transform.rotation = Quaternion.Euler(0f, (float)(rng.NextDouble() * 90f), 0f);
                }
            }
        }

        private static void BuildSystems(GameAssets assets)
        {
            var game = new GameObject("Game");
            var gm = game.AddComponent<GameManager>();
            var so = new SerializedObject(gm);
            so.FindProperty("assets").objectReferenceValue = assets;
            so.ApplyModifiedPropertiesWithoutUndo();

            game.AddComponent<PoolService>();
            game.AddComponent<WaveSpawner>();
            game.AddComponent<UpgradeSystem>();
            game.AddComponent<GameInput>();

            var audio = new GameObject("Audio");
            audio.transform.SetParent(game.transform, false);
            var source = audio.AddComponent<AudioSource>();
            source.playOnAwake = false;
            audio.AddComponent<AudioManager>();
        }

        private static void BuildEventSystem()
        {
            var go = new GameObject("EventSystem");
            go.AddComponent<EventSystem>();
            var module = go.AddComponent<InputSystemUIInputModule>();

            // Point the module at the package's default UI actions (a persistent asset, so the
            // reference survives saving the scene). Assigning the asset maps the "UI" actions by name.
            var actions = AssetDatabase.LoadAssetAtPath<InputActionAsset>(DefaultUiActions);
            if (actions != null)
            {
                module.actionsAsset = actions;
                var refs = AssetDatabase.LoadAllAssetsAtPath(DefaultUiActions);
                module.point = FindRef(refs, "UI/Point") ?? module.point;
                module.leftClick = FindRef(refs, "UI/Click") ?? module.leftClick;
                module.middleClick = FindRef(refs, "UI/MiddleClick") ?? module.middleClick;
                module.rightClick = FindRef(refs, "UI/RightClick") ?? module.rightClick;
                module.scrollWheel = FindRef(refs, "UI/ScrollWheel") ?? module.scrollWheel;
                module.move = FindRef(refs, "UI/Navigate") ?? module.move;
                module.submit = FindRef(refs, "UI/Submit") ?? module.submit;
                module.cancel = FindRef(refs, "UI/Cancel") ?? module.cancel;
            }
            else
            {
                Debug.LogWarning("[Arena] DefaultInputActions.inputactions not found in the Input System package; UI will use the module's built-in defaults.");
            }
        }

        private static InputActionReference FindRef(Object[] subAssets, string actionPath)
        {
            foreach (var o in subAssets)
            {
                if (o is InputActionReference r && r.action != null)
                {
                    var path = r.action.actionMap != null ? r.action.actionMap.name + "/" + r.action.name : r.action.name;
                    if (path == actionPath) return r;
                }
            }
            return null;
        }

        private static void BuildUi()
        {
            var canvasGo = new GameObject("UI", typeof(RectTransform));
            canvasGo.layer = LayerMask.NameToLayer("UI");
            var canvas = canvasGo.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 10;
            var scaler = canvasGo.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920f, 1080f);
            scaler.screenMatchMode = CanvasScaler.ScreenMatchMode.MatchWidthOrHeight;
            scaler.matchWidthOrHeight = 0.5f;
            canvasGo.AddComponent<GraphicRaycaster>();

            // Later siblings draw on top.
            AddPanel<HUD>(canvasGo.transform, "HUD");
            AddPanel<UpgradeUI>(canvasGo.transform, "UpgradeUI");
            AddPanel<PauseUI>(canvasGo.transform, "PauseUI");
            AddPanel<GameOverUI>(canvasGo.transform, "GameOverUI");
            AddPanel<MainMenuUI>(canvasGo.transform, "MainMenuUI");
        }

        private static T AddPanel<T>(Transform parent, string name) where T : UIPanel
        {
            var go = new GameObject(name, typeof(RectTransform));
            go.layer = LayerMask.NameToLayer("UI");
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            UIFactory.Stretch(rt);
            var panel = go.AddComponent<T>();
            panel.Build();
            return panel;
        }
    }
}
