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

        public Transform() : base(New()) { }

        [JsonProperty]
        [NativeProperty]
        public partial Vector3 LocalPosition { get; set; }

        [NativeProperty]
        public partial Quaternion LocalRotation { get; set; }

        [NativeProperty]
        public partial Vector3 LocalEulerAngles { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial Vector3 LocalScale { get; set; }

        [NativeProperty]
        public partial Vector3 Position { get; set; }

        [NativeProperty]
        public partial Quaternion Rotation { get; set; }

        [NativeProperty]
        public partial Vector3 EulerAngles { get; set; }

        [NativeProperty]
        public partial Vector3 LossyScale { get; }

        [NativeProperty]
        public partial Matrix4x4 LocalToWorldMatrix { get; set; }

        [NativeProperty]
        public partial Matrix4x4 WorldToLocalMatrix { get; set; }

        [NativeProperty]
        public partial Vector3 Forward { get; }

        [NativeProperty]
        public partial Vector3 Right { get; }

        [NativeProperty]
        public partial Vector3 Up { get; }

        /// <summary>
        /// 变换一个向量，受 rotation 和 scale 影响
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        [NativeMethod]
        public partial Vector3 TransformVector(Vector3 vector);

        /// <summary>
        /// 变换一个方向，只受 rotation 影响
        /// </summary>
        /// <param name="direction"></param>
        /// <returns></returns>
        [NativeMethod]
        public partial Vector3 TransformDirection(Vector3 direction);

        /// <summary>
        /// 变换一个点，受 rotation、scale 和 position 影响
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        [NativeMethod]
        public partial Vector3 TransformPoint(Vector3 point);

        /// <summary>
        /// 逆变换一个向量，受 rotation 和 scale 影响
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        [NativeMethod]
        public partial Vector3 InverseTransformVector(Vector3 vector);

        /// <summary>
        /// 逆变换一个方向，只受 rotation 影响
        /// </summary>
        /// <param name="direction"></param>
        /// <returns></returns>
        [NativeMethod]
        public partial Vector3 InverseTransformDirection(Vector3 direction);

        /// <summary>
        /// 逆变换一个点，受 rotation、scale 和 position 影响
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        [NativeMethod]
        public partial Vector3 InverseTransformPoint(Vector3 point);

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
            child.SetNativeParent(this);
        }

        public void InsertChild(int index, Transform child)
        {
            if (child.m_Parent != null)
            {
                throw new InvalidOperationException("The transform already has a parent.");
            }

            m_Children.Insert(index, child);
            child.m_Parent = this;
            child.SetNativeParent(this);
        }

        public void Detach()
        {
            if (m_Parent == null)
            {
                return;
            }

            m_Parent.m_Children.Remove(this);
            m_Parent = null;
            SetNativeParent(null);
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            foreach (Transform child in m_Children)
            {
                child.m_Parent = this;
                child.SetNativeParent(this);
            }
        }

        [JsonProperty(nameof(LocalRotation))]
        [EditorBrowsable(EditorBrowsableState.Never)]
        private Quaternion LocalRotationSerializationOnly
        {
            get => LocalRotation;
            set => SetLocalRotationWithoutSyncEulerAngles(value);
        }

        [JsonProperty(nameof(LocalEulerAngles))]
        [EditorBrowsable(EditorBrowsableState.Never)]
        private Vector3 LocalEulerAnglesSerializationOnly
        {
            get => LocalEulerAngles;
            set => SetLocalEulerAnglesWithoutSyncRotation(value);
        }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod("SetParent")]
        private partial void SetNativeParent(Transform? parent);

        [NativeMethod]
        private partial void SetLocalRotationWithoutSyncEulerAngles(Quaternion value);

        [NativeMethod]
        private partial void SetLocalEulerAnglesWithoutSyncRotation(Vector3 value);
    }
}
