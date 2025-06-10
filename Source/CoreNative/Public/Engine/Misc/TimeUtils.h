#pragma once

#include <time.h>
#include <stdexcept>

namespace march
{
    struct TimeUtils
    {
        static tm GetLocalTime(time_t time)
        {
            tm result{};
            if (::localtime_s(&result, &time) != 0)
            {
                throw std::runtime_error("Failed to get local time.");
            }
            return result;
        }

        static tm GetLocalTime()
        {
            return GetLocalTime(::time(nullptr));
        }
    };
}
