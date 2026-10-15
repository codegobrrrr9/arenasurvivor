using UnityEngine;

namespace Arena
{
    /// <summary>Runtime stat block. Upgrades mutate it; ResetToBase restores defaults.</summary>
    public class PlayerStats : MonoBehaviour
    {
        [Header("Base values")]
        public float baseMoveSpeed = 6.5f;
        public float baseFireRate = 4f;        // shots per second
        public float baseDamage = 10f;
        public float baseMaxHp = 100f;
        public int baseProjectileCount = 1;
        public float baseProjectileSpeed = 24f;

        [Header("Current values (runtime)")]
        public float moveSpeed;
        public float fireRate;
        public float damage;
        public float maxHp;
        public int projectileCount;
        public float projectileSpeed;

        private void Awake()
        {
            ResetToBase();
        }

        public void ResetToBase()
        {
            moveSpeed = baseMoveSpeed;
            fireRate = baseFireRate;
            damage = baseDamage;
            maxHp = baseMaxHp;
            projectileCount = baseProjectileCount;
            projectileSpeed = baseProjectileSpeed;
        }

        public void ApplyUpgrade(UpgradeDefinition def)
        {
            switch (def.stat)
            {
                case StatType.MoveSpeed: moveSpeed = Apply(moveSpeed, def); break;
                case StatType.FireRate: fireRate = Apply(fireRate, def); break;
                case StatType.Damage: damage = Apply(damage, def); break;
                case StatType.MaxHp: maxHp = Apply(maxHp, def); break;
                case StatType.ProjectileCount: projectileCount = Mathf.RoundToInt(Apply(projectileCount, def)); break;
                case StatType.ProjectileSpeed: projectileSpeed = Apply(projectileSpeed, def); break;
            }
        }

        private static float Apply(float current, UpgradeDefinition def)
        {
            return def.isMultiplier ? current * def.value : current + def.value;
        }
    }
}
