#include "DxException.h"
#include <comdef.h>

namespace march
{
    DxException::DxException(HRESULT hr, const std::wstring& expr, const std::wstring& filename, int line)
        : m_ErrorCode(hr), m_Expression(expr), m_Filename(filename), m_Line(line)
    {

    }

    std::wstring DxException::ToString() const
    {
        _com_error err(m_ErrorCode);
        std::wstring msg = err.ErrorMessage();
        return m_Expression + L"\nerror: " + msg + L"\nfile: " + m_Filename + L"\nline: " + std::to_wstring(m_Line);
    }
}
