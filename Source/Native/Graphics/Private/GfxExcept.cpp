#include "GfxExcept.h"
#include "StringUtility.h"
#include <comdef.h>

namespace march
{
    GfxException::GfxException(const std::string& message) : m_Message(message)
    {
    }

    char const* GfxException::what() const
    {
        return m_Message.c_str();
    }

    GfxHResultException::GfxHResultException(HRESULT hr, const std::string& expr, const std::string& filename, int line)
    {
        _com_error err(hr);
        m_Message = expr + "\nerror: " + StringUtility::Utf16ToUtf8(err.ErrorMessage())
            + "\nfile: " + filename + "\nline: " + std::to_string(line);
    }

    char const* GfxHResultException::what() const
    {
        return m_Message.c_str();
    }
}
