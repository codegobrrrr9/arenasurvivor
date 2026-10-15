using UnityEngine;

namespace Arena
{
    /// <summary>
    /// One enemy class, three behaviours. Dynamic rigidbody so enemies push each
    /// other and collide with props; velocity is set directly every physics step.
    /// </summary>
    [RequireComponent(typeof(Rigidbody), typeof(Health))]
    public class Enemy : MonoBehaviour, IPoolable
    {
        public EnemyDefinition definition;

        private Rigidbody _rb;
        private Health _health;
        private Transform _player;
        private float _nextContactTime;
        private float _dashTimer;
        private bool _dashing;
        private Vector3 _dashDir;
        private int _orbitSign = 1;
        private float _hpMultiplier = 1f;
        private float _scale = 1f;
        private bool _dead;
        private Vector3 _knockback;
        private float _arenaRadius = 20f;

        private void Awake()
        {
            _rb = GetComponent<Rigidbody>();
            _health = GetComponent<Health>();
            _rb.useGravity = false;
            _rb.constraints = RigidbodyConstraints.FreezeRotation | RigidbodyConstraints.FreezePositionY;
            _rb.interpolation = RigidbodyInterpolation.Interpolate;
            _rb.collisionDetectionMode = CollisionDetectionMode.Discrete;
            _health.Died += OnDied;
            _health.Damaged += OnDamaged;
        }

        public void Init(EnemyDefinition def, float hpMultiplier, float scale = 1f)
        {
            definition = def;
            _hpMultiplier = hpMultiplier;
            _scale = scale;
            ApplyDefinition();
        }

        private void ApplyDefinition()
        {
            if (definition == null) return;
            _rb.mass = definition.mass;
            _rb.linearDamping = 1.5f;
            transform.localScale = Vector3.one * _scale;
            _health.Configure(definition.hp * _hpMultiplier);
            _dead = false;
            _dashing = false;
            _dashTimer = Random.Range(0.5f, 1.5f);
            _orbitSign = Random.value < 0.5f ? -1 : 1;
            _knockback = Vector3.zero;
            _nextContactTime = Time.time + 0.3f;
            var gm = GameManager.Instance;
            if (gm != null && gm.Assets != null) _arenaRadius = gm.Assets.arenaRadius;
        }

        public void OnSpawned()
        {
            var gm = GameManager.Instance;
            _player = gm != null && gm.Player != null ? gm.Player.transform : null;
            if (definition != null) ApplyDefinition();
        }

        public void OnDespawned()
        {
            _rb.linearVelocity = Vector3.zero;
            transform.localScale = Vector3.one;
        }

        private void FixedUpdate()
        {
            if (_dead || _player == null || definition == null) return;
            var gm = GameManager.Instance;
            if (gm == null || gm.State != GameState.Playing)
            {
                _rb.linearVelocity = Vector3.zero;
                return;
            }

            var toPlayer = _player.position - transform.position;
            toPlayer.y = 0f;
            float dist = toPlayer.magnitude;
            var dir = dist > 0.001f ? toPlayer / dist : Vector3.forward;
            Vector3 desired;

            switch (definition.behaviour)
            {
                case EnemyBehaviour.Dasher:
                    desired = DasherVelocity(dir, dist);
                    break;
                case EnemyBehaviour.Tank:
                    desired = dir * definition.moveSpeed;
                    break;
                default:
                    desired = dir * definition.moveSpeed;
                    break;
            }

            _knockback = Vector3.MoveTowards(_knockback, Vector3.zero, 30f * Time.fixedDeltaTime);
            _rb.linearVelocity = desired + _knockback;

            if (desired.sqrMagnitude > 0.01f)
            {
                _rb.MoveRotation(Quaternion.LookRotation(desired.normalized, Vector3.up));
            }

            var p = _rb.position;
            var flat = new Vector3(p.x, 0f, p.z);
            if (flat.magnitude > _arenaRadius + 1f)
            {
                flat = flat.normalized * (_arenaRadius + 1f);
                _rb.position = new Vector3(flat.x, 0f, flat.z);
            }
        }

        private Vector3 DasherVelocity(Vector3 dir, float dist)
        {
            _dashTimer -= Time.fixedDeltaTime;
            if (_dashing)
            {
                if (_dashTimer <= 0f)
                {
                    _dashing = false;
                    _dashTimer = Random.Range(1.2f, 2.0f);
                }
                return _dashDir * definition.moveSpeed * 3.2f;
            }

            if (_dashTimer <= 0f && dist < 9f)
            {
                _dashing = true;
                _dashDir = dir;
                _dashTimer = 0.35f;
                return _dashDir * definition.moveSpeed * 3.2f;
            }

            // Orbit at ~5 m, close in slowly.
            var tangent = Vector3.Cross(Vector3.up, dir) * _orbitSign;
            float approach = Mathf.Clamp((dist - 5f) * 0.5f, -1f, 1f);
            return (tangent * 0.8f + dir * approach).normalized * definition.moveSpeed;
        }

        private void OnCollisionStay(Collision collision)
        {
            TryContactDamage(collision.collider);
        }

        private void OnCollisionEnter(Collision collision)
        {
            TryContactDamage(collision.collider);
        }

        private void TryContactDamage(Collider other)
        {
            if (_dead || definition == null || Time.time < _nextContactTime) return;
            var player = other.GetComponentInParent<PlayerController>();
            if (player == null) return;
            _nextContactTime = Time.time + definition.contactCooldown;
            var dir = (player.transform.position - transform.position);
            dir.y = 0f;
            player.Health.Damage(definition.contactDamage, dir.normalized);
            // Small self-knockback so chasers do not stack inside the player.
            _knockback = -dir.normalized * 3f * (1f - definition.knockbackResistance);
        }

        private void OnDamaged(float amount)
        {
            if (definition == null) return;
            if (_player != null)
            {
                var away = transform.position - _player.position;
                away.y = 0f;
                _knockback += away.normalized * 4f * (1f - definition.knockbackResistance);
            }
        }

        private void OnDied()
        {
            if (_dead) return;
            _dead = true;
            var gm = GameManager.Instance;
            if (gm != null)
            {
                gm.OnEnemyKilled(definition, transform.position);
                gm.Spawner?.OnEnemyRemoved();
                gm.Pools?.Release(gameObject);
            }
        }
    }
}
