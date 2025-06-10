namespace March.Core
{
    public interface ISceneManagerImpl
    {
        Scene CurrentScene { get; }

        void LoadScene(string path);
    }

    file class DefaultSceneManagerImpl : ISceneManagerImpl
    {
        public Scene CurrentScene { get; } = new();

        public void LoadScene(string path) { }
    }

    public static class SceneManager
    {
        internal static ISceneManagerImpl Impl { get; set; } = new DefaultSceneManagerImpl();

        public static Scene CurrentScene => Impl.CurrentScene;

        public static void LoadScene(string path) => Impl.LoadScene(path);
    }
}
