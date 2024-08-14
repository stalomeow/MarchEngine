using DX12Demo.Core;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor.Windows
{
    internal static class InspectorWindow
    {
        private static readonly DrawerCache<InspectorDrawer> s_DrawerCache = new(typeof(InspectorDrawerFor<>));
        private static EngineObject? s_LastTarget;
        private static InspectorDrawer? s_LastDrawer;

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            EngineObject? target = Selection.Active;

            if (target != s_LastTarget)
            {
                s_LastDrawer?.OnDestroy();

                if (target != null && s_DrawerCache.TryGetType(target.GetType(), out Type? drawerType))
                {
                    s_LastDrawer = (InspectorDrawer)Activator.CreateInstance(drawerType)!;
                    s_LastDrawer.Target = target;
                    s_LastDrawer.OnCreate();
                }
                else
                {
                    s_LastDrawer = null;
                }

                s_LastTarget = target;
            }

            s_LastDrawer?.Draw();
        }
    }
}
