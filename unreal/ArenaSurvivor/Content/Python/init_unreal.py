"""Runs automatically when the editor starts (Python Editor Script Plugin).

Defers the real bootstrap to the first Slate tick so the editor has finished
initialising before assets are imported and a level is created.
"""
import unreal

try:
    import arena_bootstrap
except Exception as exc:  # pragma: no cover - editor only
    unreal.log_error(f"[Arena] could not import arena_bootstrap: {exc}")
    arena_bootstrap = None

_handle = None


def _once(delta_seconds):
    global _handle
    if _handle is not None:
        unreal.unregister_slate_post_tick_callback(_handle)
        _handle = None
    if arena_bootstrap is not None:
        try:
            arena_bootstrap.run()
        except Exception as exc:
            unreal.log_error(f"[Arena] bootstrap failed: {exc}")


if arena_bootstrap is not None:
    _handle = unreal.register_slate_post_tick_callback(_once)
