using March.Core;

namespace March.Editor
{
    public abstract class InspectorDrawer : IDrawer
    {
        public MarchObject? Target { get; internal set; }

        public virtual void OnCreate() { }

        public virtual void OnDestroy() { }

        public abstract void Draw();
    }

    public abstract class InspectorDrawerFor<T> : InspectorDrawer where T : MarchObject
    {
        public new T Target => (T)base.Target!;
    }
}
