#pragma once

#include <string>

namespace march
{
    enum class PathStyle
    {
        Unix,
        Windows,
    };

    namespace PathHelper
    {
        // 路径最后没有斜杠
        std::wstring GetWorkingDirectoryUtf16(PathStyle style = PathStyle::Windows);

        // 路径最后没有斜杠
        std::string GetWorkingDirectoryUtf8(PathStyle style = PathStyle::Windows);
    }
}
