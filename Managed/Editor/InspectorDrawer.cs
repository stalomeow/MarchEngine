using DX12Demo.Core;

namespace DX12Demo.Editor
{
    public abstract class InspectorDrawer : IDrawer
    {
        public EngineObject? Target { get; internal set; }

        public virtual void OnCreate() { }

        public virtual void OnDestroy() { }

        public abstract void Draw();
    }

    public abstract class InspectorDrawerFor<T> : InspectorDrawer where T : EngineObject
    {
        public new T Target => (T)base.Target!;
    }
}
