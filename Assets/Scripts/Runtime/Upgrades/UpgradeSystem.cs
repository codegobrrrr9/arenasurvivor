using System.Collections.Generic;
using UnityEngine;

namespace Arena
{
    public class UpgradeSystem : MonoBehaviour
    {
        private readonly Dictionary<UpgradeDefinition, int> _stacks = new();
        private List<UpgradeDefinition> _currentOffer = new();

        public IReadOnlyList<UpgradeDefinition> CurrentOffer => _currentOffer;

        public void ResetRun()
        {
            _stacks.Clear();
            _currentOffer.Clear();
        }

        public int StacksOf(UpgradeDefinition def) => _stacks.TryGetValue(def, out var n) ? n : 0;

        /// <summary>Picks up to <paramref name="count"/> distinct upgrades that still have stacks left.</summary>
        public List<UpgradeDefinition> Roll(int count)
        {
            var gm = GameManager.Instance;
            var pool = new List<UpgradeDefinition>();
            if (gm != null && gm.Assets != null && gm.Assets.upgrades != null)
            {
                foreach (var u in gm.Assets.upgrades)
                {
                    if (u != null && StacksOf(u) < u.maxStacks) pool.Add(u);
                }
            }
            for (int i = pool.Count - 1; i > 0; i--)
            {
                int j = Random.Range(0, i + 1);
                (pool[i], pool[j]) = (pool[j], pool[i]);
            }
            if (pool.Count > count) pool.RemoveRange(count, pool.Count - count);
            _currentOffer = pool;
            return pool;
        }

        public void Choose(UpgradeDefinition def)
        {
            var gm = GameManager.Instance;
            if (gm == null || def == null || gm.Player == null) return;

            var stats = gm.Player.Stats;
            float oldMax = stats.maxHp;
            stats.ApplyUpgrade(def);
            _stacks[def] = StacksOf(def) + 1;

            if (def.stat == StatType.MaxHp)
            {
                gm.Player.Health.Configure(stats.maxHp, refill: false);
                gm.Player.Health.Heal(stats.maxHp - oldMax);
            }
            if (def.healOnPick) gm.Player.Health.Heal(stats.maxHp * 0.25f);

            gm.Audio?.Play(gm.Assets.uiClick, 0.6f);
            gm.OnUpgradeChosen();
        }
    }
}
