using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Fire-and-forget sound playback. On WebGL the browser blocks audio until the
    /// first user gesture; the main menu's Play button is that gesture.
    /// </summary>
    [RequireComponent(typeof(AudioSource))]
    public class AudioManager : MonoBehaviour
    {
        [Range(0f, 1f)] public float masterVolume = 0.8f;
        public float pitchJitter = 0.05f;

        private AudioSource _source;

        private void Awake()
        {
            _source = GetComponent<AudioSource>();
            _source.playOnAwake = false;
            _source.spatialBlend = 0f;
            _source.ignoreListenerPause = true;
        }

        public void Play(AudioClip clip, float volume = 1f)
        {
            if (clip == null || _source == null) return;
            _source.pitch = 1f + Random.Range(-pitchJitter, pitchJitter);
            _source.PlayOneShot(clip, volume * masterVolume);
        }
    }
}
