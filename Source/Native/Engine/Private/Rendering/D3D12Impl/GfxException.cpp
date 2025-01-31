#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Misc/StringUtils.h"
#include <comdef.h>

namespace march
{
    GfxHResultException::GfxHResultException(HRESULT hr, const std::string& expr, const std::string& filename, int line)
    {
        _com_error err(hr);
        m_Message = expr + "\nerror: " + StringUtils::Utf16ToUtf8(err.ErrorMessage())
            + "\nfile: " + filename + "\nline: " + std::to_string(line);
    }
}
