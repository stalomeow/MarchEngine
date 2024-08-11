using DX12Demo.Core;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor.Windows
{
    internal static class InspectorWindow
    {
        private static readonly DrawerCache<IInspectorDrawer> s_DrawerCache = new(typeof(IInspectorDrawerFor<>));

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            if (Selection.Active == null)
            {
                return;
            }

            EngineObject target = Selection.Active;

            if (!s_DrawerCache.TryGet(target.GetType(), out IInspectorDrawer? drawer))
            {
                return;
            }

            drawer.Draw(target);
        }
    }
}
