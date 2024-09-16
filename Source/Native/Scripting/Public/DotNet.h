#pragma once

namespace march
{
    class DotNetEnv
    {
    public:
        void Load();
        void InvokeTickFunc();
        void InvokeInitFunc();
        void InvokeDrawInspectorFunc();
        void InvokeDrawProjectWindowFunc();
        void InvokeDrawHierarchyWindowFunc();

    private:
        typedef void (*VoidDelegate)(void);

        VoidDelegate m_TickFunc = nullptr;
        VoidDelegate m_InitFunc = nullptr;
        VoidDelegate m_DrawInspectorFunc = nullptr;
        VoidDelegate m_DrawProjectWindowFunc = nullptr;
        VoidDelegate m_DrawHierarchyWindowFunc = nullptr;
    };
}
