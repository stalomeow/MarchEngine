using DX12Demo.Core;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor
{
    public interface IComponentDrawer : IDrawer
    {
        bool Draw(Component component, JsonObjectContract contract);
    }

    public interface IComponentDrawerFor<T> : IComponentDrawer where T : Component { }
}
