using March.Core;
using March.Core.Interop;
using March.Editor.IconFont;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Inspector")]
    internal class InspectorWindow : EditorWindow
    {
        private static readonly DrawerCache<InspectorDrawer> s_DrawerCache = new(typeof(InspectorDrawerFor<>));
        private MarchObject? m_LastTarget;
        private InspectorDrawer? m_LastDrawer;

        public InspectorWindow() : base(FontAwesome6.CircleInfo, "Inspector")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            bool isMultiSelection = false;
            MarchObject? target = null;

            if (Selection.Count > 1)
            {
                isMultiSelection = true;
            }
            else
            {
                target = Selection.FirstOrDefault();
            }

            if (target != m_LastTarget)
            {
                m_LastDrawer?.OnDestroy();

                if (target != null && s_DrawerCache.TryGetType(target.GetType(), out Type? drawerType))
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

            if (m_LastDrawer != null)
            {
                m_LastDrawer.Draw();
            }
            else if (isMultiSelection)
            {
                DisplayMessage("Multiple selection is not supported");
            }
            else
            {
                DisplayMessage("Select an object to inspect");
            }

            static void DisplayMessage(StringLike message)
            {
                EditorGUI.Space();
                EditorGUI.CenterText(message);
            }
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
