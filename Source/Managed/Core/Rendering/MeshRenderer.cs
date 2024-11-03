using March.Core.Interop;
using Newtonsoft.Json;

namespace March.Core.Rendering
{
    public partial class MeshRenderer : Component
    {
        private Mesh? m_Mesh;
        private List<Material?> m_Materials = [];

        public MeshRenderer() : base(RenderObject_New()) { }

        protected override void DisposeNative()
        {
            RenderObject_Delete(NativePtr);
            base.DisposeNative();
        }

        [JsonProperty]
        public Mesh? Mesh
        {
            get => m_Mesh;
            set
            {
                m_Mesh = value;
                RenderObject_SetMesh(NativePtr, value?.NativePtr ?? nint.Zero);
            }
        }

        /// <summary>
        /// 返回 Material 列表的拷贝，如果修改其中的 Material，需要重新将列表赋值回来
        /// </summary>
        [JsonProperty]
        public List<Material?> Materials
        {
            get => m_Materials;
            set
            {
                m_Materials = value;
                SyncNativeMaterials();
            }
        }

        internal void SyncNativeMaterials()
        {
            using NativeArray<nint> materials = new(m_Materials.Count);

            for (int i = 0; i < m_Materials.Count; i++)
            {
                materials[i] = m_Materials[i]?.NativePtr ?? nint.Zero;
            }

            RenderObject_SetMaterials(NativePtr, materials.Data);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint RenderObject_New();

        [NativeFunction]
        private static partial void RenderObject_Delete(nint self);

        [NativeFunction]
        private static partial void RenderObject_SetMesh(nint self, nint value);

        [NativeFunction]
        private static partial void RenderObject_SetMaterials(nint self, nint materials);

        #endregion
    }
}
