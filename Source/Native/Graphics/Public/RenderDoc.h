#pragma once

#include <stdint.h>
#include <tuple>
#include <string>

namespace march
{
    class RenderDoc final
    {
    public:
        void Load();
        void CaptureSingleFrame() const;
        uint32_t GetNumCaptures() const;
        std::tuple<int, int, int> GetVersion() const;
        std::string GetLibraryPath() const;

        bool IsLoaded() const { return m_Api != nullptr; }

    private:
        void* m_Api = nullptr;
    };
}
