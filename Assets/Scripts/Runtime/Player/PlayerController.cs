using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Physics-driven movement on the y = 0 plane. The rigidbody is dynamic so
    /// pillars and crates block the player, but gravity is off and Y is frozen.
    /// </summary>
    [RequireComponent(typeof(Rigidbody), typeof(PlayerStats), typeof(Health))]
    public class PlayerController : MonoBehaviour
    {
        public float turnSpeed = 720f;

        public PlayerStats Stats { get; private set; }
        public Health Health { get; private set; }
        public PlayerAim Aim { get; private set; }
        public Rigidbody Body { get; private set; }

        private Vector2 _moveInput;
        private float _arenaRadius = 20f;

        private void Awake()
        {
            Stats = GetComponent<PlayerStats>();
            Health = GetComponent<Health>();
            Aim = GetComponent<PlayerAim>();
            Body = GetComponent<Rigidbody>();
            Body.useGravity = false;
            Body.constraints = RigidbodyConstraints.FreezeRotation | RigidbodyConstraints.FreezePositionY;
            Body.interpolation = RigidbodyInterpolation.Interpolate;
            Health.Damaged += OnDamaged;
        }

        private void Start()
        {
            var gm = GameManager.Instance;
            if (gm != null && gm.Assets != null) _arenaRadius = gm.Assets.arenaRadius;
            if (Health.Max <= 0f) Health.Configure(Stats.maxHp);
        }

        public void ResetForRun()
        {
            Stats.ResetToBase();
            Health.Configure(Stats.maxHp);
            Body.linearVelocity = Vector3.zero;
            Body.position = Vector3.zero;
            transform.position = Vector3.zero;
            transform.rotation = Quaternion.identity;
            gameObject.SetActive(true);
            var cam = Camera.main != null ? Camera.main.GetComponent<CameraFollow>() : null;
            if (cam != null) cam.SnapTo(Vector3.zero);
        }

        private void Update()
        {
            var gm = GameManager.Instance;
            if (gm == null || gm.State != GameState.Playing || gm.Input == null)
            {
                _moveInput = Vector2.zero;
                return;
            }
            _moveInput = Vector2.ClampMagnitude(gm.Input.Move, 1f);

            if (Aim != null)
            {
                var target = Quaternion.LookRotation(Aim.AimDirection, Vector3.up);
                transform.rotation = Quaternion.RotateTowards(transform.rotation, target, turnSpeed * Time.deltaTime);
            }
        }

        private void FixedUpdate()
        {
            var velocity = new Vector3(_moveInput.x, 0f, _moveInput.y) * Stats.moveSpeed;
            Body.linearVelocity = velocity;

            var p = Body.position;
            var flat = new Vector3(p.x, 0f, p.z);
            if (flat.magnitude > _arenaRadius)
            {
                flat = flat.normalized * _arenaRadius;
                Body.position = new Vector3(flat.x, 0f, flat.z);
            }
        }

        private void OnDamaged(float amount)
        {
            var gm = GameManager.Instance;
            if (gm != null && gm.Assets != null) gm.Audio?.Play(gm.Assets.playerHurt, 0.8f);
        }
    }
}
