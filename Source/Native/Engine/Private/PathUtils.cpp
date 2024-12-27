#include "pch.h"
#include "Engine/PathUtils.h"
#include "Engine/StringUtils.h"
#include <Windows.h>
#include <algorithm>

namespace march::PathUtils
{
    std::wstring GetWorkingDirectoryUtf16(PathStyle style)
    {
        wchar_t buf[MAX_PATH];
        GetModuleFileNameW(NULL, buf, MAX_PATH);

        std::wstring path(buf);

        if (size_t pos = path.find_last_of(L"\\"); pos != std::wstring::npos)
        {
            path.erase(pos, std::wstring::npos);
        }

        if (style == PathStyle::Unix)
        {
            std::replace(path.begin(), path.end(), L'\\', L'/');
        }

        return path;
    }

    std::string GetWorkingDirectoryUtf8(PathStyle style)
    {
        return StringUtils::Utf16ToUtf8(GetWorkingDirectoryUtf16(style));
    }
}
