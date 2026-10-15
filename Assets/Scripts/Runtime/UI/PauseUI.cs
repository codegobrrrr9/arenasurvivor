using UnityEngine;
using UnityEngine.UI;

namespace Arena
{
    public class PauseUI : UIPanel
    {
        [SerializeField] private Button resumeButton;
        [SerializeField] private Button restartButton;
        [SerializeField] private Button menuButton;

        public override void Build()
        {
            visibleIn = new[] { GameState.Paused };
            var panel = UIFactory.CreatePanel(transform, "Panel", UIFactory.PanelColor);
            var t = panel.transform;
            UIFactory.CreateText(t, "Title", "PAUSED", 72, new Vector2(0.5f, 0.5f), new Vector2(0f, 150f), new Vector2(800f, 90f), TextAnchor.MiddleCenter, FontStyle.Bold);
            resumeButton = UIFactory.CreateButton(t, "ResumeButton", "RESUME", new Vector2(0.5f, 0.5f), new Vector2(0f, 20f), new Vector2(320f, 70f));
            restartButton = UIFactory.CreateButton(t, "RestartButton", "RESTART", new Vector2(0.5f, 0.5f), new Vector2(0f, -70f), new Vector2(320f, 70f));
            menuButton = UIFactory.CreateButton(t, "MenuButton", "MAIN MENU", new Vector2(0.5f, 0.5f), new Vector2(0f, -160f), new Vector2(320f, 70f));
            built = true;
        }

        protected override void Awake()
        {
            base.Awake();
            resumeButton?.onClick.AddListener(() => { Click(); Gm?.Resume(); });
            restartButton?.onClick.AddListener(() => { Click(); Gm?.StartRun(); });
            menuButton?.onClick.AddListener(() => { Click(); Gm?.ReturnToMenu(); });
        }
    }
}
