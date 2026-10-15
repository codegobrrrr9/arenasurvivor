using UnityEngine;

namespace Arena
{
    /// <summary>Base for state-driven panels: visible only in the states it lists.</summary>
    public abstract class UIPanel : MonoBehaviour
    {
        [SerializeField] protected GameState[] visibleIn;
        [SerializeField] protected bool built;

        protected GameManager Gm => GameManager.Instance;

        /// <summary>Creates the child hierarchy and assigns serialized references. Safe to call in the editor.</summary>
        public abstract void Build();

        protected virtual void Awake()
        {
            if (!built) Build();
        }

        protected virtual void OnEnable()
        {
            if (Gm == null) return;
            Gm.StateChanged += OnStateChanged;
            OnStateChanged(Gm.State);
        }

        protected virtual void OnDisable()
        {
            if (Gm == null) return;
            Gm.StateChanged -= OnStateChanged;
        }

        protected virtual void Start()
        {
            if (Gm != null) OnStateChanged(Gm.State);
        }

        private void OnStateChanged(GameState state)
        {
            bool show = false;
            foreach (var s in visibleIn) if (s == state) { show = true; break; }
            SetVisible(show);
            if (show) OnShown();
        }

        protected virtual void SetVisible(bool show)
        {
            if (transform.childCount == 0) return;
            for (int i = 0; i < transform.childCount; i++) transform.GetChild(i).gameObject.SetActive(show);
        }

        protected virtual void OnShown() { }

        protected void Click(AudioClip clip = null)
        {
            if (Gm == null || Gm.Assets == null) return;
            Gm.Audio?.Play(clip != null ? clip : Gm.Assets.uiClick, 0.6f);
        }
    }
}
