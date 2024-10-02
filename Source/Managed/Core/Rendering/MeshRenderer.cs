using March.Core.Binding;
using Newtonsoft.Json;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
    public enum MeshType
    {
        Cube = 0,
        Sphere = 1,
    }

    public partial class MeshRenderer : Component
    {
        [JsonProperty] private MeshType m_MeshType = MeshType.Cube;
        [JsonProperty] private List<Material?> m_Materials = [];

        public MeshRenderer() : base(RenderObject_New())
        {
            RenderObject_SetMesh(NativePtr, SimpleMesh_New());
        }

        protected override void Dispose(bool disposing)
        {
            SimpleMesh_Delete(RenderObject_GetMesh(NativePtr));
            RenderObject_Delete(NativePtr);
            base.Dispose(disposing);
        }

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
                UpdateMesh();
            }
        }

        public List<Material?> Materials
        {
            get => m_Materials;
            set => m_Materials = value;
        }

        protected override void OnUpdate()
        {
            base.OnUpdate();

            if (m_Materials.Count > 0)
            {
                RenderObject_SetMaterial(NativePtr, m_Materials[0]?.NativePtr ?? nint.Zero);
            }
            else
            {
                RenderObject_SetMaterial(NativePtr, nint.Zero);
            }
        }

        private void UpdateMesh()
        {
            nint mesh = RenderObject_GetMesh(NativePtr);
            SimpleMesh_ClearSubMeshes(mesh);

            switch (MeshType)
            {
                case MeshType.Cube:
                    SimpleMesh_AddSubMeshCube(mesh);
                    break;

                case MeshType.Sphere:
                    SimpleMesh_AddSubMeshSphere(mesh, 0.5f, 40, 40);
                    break;

                default:
                    throw new NotImplementedException($"Mesh type {MeshType} not implemented");
            }
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            UpdateMesh();
        }

        #region RenderObject

        [NativeFunction]
        private static partial nint RenderObject_New();

        [NativeFunction]
        private static partial void RenderObject_Delete(nint self);

        [NativeFunction]
        private static partial nint RenderObject_GetMesh(nint self);

        [NativeFunction]
        private static partial void RenderObject_SetMesh(nint self, nint value);

        [NativeFunction]
        private static partial void RenderObject_SetMaterial(nint self, nint value);

        #endregion

        #region SimpleMesh

        [NativeFunction]
        private static partial nint SimpleMesh_New();

        [NativeFunction]
        private static partial void SimpleMesh_Delete(nint self);

        [NativeFunction]
        private static partial void SimpleMesh_ClearSubMeshes(nint self);

        [NativeFunction]
        private static partial void SimpleMesh_AddSubMeshCube(nint self);

        [NativeFunction]
        private static partial void SimpleMesh_AddSubMeshSphere(nint self, float radius, uint sliceCount, uint stackCount);

        #endregion
    }
}
