using March.Core.Binding;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
    public enum MeshType
    {
        Cube,
        Sphere,
        Custom,
    }

    public partial class MeshRenderer : Component
    {
        private MeshType m_MeshType = MeshType.Cube;
        private List<Material?> m_Materials = [];
        private Mesh? m_Mesh;

        public MeshRenderer() : base(RenderObject_New()) { }

        protected override void DisposeNative()
        {
            RenderObject_Delete(NativePtr);
            base.DisposeNative();
        }

        [JsonProperty]
        public Mesh Mesh
        {
            get => m_Mesh!;
            set
            {
                m_Mesh = value;
                UpdateMesh(false);
            }
        }

        [JsonProperty]
        public MeshType MeshType
        {
            get => m_MeshType;
            set
            {
                if (m_MeshType == value)
                {
                    return;
                }

                m_MeshType = value;
                UpdateMesh(true);
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

        private void UpdateMesh(bool resetMesh)
        {
            if (resetMesh)
            {
                switch (MeshType)
                {
                    case MeshType.Cube:
                        m_Mesh = new Mesh();
                        m_Mesh.AddSubMeshCube();
                        break;

                    case MeshType.Sphere:
                        m_Mesh = new Mesh();
                        m_Mesh.AddSubMeshSphere(0.5f, 40, 40);
                        break;
                }
            }
            else
            {
                if (m_Mesh != null)
                {
                    m_MeshType = MeshType.Custom;
                }
                else
                {
                    m_MeshType = MeshType.Cube;
                    m_Mesh = new Mesh();
                    m_Mesh.AddSubMeshCube();
                }
            }

            RenderObject_SetMesh(NativePtr, m_Mesh?.NativePtr ?? nint.Zero);
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            UpdateMesh(false);
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
