using March.Core.Interop;

namespace March.Core.Rendering
{
    public static partial class RenderPipeline
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
        public static partial void Render(Camera camera, Material? gridGizmoMaterial);
    }
}
