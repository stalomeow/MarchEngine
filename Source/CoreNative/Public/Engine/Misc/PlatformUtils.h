#pragma once

#include <string>
#include <string_view>
#include <stdint.h>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace march
{
    struct PlatformUtils
    {
        static bool IsDebuggerPresent();
        static void DebugBreak();
        static void DebugOutput(std::string_view s);

        static std::string GetExecutableDirectory();

        static void* GetDllHandle(std::string_view dllFileName);
        static void* GetDllExport(void* dllHandle, std::string_view exportName);
        static void FreeDllHandle(void* dllHandle);

        static void* GetCurrentThreadHandle();
        static void SetThreadName(void* threadHandle, std::string_view name);

        static void* GetCurrentProcessHandle();
        static size_t GetProcessPhysicalMemorySizeInBytes(void* processHandle);

#ifdef PLATFORM_WINDOWS
        struct Windows
        {
            static std::wstring Utf8ToWide(std::string_view s);
            static std::string Utf8ToAnsi(std::string_view s);
            static std::string WideToUtf8(std::wstring_view s);
            static std::string WideToAnsi(std::wstring_view s);
            static std::string AnsiToUtf8(std::string_view s);
            static std::wstring AnsiToWide(std::string_view s);

            static std::string GetLastErrorMessage();
            static std::string GetHRErrorMessage(HRESULT hr);
        };
#endif
    };
}
