"""Generate the game's sound effects as 16-bit mono WAV files (stdlib only).

    python3 -I tools/audio/gen_sfx.py Assets/Audio
"""
import math
import os
import random
import struct
import sys
import wave

RATE = 44100


def env(t, dur, attack=0.005, release=0.05):
    if t < attack:
        return t / attack
    if t > dur - release:
        return max(0.0, (dur - t) / release)
    return 1.0


def render(dur, fn, gain=0.8, seed=1):
    rng = random.Random(seed)
    n = int(RATE * dur)
    out = []
    for i in range(n):
        t = i / RATE
        s = fn(t, rng) * env(t, dur) * gain
        out.append(max(-1.0, min(1.0, s)))
    return out


def square(ph):
    return 1.0 if (ph % 1.0) < 0.5 else -1.0


def shoot(t, rng):
    f = 880 * (1 - t * 5) + 120
    return 0.6 * square(f * t) + 0.3 * (rng.random() * 2 - 1) * (1 - t * 8)


def hit(t, rng):
    return 0.7 * (rng.random() * 2 - 1) * math.exp(-t * 40) + 0.4 * math.sin(2 * math.pi * 180 * t) * math.exp(-t * 20)


_lp = [0.0]
def enemy_die(t, rng):
    noise = rng.random() * 2 - 1
    _lp[0] = _lp[0] * 0.85 + noise * 0.15
    f = 220 * math.exp(-t * 4)
    return 0.8 * _lp[0] * math.exp(-t * 6) + 0.4 * square(f * t) * math.exp(-t * 8)


def pickup(t, rng):
    notes = [523.25, 659.25, 783.99]
    idx = min(2, int(t / 0.06))
    return 0.6 * math.sin(2 * math.pi * notes[idx] * t) * math.exp(-(t - idx * 0.06) * 12)


def upgrade(t, rng):
    f = 300 + 900 * t
    return 0.5 * math.sin(2 * math.pi * f * t) + 0.25 * math.sin(2 * math.pi * f * 1.5 * t)


def player_hurt(t, rng):
    f = 200 - 120 * t
    return 0.6 * square(f * t) * math.exp(-t * 6) + 0.3 * (rng.random() * 2 - 1) * math.exp(-t * 10)


def game_over(t, rng):
    notes = [392.0, 349.23, 311.13, 261.63]
    idx = min(3, int(t / 0.2))
    return 0.5 * (0.6 * math.sin(2 * math.pi * notes[idx] * t) + 0.4 * square(notes[idx] * t))


def ui_click(t, rng):
    return 0.5 * math.sin(2 * math.pi * 1200 * t) * math.exp(-t * 60)


def wave_start(t, rng):
    f = 440 if t < 0.12 else 660
    return 0.5 * square(f * t) * (1 if t < 0.1 or 0.12 < t < 0.3 else 0)


SOUNDS = {
    "shoot": (0.09, shoot, 0.6),
    "hit": (0.12, hit, 0.7),
    "enemy_die": (0.35, enemy_die, 0.8),
    "pickup": (0.22, pickup, 0.7),
    "upgrade": (0.45, upgrade, 0.6),
    "player_hurt": (0.3, player_hurt, 0.8),
    "game_over": (0.85, game_over, 0.7),
    "ui_click": (0.08, ui_click, 0.6),
    "wave_start": (0.32, wave_start, 0.5),
}


def main(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for name, (dur, fn, gain) in SOUNDS.items():
        _lp[0] = 0.0
        samples = render(dur, fn, gain)
        path = os.path.join(out_dir, f"{name}.wav")
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(RATE)
            w.writeframes(b"".join(struct.pack("<h", int(s * 32767)) for s in samples))
        print(f"wrote {path} ({dur:.2f}s)")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "Assets/Audio")
