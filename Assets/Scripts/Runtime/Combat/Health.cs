using System;
using UnityEngine;

namespace Arena
{
    public class Health : MonoBehaviour, IDamageable
    {
        [SerializeField] private float max = 100f;
        public float invulnerabilityAfterHit = 0f;

        public float Max => max;
        public float Current { get; private set; }
        public bool IsDead => Current <= 0f;
        public float Fraction => max > 0f ? Mathf.Clamp01(Current / max) : 0f;

        public event Action<float, float> Changed;   // current, max
        public event Action<float> Damaged;
        public event Action Died;

        private float _invulnerableUntil;

        private void Awake()
        {
            if (Current <= 0f) Current = max;
        }

        public void Configure(float newMax, bool refill = true)
        {
            max = newMax;
            if (refill) Current = max;
            else Current = Mathf.Min(Current, max);
            Changed?.Invoke(Current, max);
        }

        public void Damage(float amount, Vector3 hitDirection)
        {
            if (IsDead || amount <= 0f) return;
            if (Time.time < _invulnerableUntil) return;
            _invulnerableUntil = Time.time + invulnerabilityAfterHit;

            Current = Mathf.Max(0f, Current - amount);
            Damaged?.Invoke(amount);
            Changed?.Invoke(Current, max);
            if (Current <= 0f) Died?.Invoke();
        }

        public void Heal(float amount)
        {
            if (amount <= 0f) return;
            Current = Mathf.Min(max, Current + amount);
            Changed?.Invoke(Current, max);
        }
    }
}
