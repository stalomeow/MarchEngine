#pragma once

#include "ThirdParty/coreclr_delegates.h"
#include "ThirdParty/hostfxr.h"
#include "ThirdParty/nethost.h"

namespace dx12demo
{
    class DotNetEnv
    {
    public:
        void Load();
        void InvokeMainFunc();
        void InvokeTickFunc();
        void InvokeInitFunc();
        void InvokeDrawInspectorFunc();

    private:
        typedef void (*SetLookUpFnDelegate)(void* fn);
        typedef void (*VoidDelegate)(void);

        SetLookUpFnDelegate m_SetLookUpFn = nullptr;
        VoidDelegate m_TickFunc = nullptr;
        VoidDelegate m_InitFunc = nullptr;
        VoidDelegate m_DrawInspectorFunc = nullptr;
    };
}
