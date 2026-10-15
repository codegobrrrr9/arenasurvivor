using UnityEngine;
using UnityEngine.UI;

namespace Arena
{
    public class MainMenuUI : UIPanel
    {
        [SerializeField] private Button playButton;
        [SerializeField] private Button quitButton;
        [SerializeField] private Text highScoreText;

        public override void Build()
        {
            visibleIn = new[] { GameState.Menu };
            var panel = UIFactory.CreatePanel(transform, "Panel", new Color(0.03f, 0.04f, 0.07f, 0.75f));
            var t = panel.transform;

            UIFactory.CreateText(t, "Title", "ARENA SURVIVOR", 96, new Vector2(0.5f, 0.5f), new Vector2(0f, 190f), new Vector2(1200f, 120f), TextAnchor.MiddleCenter, FontStyle.Bold);
            var sub = UIFactory.CreateText(t, "Subtitle", "Survive the waves. Pick upgrades. Beat your high score.", 30, new Vector2(0.5f, 0.5f), new Vector2(0f, 110f), new Vector2(1200f, 40f));
            sub.color = UIFactory.Muted;

            playButton = UIFactory.CreateButton(t, "PlayButton", "PLAY", new Vector2(0.5f, 0.5f), new Vector2(0f, 0f), new Vector2(320f, 80f), 38);
            quitButton = UIFactory.CreateButton(t, "QuitButton", "QUIT", new Vector2(0.5f, 0.5f), new Vector2(0f, -100f), new Vector2(320f, 64f), 28);

            highScoreText = UIFactory.CreateText(t, "HighScore", "High score: 0", 28, new Vector2(0.5f, 0.5f), new Vector2(0f, -190f), new Vector2(600f, 40f));
            var controls = UIFactory.CreateText(t, "Controls",
                "WASD / Left stick: move     Mouse / Right stick: aim     Click / RT / Space: fire     Esc / Start: pause",
                22, new Vector2(0.5f, 0f), new Vector2(0f, 40f), new Vector2(1600f, 30f));
            controls.color = UIFactory.Muted;
            built = true;
        }

        protected override void Awake()
        {
            base.Awake();
            if (playButton != null) playButton.onClick.AddListener(OnPlay);
            if (quitButton != null) quitButton.onClick.AddListener(OnQuit);
#if UNITY_WEBGL
            if (quitButton != null) quitButton.gameObject.SetActive(false);
#endif
        }

        protected override void OnShown()
        {
            if (highScoreText != null && Gm != null) highScoreText.text = $"High score: {Gm.HighScore}";
#if UNITY_WEBGL
            if (quitButton != null) quitButton.gameObject.SetActive(false);
#endif
        }

        private void OnPlay()
        {
            Click();
            Gm?.StartRun();
        }

        private void OnQuit()
        {
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }
    }
}
