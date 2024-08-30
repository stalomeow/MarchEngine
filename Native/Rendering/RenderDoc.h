#pragma once

#include "ThirdParty/renderdoc_app.h"
#include <Windows.h>
#include <stdint.h>
#include <tuple>
#include <string>

namespace dx12demo
{
    class RenderDoc final
    {
    public:
        RenderDoc() = default;
        ~RenderDoc() = default;

        void Load();
        void CaptureSingleFrame() const;
        uint32_t GetNumCaptures() const;
        std::tuple<int, int, int> GetVersion() const;
        std::string GetLibraryPath() const;

        bool IsLoaded() const { return m_Api != nullptr; }

    private:
        RENDERDOC_API_1_5_0* m_Api = nullptr;
    };
}
