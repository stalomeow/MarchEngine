using March.Core;

namespace March.Editor
{
    public interface IGizmoDrawer : IDrawer
    {
        void Draw(Component component, bool isSelected);

        void DrawGUI(Component component, bool isSelected);
    }

    public interface IGizmoDrawerFor<T> : IGizmoDrawer where T : Component
    {
        void IGizmoDrawer.Draw(Component component, bool isSelected) => Draw((T)component, isSelected);

        void IGizmoDrawer.DrawGUI(Component component, bool isSelected) => DrawGUI((T)component, isSelected);

        void Draw(T component, bool isSelected);

        void DrawGUI(T component, bool isSelected);
    }
}
