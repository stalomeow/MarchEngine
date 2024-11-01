using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.ComponentModel;
using System.Numerics;
using System.Runtime.Serialization;

namespace March.Core
{
    [DisableComponentEnabledCheckbox]
    public partial class Transform : Component
    {
        [JsonProperty]
        private List<Transform> m_Children = [];
        private Transform? m_Parent = null;

        public Transform() : base(Transform_Create()) { }

        protected override void DisposeNative()
        {
            Transform_Delete(NativePtr);
            base.DisposeNative();
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

        public Matrix4x4 LocalToWorldMatrix
        {
            get => Transform_GetLocalToWorldMatrix(NativePtr);
            set => Transform_SetLocalToWorldMatrix(NativePtr, value);
        }

        public Matrix4x4 WorldToLocalMatrix
        {
            get => Transform_GetWorldToLocalMatrix(NativePtr);
            set => Transform_SetWorldToLocalMatrix(NativePtr, value);
        }

        public Vector3 Forward => Transform_GetForward(NativePtr);

        public Vector3 Right => Transform_GetRight(NativePtr);

        public Vector3 Up => Transform_GetUp(NativePtr);

        /// <summary>
        /// 变换一个向量，受 rotation 和 scale 影响
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public Vector3 TransformVector(Vector3 vector) => Transform_TransformVector(NativePtr, vector);

        /// <summary>
        /// 变换一个方向，只受 rotation 影响
        /// </summary>
        /// <param name="direction"></param>
        /// <returns></returns>
        public Vector3 TransformDirection(Vector3 direction) => Transform_TransformDirection(NativePtr, direction);

        /// <summary>
        /// 变换一个点，受 rotation、scale 和 position 影响
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        public Vector3 TransformPoint(Vector3 point) => Transform_TransformPoint(NativePtr, point);

        /// <summary>
        /// 逆变换一个向量，受 rotation 和 scale 影响
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public Vector3 InverseTransformVector(Vector3 vector) => Transform_InverseTransformVector(NativePtr, vector);

        /// <summary>
        /// 逆变换一个方向，只受 rotation 影响
        /// </summary>
        /// <param name="direction"></param>
        /// <returns></returns>
        public Vector3 InverseTransformDirection(Vector3 direction) => Transform_InverseTransformDirection(NativePtr, direction);

        /// <summary>
        /// 逆变换一个点，受 rotation、scale 和 position 影响
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        public Vector3 InverseTransformPoint(Vector3 point) => Transform_InverseTransformPoint(NativePtr, point);

        public Transform? Parent => m_Parent;

        public int ChildCount => m_Children.Count;

        public Transform GetChild(int index) => m_Children[index];

        public void AddChild(Transform child)
        {
            if (child.m_Parent != null)
            {
                throw new InvalidOperationException("The transform already has a parent.");
            }

            m_Children.Add(child);
            child.m_Parent = this;
            Transform_SetParent(child.NativePtr, NativePtr);
        }

        public void InsertChild(int index, Transform child)
        {
            if (child.m_Parent != null)
            {
                throw new InvalidOperationException("The transform already has a parent.");
            }

            m_Children.Insert(index, child);
            child.m_Parent = this;
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
        private static partial void Transform_SetLocalToWorldMatrix(nint transform, Matrix4x4 value);

        [NativeFunction]
        private static partial Matrix4x4 Transform_GetWorldToLocalMatrix(nint transform);

        [NativeFunction]
        private static partial void Transform_SetWorldToLocalMatrix(nint transform, Matrix4x4 value);

        [NativeFunction]
        private static partial Vector3 Transform_GetForward(nint transform);

        [NativeFunction]
        private static partial Vector3 Transform_GetRight(nint transform);

        [NativeFunction]
        private static partial Vector3 Transform_GetUp(nint transform);

        [NativeFunction]
        private static partial Vector3 Transform_TransformVector(nint transform, Vector3 vector);

        [NativeFunction]
        private static partial Vector3 Transform_TransformDirection(nint transform, Vector3 direction);

        [NativeFunction]
        private static partial Vector3 Transform_TransformPoint(nint transform, Vector3 point);

        [NativeFunction]
        private static partial Vector3 Transform_InverseTransformVector(nint transform, Vector3 vector);

        [NativeFunction]
        private static partial Vector3 Transform_InverseTransformDirection(nint transform, Vector3 direction);

        [NativeFunction]
        private static partial Vector3 Transform_InverseTransformPoint(nint transform, Vector3 point);

        #endregion
    }
}
