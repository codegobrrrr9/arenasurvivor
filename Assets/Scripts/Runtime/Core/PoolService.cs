using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Pool;

namespace Arena
{
    public interface IPoolable
    {
        void OnSpawned();
        void OnDespawned();
    }

    /// <summary>Prefab-keyed object pooling on top of UnityEngine.Pool.</summary>
    public class PoolService : MonoBehaviour
    {
        private readonly Dictionary<GameObject, ObjectPool<GameObject>> _pools = new();
        private readonly Dictionary<GameObject, GameObject> _instanceToPrefab = new();
        private readonly HashSet<GameObject> _active = new();
        private Transform _root;

        private void Awake()
        {
            _root = new GameObject("Pooled").transform;
            _root.SetParent(transform, false);
        }

        public GameObject Get(GameObject prefab, Vector3 position, Quaternion rotation)
        {
            if (!_pools.TryGetValue(prefab, out var pool))
            {
                pool = new ObjectPool<GameObject>(
                    createFunc: () =>
                    {
                        var go = Instantiate(prefab, _root);
                        _instanceToPrefab[go] = prefab;
                        return go;
                    },
                    actionOnGet: go => go.SetActive(true),
                    actionOnRelease: go => go.SetActive(false),
                    actionOnDestroy: go => { _instanceToPrefab.Remove(go); Destroy(go); },
                    collectionCheck: false,
                    defaultCapacity: 16,
                    maxSize: 512);
                _pools[prefab] = pool;
            }

            var instance = pool.Get();
            instance.transform.SetPositionAndRotation(position, rotation);
            _active.Add(instance);
            foreach (var p in instance.GetComponents<IPoolable>()) p.OnSpawned();
            return instance;
        }

        public void Release(GameObject instance)
        {
            if (instance == null || !_active.Remove(instance)) return;
            foreach (var p in instance.GetComponents<IPoolable>()) p.OnDespawned();
            if (_instanceToPrefab.TryGetValue(instance, out var prefab) && _pools.TryGetValue(prefab, out var pool))
            {
                pool.Release(instance);
            }
            else
            {
                Destroy(instance);
            }
        }

        public void ReleaseAll()
        {
            var snapshot = new List<GameObject>(_active);
            foreach (var go in snapshot) Release(go);
        }

        public int ActiveCount => _active.Count;
    }
}
