using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.ComponentModel;
using System.Numerics;
using System.Runtime.Serialization;

namespace March.Core
{
    [CustomComponent(DisableCheckbox = true, HideRemoveButton = true)]
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
            InsertChild(m_Children.Count, child);
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

            SceneManager.CurrentScene.RootGameObjects.Remove(child.gameObject);
        }

        public void Detach(bool worldPositionStays = true)
        {
            if (m_Parent == null)
            {
                return;
            }

            if (worldPositionStays)
            {
                Matrix4x4 matrix = LocalToWorldMatrix;
                Impl();
                LocalToWorldMatrix = matrix;
            }
            else
            {
                Impl();
            }

            void Impl()
            {
                m_Parent.m_Children.Remove(this);
                m_Parent = null;
                SetNativeParent(null);

                SceneManager.CurrentScene.RootGameObjects.Add(gameObject);
            }
        }

        public bool IsChildOf(Transform parent)
        {
            Transform? transform = this;

            while (transform != null)
            {
                // 如果自己就是 parent，那么也返回 true
                if (transform == parent)
                {
                    return true;
                }

                transform = transform.Parent;
            }

            return false;
        }

        public int GetSiblingIndex()
        {
            if (m_Parent == null)
            {
                return SceneManager.CurrentScene.RootGameObjects.IndexOf(gameObject);
            }

            return m_Parent.m_Children.IndexOf(this);
        }

        public void SetSiblingIndex(int index)
        {
            if (m_Parent == null)
            {
                Move(SceneManager.CurrentScene.RootGameObjects, gameObject, index);
            }
            else
            {
                Move(m_Parent.m_Children, this, index);
            }

            static void Move<T>(List<T> list, T item, int newIndex)
            {
                int index = list.IndexOf(item);

                if (index < 0)
                {
                    throw new InvalidOperationException("The item is not in the list.");
                }

                if (index == newIndex)
                {
                    return;
                }

                list.RemoveAt(index);

                if (index < newIndex)
                {
                    newIndex--;
                }

                list.Insert(newIndex, item);
            }
        }

        public void SetParent(Transform? parent, bool worldPositionStays = true)
        {
            if (parent == null)
            {
                Detach(worldPositionStays);
                return;
            }

            if (parent == m_Parent)
            {
                return;
            }

            if (parent.IsChildOf(this))
            {
                throw new InvalidOperationException("Cannot set parent to a child transform.");
            }

            if (worldPositionStays)
            {
                Matrix4x4 matrix = LocalToWorldMatrix;
                Impl(parent);
                LocalToWorldMatrix = matrix;
            }
            else
            {
                Impl(parent);
            }

            void Impl(Transform parent)
            {
                if (m_Parent != null)
                {
                    m_Parent.m_Children.Remove(this);
                }
                else
                {
                    SceneManager.CurrentScene.RootGameObjects.Remove(gameObject);
                }

                m_Parent = parent;
                m_Parent.m_Children.Add(this);
                SetNativeParent(parent);
            }
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
