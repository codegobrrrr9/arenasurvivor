using UnityEngine;

namespace Arena
{
    /// <summary>Fires pooled projectiles in a fan whose width grows with projectileCount.</summary>
    [RequireComponent(typeof(PlayerStats), typeof(PlayerAim))]
    public class PlayerShooter : MonoBehaviour
    {
        public Transform muzzle;
        public float fanAngle = 12f;

        private PlayerStats _stats;
        private PlayerAim _aim;
        private float _cooldown;

        private void Awake()
        {
            _stats = GetComponent<PlayerStats>();
            _aim = GetComponent<PlayerAim>();
        }

        private void Update()
        {
            var gm = GameManager.Instance;
            if (gm == null || gm.State != GameState.Playing || gm.Input == null) return;

            _cooldown -= Time.deltaTime;
            if (!gm.Input.FireHeld || _cooldown > 0f) return;
            _cooldown = 1f / Mathf.Max(0.1f, _stats.fireRate);
            Fire(gm);
        }

        private void Fire(GameManager gm)
        {
            var assets = gm.Assets;
            if (assets == null || assets.projectilePrefab == null || gm.Pools == null) return;

            var origin = muzzle != null ? muzzle.position : transform.position + Vector3.up;
            var forward = _aim.AimDirection;
            int count = Mathf.Max(1, _stats.projectileCount);
            float start = -fanAngle * (count - 1) * 0.5f;

            for (int i = 0; i < count; i++)
            {
                var dir = Quaternion.Euler(0f, start + fanAngle * i, 0f) * forward;
                var go = gm.Pools.Get(assets.projectilePrefab, origin, Quaternion.LookRotation(dir, Vector3.up));
                go.GetComponent<Projectile>()?.Launch(dir, _stats.damage, _stats.projectileSpeed);
            }
            gm.Audio?.Play(assets.shoot, 0.35f);
        }
    }
}
