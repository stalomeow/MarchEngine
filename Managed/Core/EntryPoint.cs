using DX12Demo.Core.Rendering;
using System.Numerics;
using System.Runtime.InteropServices;

namespace DX12Demo.Core
{
    internal unsafe partial class EntryPoint
    {
        public static event Action? OnTick;
        private static readonly List<GameObject> s_GameObjects = [];

        [UnmanagedCallersOnly]
        private static void OnNativeInitialize()
        {
            try
            {
                GameObject light = new();
                Light l = light.AddComponent<Light>();
                l.Color = Color.Red;

                GameObject sphere = new() { Scale = new Vector3(3f) };
                MeshRenderer mr = sphere.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Sphere;

                s_GameObjects.Add(light);
                s_GameObjects.Add(sphere);
            }
            catch (Exception e)
            {
                Debug.LogException(e);
            }
        }

        public static readonly float RotateSpeed = 20.0f;

        [UnmanagedCallersOnly]
        private static void OnNativeTick()
        {
            try
            {
                OnTick?.Invoke();

                foreach (var gameObject in s_GameObjects)
                {
                    gameObject.Update();

                    if (gameObject.TryGetComponent(out Light? light))
                    {
                        gameObject.Rotation *= Quaternion.CreateFromAxisAngle(Vector3.UnitY, MathF.PI / 180.0f * Time.Delta * RotateSpeed);
                    }
                }
            }
            catch (Exception e)
            {
                Debug.LogException(e);
            }
        }
    }
}
