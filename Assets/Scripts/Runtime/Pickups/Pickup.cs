using UnityEngine;

namespace Arena
{
    public enum PickupType { Xp, Health }

    /// <summary>Bobbing, spinning drop that flies to the player once they are close.</summary>
    public class Pickup : MonoBehaviour, IPoolable
    {
        public PickupType type = PickupType.Xp;
        public float magnetRadius = 3.5f;
        public float magnetSpeed = 12f;
        public float lifetime = 20f;

        private int _value = 1;
        private float _spawnTime;
        private float _bobOffset;
        private bool _collected;

        public void Init(PickupType t, int value)
        {
            type = t;
            _value = value;
        }

        public void OnSpawned()
        {
            _spawnTime = Time.time;
            _bobOffset = Random.Range(0f, 10f);
            _collected = false;
        }

        public void OnDespawned() { }

        private void Update()
        {
            var gm = GameManager.Instance;
            if (gm == null || _collected) return;
            if (Time.time - _spawnTime > lifetime) { gm.Pools?.Release(gameObject); return; }

            var p = transform.position;
            p.y = 0.5f + Mathf.Sin((Time.time + _bobOffset) * 3f) * 0.15f;
            transform.position = p;
            transform.Rotate(0f, 120f * Time.deltaTime, 0f, Space.World);

            if (gm.Player == null || gm.State != GameState.Playing) return;
            var to = gm.Player.transform.position + Vector3.up * 0.8f - transform.position;
            float d = to.magnitude;
            if (d < magnetRadius)
            {
                transform.position += to.normalized * magnetSpeed * Time.deltaTime * Mathf.Lerp(2f, 1f, d / magnetRadius);
                if (d < 0.6f) Collect(gm);
            }
        }

        private void OnTriggerEnter(Collider other)
        {
            if (_collected) return;
            var gm = GameManager.Instance;
            if (gm == null || other.GetComponentInParent<PlayerController>() == null) return;
            Collect(gm);
        }

        private void Collect(GameManager gm)
        {
            _collected = true;
            gm.OnPickupCollected(type, _value);
            gm.Pools?.Release(gameObject);
        }
    }
}
