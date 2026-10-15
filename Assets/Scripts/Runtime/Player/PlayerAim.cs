using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Resolves the aim direction on the ground plane from the mouse (ray vs plane)
    /// or the gamepad right stick. Keeps the last valid direction when idle.
    /// </summary>
    public class PlayerAim : MonoBehaviour
    {
        public Vector3 AimDirection { get; private set; } = Vector3.forward;
        public Vector3 AimPoint { get; private set; }

        private bool _usingStick;

        private void Update()
        {
            var gm = GameManager.Instance;
            if (gm == null || gm.Input == null) return;
            var input = gm.Input;

            var stick = input.AimStick;
            if (stick.sqrMagnitude > 0.1f)
            {
                _usingStick = true;
                AimDirection = new Vector3(stick.x, 0f, stick.y).normalized;
                AimPoint = transform.position + AimDirection * 6f;
                return;
            }
            if (input.PointerMovedThisFrame) _usingStick = false;
            if (_usingStick) return;

            var cam = Camera.main;
            if (cam == null) return;
            var ray = cam.ScreenPointToRay(input.PointerPosition);
            var plane = new Plane(Vector3.up, new Vector3(0f, transform.position.y + 1f, 0f));
            if (plane.Raycast(ray, out var dist))
            {
                var point = ray.GetPoint(dist);
                var dir = point - transform.position;
                dir.y = 0f;
                if (dir.sqrMagnitude > 0.05f)
                {
                    AimDirection = dir.normalized;
                    AimPoint = point;
                }
            }
        }
    }
}
