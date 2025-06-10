using March.Core.Interop;

namespace March.Core.Rendering
{
    internal static partial class RenderPipeline
    {
        [NativeMethod("AddMeshRenderer")]
        public static partial void Add(MeshRenderer renderer);

        [NativeMethod("RemoveMeshRenderer")]
        public static partial void Remove(MeshRenderer renderer);

        [NativeMethod("AddLight")]
        public static partial void Add(Light light);

        [NativeMethod("RemoveLight")]
        public static partial void Remove(Light light);

        [NativeMethod]
        public static partial void SetSkyboxMaterial(Material? material);

        [NativeMethod]
        public static partial void BakeEnvLight(Texture radianceMap, float diffuseIntensityMultiplier, float specularIntensityMultiplier);

        public static void Initialize()
        {
            Init();
            Application.OnQuit += Release;
        }

        [NativeMethod("Initialize")]
        private static partial void Init();

        [NativeMethod]
        private static partial void Release();
    }
}
