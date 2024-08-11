using DX12Demo.Core;

namespace DX12Demo.Editor
{
    public interface IInspectorDrawer : IDrawer
    {
        void Draw(EngineObject target);
    }

    public interface IInspectorDrawerFor<T> : IInspectorDrawer where T : EngineObject
    {
        void IInspectorDrawer.Draw(EngineObject target) => Draw((T)target);

        void Draw(T target);
    }
}
