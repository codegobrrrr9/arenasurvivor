using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Single registry of every asset the runtime needs. Created and filled by the
    /// editor bootstrap (Arena > Build Project); the scene only references this one asset.
    /// </summary>
    [CreateAssetMenu(menuName = "Arena/Game Assets")]
    public class GameAssets : ScriptableObject
    {
        [Header("Prefabs")]
        public GameObject playerPrefab;
        public GameObject projectilePrefab;
        public GameObject xpPickupPrefab;
        public GameObject healthPickupPrefab;
        public GameObject pillarPrefab;
        public GameObject cratePrefab;

        [Header("Definitions")]
        public EnemyDefinition[] enemies;
        public UpgradeDefinition[] upgrades;

        [Header("Audio")]
        public AudioClip shoot;
        public AudioClip hit;
        public AudioClip enemyDie;
        public AudioClip pickup;
        public AudioClip upgrade;
        public AudioClip playerHurt;
        public AudioClip gameOver;
        public AudioClip uiClick;
        public AudioClip waveStart;

        [Header("World")]
        public Material paletteMaterial;
        public float arenaRadius = 20f;

        public EnemyDefinition FindEnemy(string id)
        {
            foreach (var e in enemies)
            {
                if (e != null && e.id == id) return e;
            }
            return null;
        }
    }
}
