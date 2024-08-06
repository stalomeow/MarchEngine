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

    private:
        typedef void (*SetLookUpFnDelegate)(void* fn);
        typedef void (*TickDelegate)(void);
        typedef void (*InitDelegate)(void);

        SetLookUpFnDelegate m_SetLookUpFn = nullptr;
        TickDelegate m_TickFunc = nullptr;
        InitDelegate m_InitFunc = nullptr;
    };
}
