using Newtonsoft.Json;

namespace DX12Demo.Core
{
    [JsonObject(MemberSerialization.OptIn)]
    public abstract class EngineObject
    {
        protected EngineObject() { }
    }
}
