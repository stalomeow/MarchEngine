namespace DX12Demo.Core
{
    public interface IAssetManagerImpl
    {
        T? Load<T>(string path) where T : EngineObject;
    }

    file class DefaultAssetManagerImpl : IAssetManagerImpl
    {
        public T? Load<T>(string path) where T : EngineObject
        {
            return null;
        }
    }

    public static class AssetManager
    {
        public static IAssetManagerImpl Impl { get; set; } = new DefaultAssetManagerImpl();

        public static EngineObject? Load(string path) => Load<EngineObject>(path);

        public static T? Load<T>(string path) where T : EngineObject => Impl.Load<T>(path);
    }
}
