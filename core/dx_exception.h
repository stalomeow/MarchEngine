#pragma once

#include <Windows.h>
#include <string>

namespace dx12demo
{
    class DxException final
    {
    public:
        DxException(HRESULT hr, const std::wstring& expr, const std::wstring& filename, int line);

        std::wstring ToString() const;

    private:
        HRESULT m_ErrorCode;
        std::wstring m_Expression;
        std::wstring m_Filename;
        int m_Line;
    };
}

#define THROW_IF_FAILED(x) \
{ \
    HRESULT hr = (x); \
    if (FAILED(hr)) { throw dx12demo::DxException(hr, L#x, __FILEW__, __LINE__); } \
}
