namespace March.Core
{
    public interface IAssetManagerImpl
    {
        string? GetGuidByPath(string path);

        string? GetPathByGuid(string guid);

        T? Load<T>(string path) where T : MarchObject?;

        T? LoadByGuid<T>(string guid) where T : MarchObject?;
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

        public T? Load<T>(string path) where T : MarchObject?
        {
            return null;
        }

        public T? LoadByGuid<T>(string guid) where T : MarchObject?
        {
            return null;
        }
    }

    public static class AssetManager
    {
        public static IAssetManagerImpl Impl { get; set; } = new DefaultAssetManagerImpl();

        public static string? GetGuidByPath(string path) => Impl.GetGuidByPath(path);

        public static string? GetPathByGuid(string guid) => Impl.GetPathByGuid(guid);

        public static MarchObject? Load(string path) => Load<MarchObject>(path);

        public static T? Load<T>(string path) where T : MarchObject? => Impl.Load<T>(path);

        public static MarchObject? LoadByGuid(string guid) => LoadByGuid<MarchObject>(guid);

        public static T? LoadByGuid<T>(string guid) where T : MarchObject? => Impl.LoadByGuid<T>(guid);
    }
}
