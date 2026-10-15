using UnityEngine;

namespace Arena
{
    public enum StatType { MoveSpeed, FireRate, Damage, MaxHp, ProjectileCount, ProjectileSpeed }

    [CreateAssetMenu(menuName = "Arena/Upgrade Definition")]
    public class UpgradeDefinition : ScriptableObject
    {
        public string title = "Upgrade";
        [TextArea] public string description = "";
        public StatType stat;
        public float value = 1.1f;
        public bool isMultiplier = true;
        public int maxStacks = 8;
        public bool healOnPick = false;
    }
}
