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
        internal static partial void Render(Camera camera, Material? gridGizmoMaterial);

        public static void Render(Camera camera) => Render(camera, null);
    }
}
