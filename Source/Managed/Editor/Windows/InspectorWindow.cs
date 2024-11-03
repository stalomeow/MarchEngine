using March.Core;
using March.Core.IconFont;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Inspector")]
    internal class InspectorWindow : EditorWindow
    {
        private readonly DrawerCache<InspectorDrawer> m_DrawerCache = new(typeof(InspectorDrawerFor<>));
        private MarchObject? m_LastTarget;
        private InspectorDrawer? m_LastDrawer;

        public InspectorWindow() : base(FontAwesome6.CircleInfo, "Inspector")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            MarchObject? target = Selection.Active;

            if (target != m_LastTarget)
            {
                m_LastDrawer?.OnDestroy();

                if (target != null && m_DrawerCache.TryGetType(target.GetType(), out Type? drawerType))
                {
                    m_LastDrawer = (InspectorDrawer)Activator.CreateInstance(drawerType)!;
                    m_LastDrawer.Target = target;
                    m_LastDrawer.OnCreate();
                }
                else
                {
                    m_LastDrawer = null;
                }

                m_LastTarget = target;
            }

            m_LastDrawer?.Draw();
        }
        protected override void OnClose()
        {
            if (m_LastDrawer != null)
            {
                m_LastDrawer.OnDestroy();
                m_LastDrawer = null;
            }

            base.OnClose();
        }
    }
}
