using March.Core;
using Newtonsoft.Json.Serialization;

namespace March.Editor
{
    public interface IComponentDrawer : IDrawer
    {
        bool Draw(Component component, JsonObjectContract contract);
    }

    public interface IComponentDrawerFor<T> : IComponentDrawer where T : Component
    {
        bool IComponentDrawer.Draw(Component component, JsonObjectContract contract) => Draw((T)component, contract);

        bool Draw(T component, JsonObjectContract contract);
    }
}
