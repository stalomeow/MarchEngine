using March.Core.Binding;
using Newtonsoft.Json;
using System.ComponentModel;
using System.Numerics;
using System.Runtime.Serialization;

namespace March.Core
{
    public partial class Transform : Component
    {
        [JsonProperty]
        private List<Transform> m_Children = [];
        private Transform? m_Parent = null;

        public Transform() : base(Transform_Create()) { }

        protected override void Dispose(bool disposing)
        {
            Transform_Delete(NativePtr);
            base.Dispose(disposing);
        }

        [JsonProperty]
        public Vector3 LocalPosition
        {
            get => Transform_GetLocalPosition(NativePtr);
            set => Transform_SetLocalPosition(NativePtr, value);
        }

        [JsonProperty(nameof(LocalRotation))]
        [EditorBrowsable(EditorBrowsableState.Never)]
        private Quaternion LocalRotationSerializationOnly
        {
            get => Transform_GetLocalRotation(NativePtr);
            set => Transform_SetLocalRotationWithoutSyncEulerAngles(NativePtr, value);
        }

        public Quaternion LocalRotation
        {
            get => Transform_GetLocalRotation(NativePtr);
            set => Transform_SetLocalRotation(NativePtr, value);
        }

        [JsonProperty(nameof(LocalEulerAngles))]
        [EditorBrowsable(EditorBrowsableState.Never)]
        private Vector3 LocalEulerAnglesSerializationOnly
        {
            get => Transform_GetLocalEulerAngles(NativePtr);
            set => Transform_SetLocalEulerAnglesWithoutSyncRotation(NativePtr, value);
        }

        public Vector3 LocalEulerAngles
        {
            get => Transform_GetLocalEulerAngles(NativePtr);
            set => Transform_SetLocalEulerAngles(NativePtr, value);
        }

        [JsonProperty]
        public Vector3 LocalScale
        {
            get => Transform_GetLocalScale(NativePtr);
            set => Transform_SetLocalScale(NativePtr, value);
        }

        public Vector3 Position
        {
            get => Transform_GetPosition(NativePtr);
            set => Transform_SetPosition(NativePtr, value);
        }

        public Quaternion Rotation
        {
            get => Transform_GetRotation(NativePtr);
            set => Transform_SetRotation(NativePtr, value);
        }

        public Vector3 EulerAngles
        {
            get => Transform_GetEulerAngles(NativePtr);
            set => Transform_SetEulerAngles(NativePtr, value);
        }

        public Vector3 LossyScale => Transform_GetLossyScale(NativePtr);

        public Matrix4x4 LocalToWorldMatrix => Transform_GetLocalToWorldMatrix(NativePtr);

        public Matrix4x4 WorldToLocalMatrix => Transform_GetWorldToLocalMatrix(NativePtr);

        public Transform? Parent => m_Parent;

        public int ChildCount => m_Children.Count;

        public Transform GetChild(int index) => m_Children[index];

        public void AddChild(Transform child)
        {
            child.m_Parent?.m_Children.Remove(child);
            child.m_Parent = this;
            m_Children.Add(child);

            Transform_SetParent(child.NativePtr, NativePtr);
        }

        public void InsertChild(int index, Transform child)
        {
            child.m_Parent?.m_Children.Remove(child);
            child.m_Parent = this;
            m_Children.Insert(index, child);

            Transform_SetParent(child.NativePtr, NativePtr);
        }

        public void Detach()
        {
            if (m_Parent == null)
            {
                return;
            }

            m_Parent.m_Children.Remove(this);
            m_Parent = null;

            Transform_SetParent(NativePtr, nint.Zero);
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            foreach (Transform child in m_Children)
            {
                child.m_Parent = this;
                Transform_SetParent(child.NativePtr, NativePtr);
            }
        }

        #region Bindings

        [NativeFunction]
        private static partial nint Transform_Create();

        [NativeFunction]
        private static partial void Transform_Delete(nint transform);

        [NativeFunction]
        private static partial void Transform_SetParent(nint transform, nint parent);

        [NativeFunction]
        private static partial Vector3 Transform_GetLocalPosition(nint transform);

        [NativeFunction]
        private static partial void Transform_SetLocalPosition(nint transform, Vector3 value);

        [NativeFunction]
        private static partial Quaternion Transform_GetLocalRotation(nint transform);

        [NativeFunction]
        private static partial void Transform_SetLocalRotation(nint transform, Quaternion value);

        [NativeFunction]
        private static partial void Transform_SetLocalRotationWithoutSyncEulerAngles(nint transform, Quaternion value);

        [NativeFunction]
        private static partial Vector3 Transform_GetLocalEulerAngles(nint transform);

        [NativeFunction]
        private static partial void Transform_SetLocalEulerAngles(nint transform, Vector3 value);

        [NativeFunction]
        private static partial void Transform_SetLocalEulerAnglesWithoutSyncRotation(nint transform, Vector3 value);

        [NativeFunction]
        private static partial Vector3 Transform_GetLocalScale(nint transform);

        [NativeFunction]
        private static partial void Transform_SetLocalScale(nint transform, Vector3 value);

        [NativeFunction]
        private static partial Vector3 Transform_GetPosition(nint transform);

        [NativeFunction]
        private static partial void Transform_SetPosition(nint transform, Vector3 value);

        [NativeFunction]
        private static partial Quaternion Transform_GetRotation(nint transform);

        [NativeFunction]
        private static partial void Transform_SetRotation(nint transform, Quaternion value);

        [NativeFunction]
        private static partial Vector3 Transform_GetEulerAngles(nint transform);

        [NativeFunction]
        private static partial void Transform_SetEulerAngles(nint transform, Vector3 value);

        [NativeFunction]
        private static partial Vector3 Transform_GetLossyScale(nint transform);

        [NativeFunction]
        private static partial Matrix4x4 Transform_GetLocalToWorldMatrix(nint transform);

        [NativeFunction]
        private static partial Matrix4x4 Transform_GetWorldToLocalMatrix(nint transform);

        #endregion
    }
}
