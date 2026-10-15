using UnityEngine;
using UnityEngine.InputSystem;

namespace Arena
{
    /// <summary>
    /// All input actions are built in code (no .inputactions asset to author or wire).
    /// Keyboard + mouse and gamepad both work; whichever device moved last drives aiming.
    /// </summary>
    public class GameInput : MonoBehaviour
    {
        private InputActionMap _gameplay;
        private InputActionMap _global;
        private InputAction _move;
        private InputAction _aimPointer;
        private InputAction _aimStick;
        private InputAction _fire;
        private InputAction _pause;
        private InputAction _confirm;

        public Vector2 Move => _move.ReadValue<Vector2>();
        public Vector2 PointerPosition => _aimPointer.ReadValue<Vector2>();
        public Vector2 AimStick => _aimStick.ReadValue<Vector2>();
        public bool FireHeld => _fire.IsPressed();
        public bool PausePressed => _pause.WasPressedThisFrame();
        public bool ConfirmPressed => _confirm.WasPressedThisFrame();
        public bool PointerMovedThisFrame => Mouse.current != null && Mouse.current.delta.ReadValue().sqrMagnitude > 0.01f;

        private void Awake()
        {
            _gameplay = new InputActionMap("Gameplay");

            _move = _gameplay.AddAction("Move", InputActionType.Value);
            _move.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/w").With("Down", "<Keyboard>/s")
                .With("Left", "<Keyboard>/a").With("Right", "<Keyboard>/d");
            _move.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/upArrow").With("Down", "<Keyboard>/downArrow")
                .With("Left", "<Keyboard>/leftArrow").With("Right", "<Keyboard>/rightArrow");
            _move.AddBinding("<Gamepad>/leftStick");
            _move.AddBinding("<Gamepad>/dpad");

            _aimPointer = _gameplay.AddAction("AimPointer", InputActionType.Value, "<Mouse>/position");
            _aimPointer.AddBinding("<Pointer>/position");
            _aimStick = _gameplay.AddAction("AimStick", InputActionType.Value, "<Gamepad>/rightStick");

            _fire = _gameplay.AddAction("Fire", InputActionType.Button, "<Mouse>/leftButton");
            _fire.AddBinding("<Gamepad>/rightTrigger");
            _fire.AddBinding("<Gamepad>/buttonSouth");
            _fire.AddBinding("<Keyboard>/space");

            _global = new InputActionMap("Global");
            _pause = _global.AddAction("Pause", InputActionType.Button, "<Keyboard>/escape");
            _pause.AddBinding("<Gamepad>/start");
            _pause.AddBinding("<Keyboard>/p");
            _confirm = _global.AddAction("Confirm", InputActionType.Button, "<Keyboard>/enter");
            _confirm.AddBinding("<Gamepad>/buttonSouth");

            _global.Enable();
            _gameplay.Enable();
        }

        public void SetGameplayEnabled(bool enabled)
        {
            if (_gameplay == null) return;
            if (enabled) _gameplay.Enable(); else _gameplay.Disable();
        }

        private void OnEnable()
        {
            _global?.Enable();
        }

        private void OnDisable()
        {
            _gameplay?.Disable();
            _global?.Disable();
        }

        private void OnDestroy()
        {
            _gameplay?.Dispose();
            _global?.Dispose();
        }
    }
}
