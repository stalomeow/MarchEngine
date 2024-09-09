namespace DX12Demo.Core
{
    public interface IAssetManagerImpl
    {
        string? GetGuidByPath(string path);

        string? GetPathByGuid(string guid);

        T? Load<T>(string path) where T : EngineObject?;

        T? LoadByGuid<T>(string guid) where T : EngineObject?;
    }

    file class DefaultAssetManagerImpl : IAssetManagerImpl
    {
        public string? GetGuidByPath(string path)
        {
            return null;
        }

        public string? GetPathByGuid(string guid)
        {
            return null;
        }

        public T? Load<T>(string path) where T : EngineObject?
        {
            return null;
        }

        public T? LoadByGuid<T>(string guid) where T : EngineObject?
        {
            return null;
        }
    }

    public static class AssetManager
    {
        public static IAssetManagerImpl Impl { get; set; } = new DefaultAssetManagerImpl();

        public static string? GetGuidByPath(string path) => Impl.GetGuidByPath(path);

        public static string? GetPathByGuid(string guid) => Impl.GetPathByGuid(guid);

        public static EngineObject? Load(string path) => Load<EngineObject>(path);

        public static T? Load<T>(string path) where T : EngineObject? => Impl.Load<T>(path);

        public static EngineObject? LoadByGuid(string guid) => LoadByGuid<EngineObject>(guid);

        public static T? LoadByGuid<T>(string guid) where T : EngineObject? => Impl.LoadByGuid<T>(guid);
    }
}
