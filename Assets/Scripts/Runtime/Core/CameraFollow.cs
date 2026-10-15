using UnityEngine;

namespace Arena
{
    /// <summary>Top-down chase camera with a little lead toward the aim point.</summary>
    public class CameraFollow : MonoBehaviour
    {
        public Vector3 offset = new Vector3(0f, 19f, -11f);
        public float smoothTime = 0.15f;
        public float aimLead = 2.5f;

        private Vector3 _velocity;

        private void LateUpdate()
        {
            var gm = GameManager.Instance;
            if (gm == null || gm.Player == null) return;

            var target = gm.Player.transform.position;
            var aim = gm.Player.Aim != null ? gm.Player.Aim.AimDirection : Vector3.zero;
            var focus = target + aim * aimLead;
            var desired = focus + offset;

            transform.position = Vector3.SmoothDamp(transform.position, desired, ref _velocity, smoothTime, Mathf.Infinity, Time.unscaledDeltaTime);
            transform.rotation = Quaternion.LookRotation(focus - transform.position, Vector3.up);
        }

        public void SnapTo(Vector3 target)
        {
            transform.position = target + offset;
            transform.rotation = Quaternion.LookRotation(target - transform.position, Vector3.up);
            _velocity = Vector3.zero;
        }
    }
}
