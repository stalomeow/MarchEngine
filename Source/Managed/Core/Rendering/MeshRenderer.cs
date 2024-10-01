using March.Core.Binding;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Rendering
{
    public enum MeshType
    {
        Cube = 0,
        Sphere = 1,
    }

    public partial class MeshRenderer : Component
    {
        private nint m_RenderObject;
        private MeshType m_MeshType;
        private List<Material?> m_Materials = [];

        [JsonProperty]
        public MeshType MeshType
        {
            get => m_MeshType;
            set
            {
                if (m_MeshType != value)
                {
                    m_MeshType = value;
                    UpdateMesh();
                }
            }
        }

        [JsonProperty]
        public List<Material?> Materials
        {
            get => m_Materials;
            set => m_Materials = value;
        }

        public MeshRenderer()
        {
            m_RenderObject = RenderObject_New();
            m_MeshType = MeshType.Cube;

            RenderObject_SetIsActive(m_RenderObject, false);
            RenderObject_SetMesh(m_RenderObject, SimpleMesh_New());
            UpdateMesh();
        }

        ~MeshRenderer()
        {
            Dispose();
        }

        private void Dispose()
        {
            if (m_RenderObject == nint.Zero)
            {
                return;
            }

            nint mesh = RenderObject_GetMesh(m_RenderObject);
            SimpleMesh_Delete(mesh);

            RenderObject_Delete(m_RenderObject);
            m_RenderObject = nint.Zero;
        }

        protected override void OnMount()
        {
            base.OnMount();
            RenderPipeline.AddRenderObject(m_RenderObject);
        }

        protected override void OnUnmount()
        {
            RenderPipeline.RemoveRenderObject(m_RenderObject);
            Dispose();
            GC.SuppressFinalize(this);
            base.OnUnmount();
        }

        protected override void OnEnable()
        {
            base.OnEnable();
            RenderObject_SetIsActive(m_RenderObject, true);
        }

        protected override void OnDisable()
        {
            RenderObject_SetIsActive(m_RenderObject, false);
            base.OnDisable();
        }

        protected override void OnUpdate()
        {
            base.OnUpdate();

            RenderObject_SetPosition(m_RenderObject, MountingObject.Position);
            RenderObject_SetRotation(m_RenderObject, MountingObject.Rotation);
            RenderObject_SetScale(m_RenderObject, MountingObject.Scale);

            if (m_Materials.Count > 0)
            {
                RenderObject_SetMaterial(m_RenderObject, m_Materials[0]?.NativePtr ?? nint.Zero);
            }
            else
            {
                RenderObject_SetMaterial(m_RenderObject, nint.Zero);
            }
        }

        private void UpdateMesh()
        {
            if (m_RenderObject == nint.Zero)
            {
                return;
            }

            nint mesh = RenderObject_GetMesh(m_RenderObject);

            if (mesh == nint.Zero)
            {
                return;
            }

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

        #region RenderObject

        [NativeFunction]
        private static partial nint RenderObject_New();

        [NativeFunction]
        private static partial void RenderObject_Delete(nint self);

        [NativeFunction]
        private static partial void RenderObject_SetPosition(nint self, Vector3 value);

        [NativeFunction]
        private static partial void RenderObject_SetRotation(nint self, Quaternion value);

        [NativeFunction]
        private static partial void RenderObject_SetScale(nint self, Vector3 value);

        [NativeFunction]
        private static partial nint RenderObject_GetMesh(nint self);

        [NativeFunction]
        private static partial void RenderObject_SetMesh(nint self, nint value);

        [NativeFunction]
        private static partial void RenderObject_SetIsActive(nint self, bool value);

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
