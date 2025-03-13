using March.Core.Interop;

namespace March.Core.Rendering
{
    public static partial class RenderPipeline
    {
        [NativeMethod("AddMeshRenderer")]
        internal static partial void Add(MeshRenderer renderer);

        [NativeMethod("RemoveMeshRenderer")]
        internal static partial void Remove(MeshRenderer renderer);

        [NativeMethod("AddLight")]
        internal static partial void Add(Light light);

        [NativeMethod("RemoveLight")]
        internal static partial void Remove(Light light);

        [NativeMethod]
        internal static partial void SetSkyboxMaterial(Material? material);

        [NativeMethod]
        internal static partial void BakeEnvLight(Texture radianceMap, float diffuseIntensityMultiplier, float specularIntensityMultiplier);
    }
}
