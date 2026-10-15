using System;
using UnityEngine;

namespace Arena
{
    /// <summary>
    /// Owns the game state machine, score, wave and XP bookkeeping. Every other
    /// system talks to this singleton; UI panels listen to its events.
    /// </summary>
    [DefaultExecutionOrder(-100)]
    public class GameManager : MonoBehaviour
    {
        public static GameManager Instance { get; private set; }

        [SerializeField] private GameAssets assets;
        public GameAssets Assets => assets;

        public GameState State { get; private set; } = GameState.Menu;
        public int Score { get; private set; }
        public int Wave { get; private set; }
        public int Level { get; private set; }
        public int Xp { get; private set; }
        public int XpToNext => 10 + Level * 6;
        public int HighScore { get; private set; }

        public PlayerController Player { get; private set; }
        public WaveSpawner Spawner { get; private set; }
        public UpgradeSystem Upgrades { get; private set; }
        public PoolService Pools { get; private set; }
        public AudioManager Audio { get; private set; }
        public GameInput Input { get; private set; }

        public event Action<GameState> StateChanged;
        public event Action StatsChanged;   // score, wave, xp, level
        public event Action<int> WaveStarted;

        private GameState _stateBeforePause;
        private bool _pendingLevelUpgrade;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            Spawner = FindFirstObjectByType<WaveSpawner>();
            Upgrades = FindFirstObjectByType<UpgradeSystem>();
            Pools = FindFirstObjectByType<PoolService>();
            Audio = FindFirstObjectByType<AudioManager>();
            Input = FindFirstObjectByType<GameInput>();
            HighScore = PlayerPrefs.GetInt("arena.highscore", 0);

            if (assets == null)
            {
                Debug.LogError("[Arena] GameManager has no GameAssets assigned. Run Arena > Build Project.");
            }
        }

        private void Start()
        {
            EnsurePlayer();
            SetState(GameState.Menu);
        }

        private void Update()
        {
            if (Input == null) return;
            if (Input.PausePressed)
            {
                if (State == GameState.Playing) Pause();
                else if (State == GameState.Paused) Resume();
            }
        }

        // ------------------------------------------------------------ flow

        public void StartRun()
        {
            Score = 0;
            Wave = 0;
            Level = 0;
            Xp = 0;
            _pendingLevelUpgrade = false;

            Spawner?.StopWaves();
            Pools?.ReleaseAll();
            Upgrades?.ResetRun();
            EnsurePlayer();
            Player.ResetForRun();
            StatsChanged?.Invoke();

            SetState(GameState.Playing);
            NextWave();
        }

        public void ReturnToMenu()
        {
            Pools?.ReleaseAll();
            Spawner?.StopWaves();
            Player?.ResetForRun();
            SetState(GameState.Menu);
        }

        public void Pause()
        {
            if (State != GameState.Playing) return;
            _stateBeforePause = State;
            SetState(GameState.Paused);
        }

        public void Resume()
        {
            if (State != GameState.Paused) return;
            SetState(_stateBeforePause);
        }

        public void NextWave()
        {
            Wave++;
            StatsChanged?.Invoke();
            WaveStarted?.Invoke(Wave);
            Audio?.Play(assets.waveStart, 0.7f);
            Spawner?.StartWave(Wave);
        }

        public void OnWaveCleared()
        {
            if (State != GameState.Playing) return;
            Player.Health.Heal(Player.Stats.maxHp * 0.15f);
            OpenUpgradeChoice();
        }

        public void OnUpgradeChosen()
        {
            if (State != GameState.Upgrading) return;
            SetState(GameState.Playing);
            if (_pendingLevelUpgrade)
            {
                _pendingLevelUpgrade = false;
            }
            if (Spawner != null && !Spawner.WaveInProgress)
            {
                NextWave();
            }
        }

        public void OnPlayerDied()
        {
            if (State == GameState.GameOver) return;
            Spawner?.StopWaves();
            if (Score > HighScore)
            {
                HighScore = Score;
                PlayerPrefs.SetInt("arena.highscore", HighScore);
                PlayerPrefs.Save();
            }
            Audio?.Play(assets.gameOver);
            SetState(GameState.GameOver);
        }

        // ------------------------------------------------------------ scoring

        public void OnEnemyKilled(EnemyDefinition def, Vector3 position)
        {
            Score += def.scoreValue;
            Audio?.Play(assets.enemyDie, 0.6f);
            StatsChanged?.Invoke();

            if (Pools == null) return;
            bool dropHealth = UnityEngine.Random.value < def.dropHealthChance;
            var prefab = dropHealth ? assets.healthPickupPrefab : assets.xpPickupPrefab;
            if (prefab == null) return;
            var go = Pools.Get(prefab, position + Vector3.up * 0.5f, Quaternion.identity);
            var pickup = go.GetComponent<Pickup>();
            if (pickup != null)
            {
                pickup.Init(dropHealth ? PickupType.Health : PickupType.Xp, dropHealth ? 25 : def.xpValue);
            }
        }

        public void OnPickupCollected(PickupType type, int value)
        {
            Audio?.Play(assets.pickup, 0.6f);
            if (type == PickupType.Health)
            {
                Player.Health.Heal(value);
                return;
            }
            Xp += value;
            Score += value;
            while (Xp >= XpToNext)
            {
                Xp -= XpToNext;
                Level++;
                _pendingLevelUpgrade = true;
            }
            StatsChanged?.Invoke();
            if (_pendingLevelUpgrade && State == GameState.Playing)
            {
                OpenUpgradeChoice();
            }
        }

        // ------------------------------------------------------------ helpers

        private void OpenUpgradeChoice()
        {
            if (Upgrades == null || Upgrades.Roll(3).Count == 0)
            {
                // Nothing left to offer: skip the choice.
                _pendingLevelUpgrade = false;
                if (Spawner != null && !Spawner.WaveInProgress) NextWave();
                return;
            }
            Audio?.Play(assets.upgrade, 0.7f);
            SetState(GameState.Upgrading);
        }

        private void EnsurePlayer()
        {
            if (Player != null) return;
            Player = FindFirstObjectByType<PlayerController>();
            if (Player == null && assets != null && assets.playerPrefab != null)
            {
                Player = Instantiate(assets.playerPrefab, Vector3.zero, Quaternion.identity).GetComponent<PlayerController>();
            }
            if (Player != null)
            {
                Player.Health.Died -= OnPlayerDied;
                Player.Health.Died += OnPlayerDied;
            }
        }

        private void SetState(GameState next)
        {
            State = next;
            Time.timeScale = next == GameState.Playing ? 1f : 0f;
            AudioListener.pause = next == GameState.Paused;
            Input?.SetGameplayEnabled(next == GameState.Playing);
            StateChanged?.Invoke(next);
        }

        private void OnDestroy()
        {
            if (Instance == this)
            {
                Time.timeScale = 1f;
                Instance = null;
            }
        }
    }
}
