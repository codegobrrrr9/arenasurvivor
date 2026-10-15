using UnityEngine;

namespace Arena
{
    public enum EnemyBehaviour
    {
        Chaser,   // runs straight at the player
        Dasher,   // circles, then lunges
        Tank      // slow, heavy, hard to push
    }

    [CreateAssetMenu(menuName = "Arena/Enemy Definition")]
    public class EnemyDefinition : ScriptableObject
    {
        public string id = "blob";
        public string displayName = "Blob";
        public EnemyBehaviour behaviour = EnemyBehaviour.Chaser;
        public GameObject prefab;

        [Header("Stats")]
        public float hp = 30f;
        public float moveSpeed = 3.5f;
        public float contactDamage = 10f;
        public float contactCooldown = 0.8f;
        public float mass = 1f;
        public float knockbackResistance = 0f;   // 0..1

        [Header("Rewards")]
        public int xpValue = 3;
        public int scoreValue = 10;
        [Range(0f, 1f)] public float dropHealthChance = 0.05f;

        [Header("Spawning")]
        public int firstWave = 1;
    }
}
