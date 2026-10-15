using UnityEngine;

namespace Arena
{
    public interface IDamageable
    {
        void Damage(float amount, Vector3 hitDirection);
    }
}
