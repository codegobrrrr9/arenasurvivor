using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Arena
{
    public class UpgradeUI : UIPanel
    {
        [SerializeField] private Button[] cards = new Button[3];
        [SerializeField] private Text[] cardTitles = new Text[3];
        [SerializeField] private Text[] cardDescriptions = new Text[3];
        [SerializeField] private Text header;

        private readonly List<UpgradeDefinition> _offer = new();

        public override void Build()
        {
            visibleIn = new[] { GameState.Upgrading };
            var panel = UIFactory.CreatePanel(transform, "Panel", UIFactory.PanelColor);
            var t = panel.transform;
            header = UIFactory.CreateText(t, "Header", "CHOOSE AN UPGRADE", 64, new Vector2(0.5f, 0.5f), new Vector2(0f, 230f), new Vector2(1200f, 90f), TextAnchor.MiddleCenter, FontStyle.Bold);

            cards = new Button[3];
            cardTitles = new Text[3];
            cardDescriptions = new Text[3];
            float spacing = 420f;
            for (int i = 0; i < 3; i++)
            {
                float x = (i - 1) * spacing;
                var card = UIFactory.CreateButton(t, $"Card{i}", "", new Vector2(0.5f, 0.5f), new Vector2(x, -20f), new Vector2(380f, 300f));
                var label = card.transform.Find("Label");
                if (label != null) label.gameObject.SetActive(false);
                cardTitles[i] = UIFactory.CreateText(card.transform, "Title", "Upgrade", 36, new Vector2(0.5f, 1f), new Vector2(0f, -30f), new Vector2(340f, 60f), TextAnchor.MiddleCenter, FontStyle.Bold);
                cardDescriptions[i] = UIFactory.CreateText(card.transform, "Description", "", 24, new Vector2(0.5f, 0.5f), new Vector2(0f, -30f), new Vector2(330f, 160f), TextAnchor.MiddleCenter);
                cards[i] = card;
            }
            var hint = UIFactory.CreateText(t, "Hint", "Press 1, 2 or 3", 24, new Vector2(0.5f, 0.5f), new Vector2(0f, -230f), new Vector2(600f, 30f));
            hint.color = UIFactory.Muted;
            built = true;
        }

        protected override void Awake()
        {
            base.Awake();
            for (int i = 0; i < cards.Length; i++)
            {
                int index = i;
                cards[i]?.onClick.AddListener(() => Pick(index));
            }
        }

        protected override void OnShown()
        {
            if (Gm == null || Gm.Upgrades == null) return;
            _offer.Clear();
            _offer.AddRange(Gm.Upgrades.CurrentOffer);
            if (_offer.Count == 0) _offer.AddRange(Gm.Upgrades.Roll(3));

            for (int i = 0; i < cards.Length; i++)
            {
                bool has = i < _offer.Count;
                if (cards[i] != null) cards[i].gameObject.SetActive(has);
                if (!has) continue;
                var u = _offer[i];
                int stacks = Gm.Upgrades.StacksOf(u);
                if (cardTitles[i] != null) cardTitles[i].text = u.title;
                if (cardDescriptions[i] != null) cardDescriptions[i].text = $"{u.description}\n\n{stacks}/{u.maxStacks} taken";
            }
            if (header != null) header.text = Gm.Spawner != null && Gm.Spawner.WaveInProgress ? "LEVEL UP!" : $"WAVE {Gm.Wave} CLEARED";
        }

        private void Update()
        {
            if (Gm == null || Gm.State != GameState.Upgrading) return;
            var kb = UnityEngine.InputSystem.Keyboard.current;
            if (kb == null) return;
            if (kb.digit1Key.wasPressedThisFrame) Pick(0);
            else if (kb.digit2Key.wasPressedThisFrame) Pick(1);
            else if (kb.digit3Key.wasPressedThisFrame) Pick(2);
        }

        private void Pick(int index)
        {
            if (Gm == null || index >= _offer.Count) return;
            Gm.Upgrades.Choose(_offer[index]);
        }
    }
}
