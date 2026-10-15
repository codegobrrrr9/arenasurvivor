using System.Collections.Generic;
using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Formula-driven waves: more enemies, more HP and faster spawning as waves go up.
    /// Dashers join from wave 3, tanks from wave 5, a boss tank every 5th wave.
    /// </summary>
    public class WaveSpawner : MonoBehaviour
    {
        public float spawnRadiusPadding = 1.5f;

        public bool WaveInProgress { get; private set; }
        public int Alive { get; private set; }
        public int Remaining => _queue.Count;

        private readonly Queue<(EnemyDefinition def, float scale, float hpMul)> _queue = new();
        private float _spawnInterval = 1f;
        private float _nextSpawn;
        private int _wave;

        public void StartWave(int wave)
        {
            var gm = GameManager.Instance;
            if (gm == null || gm.Assets == null) return;
            _wave = wave;
            _queue.Clear();

            var blob = gm.Assets.FindEnemy("blob");
            var spiky = gm.Assets.FindEnemy("spiky");
            var tank = gm.Assets.FindEnemy("tank");

            float hpMul = 1f + 0.12f * (wave - 1);
            int blobs = 6 + wave * 3;
            int dashers = wave >= 3 ? 2 + wave : 0;
            int tanks = wave >= 5 ? 1 + (wave - 5) / 2 : 0;

            var list = new List<(EnemyDefinition, float, float)>();
            if (blob != null) for (int i = 0; i < blobs; i++) list.Add((blob, 1f, hpMul));
            if (spiky != null) for (int i = 0; i < dashers; i++) list.Add((spiky, 1f, hpMul));
            if (tank != null) for (int i = 0; i < tanks; i++) list.Add((tank, 1f, hpMul));
            Shuffle(list);
            if (tank != null && wave % 5 == 0) list.Add((tank, 1.8f, hpMul * 4f));   // boss last

            foreach (var e in list) _queue.Enqueue(e);
            _spawnInterval = Mathf.Max(0.25f, 1.2f - 0.05f * wave);
            _nextSpawn = Time.time + 0.5f;
            WaveInProgress = true;
        }

        public void StopWaves()
        {
            _queue.Clear();
            WaveInProgress = false;
            Alive = 0;
        }

        public void OnEnemyRemoved()
        {
            Alive = Mathf.Max(0, Alive - 1);
            CheckCleared();
        }

        private void Update()
        {
            var gm = GameManager.Instance;
            if (!WaveInProgress || gm == null || gm.State != GameState.Playing) return;
            if (_queue.Count == 0) { CheckCleared(); return; }
            if (Time.time < _nextSpawn) return;

            _nextSpawn = Time.time + _spawnInterval;
            var (def, scale, hpMul) = _queue.Dequeue();
            Spawn(gm, def, scale, hpMul);
        }

        private void Spawn(GameManager gm, EnemyDefinition def, float scale, float hpMul)
        {
            if (def.prefab == null || gm.Pools == null) return;
            float angle = Random.Range(0f, Mathf.PI * 2f);
            float r = gm.Assets.arenaRadius - spawnRadiusPadding;
            var pos = new Vector3(Mathf.Cos(angle) * r, 0f, Mathf.Sin(angle) * r);
            var go = gm.Pools.Get(def.prefab, pos, Quaternion.LookRotation(-pos.normalized, Vector3.up));
            go.GetComponent<Enemy>()?.Init(def, hpMul, scale);
            Alive++;
        }

        private void CheckCleared()
        {
            if (!WaveInProgress || _queue.Count > 0 || Alive > 0) return;
            WaveInProgress = false;
            GameManager.Instance?.OnWaveCleared();
        }

        private static void Shuffle<T>(IList<T> list)
        {
            for (int i = list.Count - 1; i > 0; i--)
            {
                int j = Random.Range(0, i + 1);
                (list[i], list[j]) = (list[j], list[i]);
            }
        }
    }
}
