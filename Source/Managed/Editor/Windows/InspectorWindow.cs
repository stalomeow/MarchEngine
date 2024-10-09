using March.Core;

namespace March.Editor.Windows
{
    internal class InspectorWindow : EditorWindow
    {
        private readonly DrawerCache<InspectorDrawer> m_DrawerCache = new(typeof(InspectorDrawerFor<>));
        private MarchObject? m_LastTarget;
        private InspectorDrawer? m_LastDrawer;

        public InspectorWindow() : base("Inspector") { }

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
    }
}
