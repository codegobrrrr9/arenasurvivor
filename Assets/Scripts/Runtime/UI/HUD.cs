using UnityEngine;
using UnityEngine.UI;

namespace Arena
{
    public class HUD : UIPanel
    {
        [SerializeField] private Image hpFill;
        [SerializeField] private Image xpFill;
        [SerializeField] private Text hpText;
        [SerializeField] private Text waveText;
        [SerializeField] private Text scoreText;
        [SerializeField] private Text enemiesText;
        [SerializeField] private Text levelText;

        public override void Build()
        {
            visibleIn = new[] { GameState.Playing, GameState.Paused, GameState.Upgrading };
            var root = UIFactory.CreateRect(transform, "Root");
            UIFactory.Stretch(root);

            var (hpBack, hpF) = UIFactory.CreateBar(root, "HpBar", new Vector2(0f, 1f), new Vector2(30f, -30f), new Vector2(420f, 34f), new Color(0.85f, 0.2f, 0.25f));
            hpFill = hpF;
            hpText = UIFactory.CreateText(hpBack.transform, "HpText", "100 / 100", 22, new Vector2(0.5f, 0.5f), Vector2.zero, new Vector2(420f, 34f), TextAnchor.MiddleCenter, FontStyle.Bold);

            var (xpBack, xpF) = UIFactory.CreateBar(root, "XpBar", new Vector2(0f, 1f), new Vector2(30f, -72f), new Vector2(420f, 18f), new Color(0.3f, 0.85f, 0.95f));
            xpFill = xpF;
            levelText = UIFactory.CreateText(root, "LevelText", "Lv 1", 20, new Vector2(0f, 1f), new Vector2(460f, -72f), new Vector2(120f, 24f), TextAnchor.MiddleLeft);

            waveText = UIFactory.CreateText(root, "WaveText", "Wave 1", 40, new Vector2(0.5f, 1f), new Vector2(0f, -28f), new Vector2(400f, 50f), TextAnchor.MiddleCenter, FontStyle.Bold);
            enemiesText = UIFactory.CreateText(root, "EnemiesText", "", 22, new Vector2(0.5f, 1f), new Vector2(0f, -74f), new Vector2(400f, 30f));
            enemiesText.color = UIFactory.Muted;
            scoreText = UIFactory.CreateText(root, "ScoreText", "Score 0", 34, new Vector2(1f, 1f), new Vector2(-30f, -30f), new Vector2(420f, 44f), TextAnchor.MiddleRight, FontStyle.Bold);

            built = true;
        }

        protected override void OnEnable()
        {
            base.OnEnable();
            if (Gm == null) return;
            Gm.StatsChanged += Refresh;
            HookPlayer();
            Refresh();
        }

        protected override void OnDisable()
        {
            base.OnDisable();
            if (Gm == null) return;
            Gm.StatsChanged -= Refresh;
            if (Gm.Player != null) Gm.Player.Health.Changed -= OnHealthChanged;
        }

        protected override void Start()
        {
            base.Start();
            HookPlayer();
            Refresh();
        }

        private void HookPlayer()
        {
            if (Gm == null || Gm.Player == null) return;
            Gm.Player.Health.Changed -= OnHealthChanged;
            Gm.Player.Health.Changed += OnHealthChanged;
            OnHealthChanged(Gm.Player.Health.Current, Gm.Player.Health.Max);
        }

        private void Update()
        {
            if (Gm == null || Gm.Spawner == null || enemiesText == null) return;
            int left = Gm.Spawner.Alive + Gm.Spawner.Remaining;
            enemiesText.text = left > 0 ? $"{left} enemies left" : "";
        }

        private void OnHealthChanged(float current, float max)
        {
            UIFactory.SetBarFraction(hpFill, max > 0f ? current / max : 0f);
            if (hpText != null) hpText.text = $"{Mathf.CeilToInt(current)} / {Mathf.CeilToInt(max)}";
        }

        private void Refresh()
        {
            if (Gm == null) return;
            if (waveText != null) waveText.text = $"Wave {Mathf.Max(1, Gm.Wave)}";
            if (scoreText != null) scoreText.text = $"Score {Gm.Score}";
            if (levelText != null) levelText.text = $"Lv {Gm.Level + 1}";
            UIFactory.SetBarFraction(xpFill, Gm.XpToNext > 0 ? (float)Gm.Xp / Gm.XpToNext : 0f);
            if (Gm.Player != null) OnHealthChanged(Gm.Player.Health.Current, Gm.Player.Health.Max);
        }
    }
}
