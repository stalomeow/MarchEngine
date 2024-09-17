#pragma once

namespace march
{
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    enum class GfxCommandListType
    {
        Graphics,
        Compute,
        Copy,
        NumCommandList,
    };

    class GfxCommandList
    {

    };
}
