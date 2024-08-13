namespace DX12Demo.Core
{
    public interface IAssetProvider
    {
        EngineObject Asset { get; }
    }

    public readonly struct AssetHandle<T>(IAssetProvider? provider) where T : EngineObject
    {
        // 在 Editor 下，某个 Asset 被重新导入后，会创建新的对象，同时旧的对象失效
        // 所以此处不直接保存 Asset 对象，而是保存一个始终不变的 provider
        // 在需要使用 Asset 时，通过 provider 获取到最新的 Asset 对象

        public bool IsValid => provider?.Asset is T;

        public T? Asset => provider?.Asset as T;

        public T MustGetAsset()
        {
            if (provider == null)
            {
                throw new NullReferenceException($"{nameof(provider)} is null");
            }

            if (provider.Asset is not T asset)
            {
                throw new InvalidCastException($"Can not cast {provider.Asset.GetType()} to {typeof(T)}");
            }

            return asset;
        }

        public IAssetProvider? GetProvider() => provider;
    }
}
