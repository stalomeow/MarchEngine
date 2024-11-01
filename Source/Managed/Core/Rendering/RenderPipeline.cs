using March.Core.Interop;

namespace March.Core.Rendering
{
    public static partial class RenderPipeline
    {
        internal static void AddRenderObject(nint renderObject)
        {
            RenderPipeline_AddRenderObject(GetInstance(), renderObject);
        }

        internal static void RemoveRenderObject(nint renderObject)
        {
            RenderPipeline_RemoveRenderObject(GetInstance(), renderObject);
        }

        internal static void AddLight(nint light)
        {
            RenderPipeline_AddLight(GetInstance(), light);
        }

        internal static void RemoveLight(nint light)
        {
            RenderPipeline_RemoveLight(GetInstance(), light);
        }

        public static void Render(Camera camera, Material? gridGizmoMaterial)
        {
            RenderPipeline_Render(GetInstance(), camera.NativePtr, gridGizmoMaterial?.NativePtr ?? nint.Zero);
        }

        private static nint GetInstance()
        {
            return Application_GetRenderPipeline();
        }

        #region Bindings

        [NativeFunction]
        private static partial nint Application_GetRenderPipeline();

        [NativeFunction]
        private static partial void RenderPipeline_AddRenderObject(nint self, nint renderObject);

        [NativeFunction]
        private static partial void RenderPipeline_RemoveRenderObject(nint self, nint renderObject);

        [NativeFunction]
        private static partial void RenderPipeline_AddLight(nint self, nint light);

        [NativeFunction]
        private static partial void RenderPipeline_RemoveLight(nint self, nint light);

        [NativeFunction]
        private static partial void RenderPipeline_Render(nint self, nint camera, nint gridGizmoMaterial);

        #endregion
    }
}
