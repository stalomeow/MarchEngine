using March.Core.Interop;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Rendering
{
    public partial class MeshRenderer : Component
    {
        private Mesh? m_Mesh;
        private List<Material?> m_Materials = [];

        public MeshRenderer() : base(New()) { }

        [JsonProperty]
        public Mesh? Mesh
        {
            get => m_Mesh;
            set
            {
                m_Mesh = value;
                SetNativeMesh(value);
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

            SetNativeMaterials(materials.Data);
        }

        protected override void OnEnable()
        {
            base.OnEnable();
            RenderPipeline.Add(this);
        }

        protected override void OnDisable()
        {
            RenderPipeline.Remove(this);
            base.OnDisable();
        }

        /// <summary>
        /// 获取世界空间 Bounds
        /// </summary>
        [NativeProperty("Bounds")]
        public partial Bounds bounds { get; }

        [NativeProperty]
        public partial Matrix4x4 PrevLocalToWorldMatrix { get; }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod("SetMesh")]
        private partial void SetNativeMesh(Mesh? mesh);

        [NativeMethod("SetMaterials")]
        private partial void SetNativeMaterials(nint materials);
    }
}
