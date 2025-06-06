using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    /// <summary>
    /// 引擎对象的基类
    /// </summary>
    [JsonObject(MemberSerialization.OptIn)]
    public abstract class MarchObject
    {
        /// <summary>
        /// 全局唯一 id。如果为 <c>null</c>，则表示该对象未被持久化
        /// </summary>
        [JsonIgnore]
        public string? PersistentGuid { get; internal set; }

        [JsonIgnore]
        public bool IsPersistent => PersistentGuid != null;

        protected MarchObject() { }

        public static T Instantiate<T>(T original) where T : MarchObject
        {
            string json = PersistentManager.SaveAsString(original);
            return PersistentManager.LoadFromString<T>(json);
        }
    }

    public abstract partial class NativeMarchObject(nint nativePtr) : MarchObject, IDisposable
    {
        [JsonIgnore]
        private bool m_IsDisposed;

        [JsonIgnore]
        public nint NativePtr { get; private set; } = nativePtr;

        ~NativeMarchObject() => Dispose(disposing: false);

        public void Dispose()
        {
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!m_IsDisposed)
            {
                OnDispose(disposing);
                DeleteNativeObject();
                NativePtr = nint.Zero;
                m_IsDisposed = true;
            }
        }

        protected virtual void OnDispose(bool disposing) { }

        [NativeMethod("Delete")]
        private partial void DeleteNativeObject();
    }
}
