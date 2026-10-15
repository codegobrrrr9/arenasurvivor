using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Collider-free projectile: each frame it raycasts the distance it is about to
    /// travel, so fast shots never tunnel through thin enemies.
    /// </summary>
    public class Projectile : MonoBehaviour, IPoolable
    {
        public float lifetime = 2.5f;

        private Vector3 _direction;
        private float _damage;
        private float _speed;
        private float _dieAt;
        private int _hitMask;
        private float _arenaRadius = 20f;

        public void Launch(Vector3 direction, float damage, float speed)
        {
            _direction = direction.normalized;
            _damage = damage;
            _speed = speed;
            _dieAt = Time.time + lifetime;
            transform.rotation = Quaternion.LookRotation(_direction, Vector3.up);

            int enemy = LayerMask.NameToLayer("Enemy");
            int obstacle = LayerMask.NameToLayer("Obstacle");
            _hitMask = 0;
            if (enemy >= 0) _hitMask |= 1 << enemy;
            if (obstacle >= 0) _hitMask |= 1 << obstacle;
            if (_hitMask == 0) _hitMask = Physics.DefaultRaycastLayers;

            var gm = GameManager.Instance;
            if (gm != null && gm.Assets != null) _arenaRadius = gm.Assets.arenaRadius + 2f;
        }

        private void Update()
        {
            var gm = GameManager.Instance;
            if (gm == null) return;

            float step = _speed * Time.deltaTime;
            if (Physics.Raycast(transform.position, _direction, out var hit, step, _hitMask, QueryTriggerInteraction.Collide))
            {
                var target = hit.collider.GetComponentInParent<IDamageable>();
                if (target != null)
                {
                    target.Damage(_damage, _direction);
                    gm.Audio?.Play(gm.Assets.hit, 0.4f);
                }
                gm.Pools?.Release(gameObject);
                return;
            }

            transform.position += _direction * step;
            var p = transform.position;
            if (Time.time >= _dieAt || new Vector2(p.x, p.z).magnitude > _arenaRadius)
            {
                gm.Pools?.Release(gameObject);
            }
        }

        public void OnSpawned() { }
        public void OnDespawned() { }
    }
}
