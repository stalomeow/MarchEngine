using Newtonsoft.Json;

namespace DX12Demo.Core
{
    [JsonObject(MemberSerialization.OptIn)]
    public abstract class EngineObject
    {
        /// <summary>
        /// 全局唯一 id。如果为 <c>null</c>，则表示该对象未被持久化
        /// </summary>
        [JsonIgnore]
        public string? PersistentGuid { get; internal set; }

        protected EngineObject() { }
    }

    public abstract class NativeEngineObject(nint nativePtr) : EngineObject, IDisposable
    {
        [JsonIgnore]
        private bool m_IsDisposed;

        [JsonIgnore]
        public nint NativePtr { get; } = nativePtr;

        ~NativeEngineObject() => DisposeImpl(disposing: false);

        public void Dispose()
        {
            DisposeImpl(disposing: true);
            GC.SuppressFinalize(this);
        }

        private void DisposeImpl(bool disposing)
        {
            if (!m_IsDisposed)
            {
                Dispose(disposing);
                m_IsDisposed = true;
            }
        }

        protected abstract void Dispose(bool disposing);
    }
}
