using UnityEngine;
using UnityEngine.UI;

namespace Arena
{
    public class GameOverUI : UIPanel
    {
        [SerializeField] private Text summaryText;
        [SerializeField] private Button restartButton;
        [SerializeField] private Button menuButton;

        public override void Build()
        {
            visibleIn = new[] { GameState.GameOver };
            var panel = UIFactory.CreatePanel(transform, "Panel", new Color(0.12f, 0.02f, 0.03f, 0.88f));
            var t = panel.transform;
            UIFactory.CreateText(t, "Title", "YOU DIED", 84, new Vector2(0.5f, 0.5f), new Vector2(0f, 170f), new Vector2(800f, 100f), TextAnchor.MiddleCenter, FontStyle.Bold);
            summaryText = UIFactory.CreateText(t, "Summary", "", 32, new Vector2(0.5f, 0.5f), new Vector2(0f, 70f), new Vector2(900f, 80f));
            restartButton = UIFactory.CreateButton(t, "RestartButton", "TRY AGAIN", new Vector2(0.5f, 0.5f), new Vector2(0f, -40f), new Vector2(320f, 70f));
            menuButton = UIFactory.CreateButton(t, "MenuButton", "MAIN MENU", new Vector2(0.5f, 0.5f), new Vector2(0f, -130f), new Vector2(320f, 70f));
            built = true;
        }

        protected override void Awake()
        {
            base.Awake();
            restartButton?.onClick.AddListener(() => { Click(); Gm?.StartRun(); });
            menuButton?.onClick.AddListener(() => { Click(); Gm?.ReturnToMenu(); });
        }

        protected override void OnShown()
        {
            if (summaryText == null || Gm == null) return;
            string best = Gm.Score >= Gm.HighScore && Gm.Score > 0 ? "  NEW HIGH SCORE!" : $"  (best {Gm.HighScore})";
            summaryText.text = $"Reached wave {Gm.Wave} with a score of {Gm.Score}{best}";
        }
    }
}
