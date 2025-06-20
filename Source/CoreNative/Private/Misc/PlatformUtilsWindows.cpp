#include "pch.h"

#ifdef PLATFORM_WINDOWS

#include "Engine/Misc/PlatformUtils.h"
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <processthreadsapi.h>
#include <comdef.h>

namespace march
{
    static_assert(std::is_pointer_v<HANDLE>, "HANDLE should be a pointer type");
    static_assert(std::is_pointer_v<HMODULE>, "HMODULE should be a pointer type");
    static_assert(std::is_pointer_v<FARPROC>, "FARPROC should be a pointer type");

    bool PlatformUtils::IsDebuggerPresent()
    {
        return ::IsDebuggerPresent() != FALSE;
    }

    void PlatformUtils::DebugBreak()
    {
        ::DebugBreak();
    }

    std::string PlatformUtils::GetExecutableDirectory()
    {
        WCHAR buf[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);

        if (len == 0 || len == MAX_PATH)
        {
            throw std::runtime_error("Failed to get current executable path: " + Windows::GetLastErrorMessage());
        }

        std::string path = Windows::WideToUtf8(buf);

        // 去掉文件名部分
        if (size_t pos = path.find_last_of('\\'); pos != std::string::npos)
        {
            path.erase(pos, std::string::npos);
        }

        // 规范路径分隔符
        std::replace(path.begin(), path.end(), '\\', '/');

        return path;
    }

    void* PlatformUtils::GetDllHandle(std::string_view dllFileName)
    {
        std::wstring fileName = Windows::Utf8ToWide(dllFileName);
        return LoadLibraryW(fileName.c_str());
    }

    void* PlatformUtils::GetDllExport(void* dllHandle, std::string_view exportName)
    {
        HMODULE hModule = static_cast<HMODULE>(dllHandle);
        std::string procName = Windows::Utf8ToAnsi(exportName);
        return reinterpret_cast<void*>(GetProcAddress(hModule, procName.c_str()));
    }

    void PlatformUtils::FreeDllHandle(void* dllHandle)
    {
        if (FreeLibrary(static_cast<HMODULE>(dllHandle)) == FALSE)
        {
            throw std::runtime_error("Failed to free DLL handle: " + Windows::GetLastErrorMessage());
        }
    }

    void PlatformUtils::SetCurrentThreadName(std::string_view name)
    {
        HANDLE hThread = GetCurrentThread();
        std::wstring description = Windows::Utf8ToWide(name);

        if (HRESULT hr = SetThreadDescription(hThread, description.c_str()); FAILED(hr))
        {
            throw std::runtime_error("Failed to set current thread name: " + Windows::GetHRErrorMessage(hr));
        }
    }

    // https://learn.microsoft.com/en-us/cpp/cpp/char-wchar-t-char16-t-char32-t?view=msvc-170
    // The wchar_t type is an implementation-defined wide character type.
    // In the Microsoft compiler, it represents a 16-bit wide character used to store Unicode encoded as UTF-16LE,
    // the native character type on Windows operating systems.

    static std::string WideToBytes(std::wstring_view s, UINT codePage)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte

        if (s.empty())
        {
            return "";
        }

        if (s.size() > std::numeric_limits<int>::max())
        {
            throw std::out_of_range("String size exceeds maximum limit for conversion");
        }

        int size = static_cast<int>(s.size());
        int len = WideCharToMultiByte(codePage, 0, s.data(), size, nullptr, 0, nullptr, nullptr);

        if (len == 0)
        {
            throw std::runtime_error(PlatformUtils::Windows::GetLastErrorMessage());
        }

        std::string result(len, 0);
        WideCharToMultiByte(codePage, 0, s.data(), size, result.data(), len, nullptr, nullptr);
        return result;
    }

    static std::wstring BytesToWide(std::string_view s, UINT codePage)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar

        if (s.empty())
        {
            return L"";
        }

        if (s.size() > std::numeric_limits<int>::max())
        {
            throw std::out_of_range("String size exceeds maximum limit for conversion");
        }

        int size = static_cast<int>(s.size());
        int len = MultiByteToWideChar(codePage, 0, s.data(), size, nullptr, 0);

        if (len == 0)
        {
            throw std::runtime_error(PlatformUtils::Windows::GetLastErrorMessage());
        }

        std::wstring result(len, 0);
        MultiByteToWideChar(codePage, 0, s.data(), size, result.data(), len);
        return result;
    }

    std::wstring PlatformUtils::Windows::Utf8ToWide(std::string_view s)
    {
        return BytesToWide(s, CP_UTF8);
    }

    std::string PlatformUtils::Windows::Utf8ToAnsi(std::string_view s)
    {
        return WideToAnsi(Utf8ToWide(s));
    }

    std::string PlatformUtils::Windows::WideToUtf8(std::wstring_view s)
    {
        return WideToBytes(s, CP_UTF8);
    }

    std::string PlatformUtils::Windows::WideToAnsi(std::wstring_view s)
    {
        return WideToBytes(s, CP_ACP);
    }

    std::string PlatformUtils::Windows::AnsiToUtf8(std::string_view s)
    {
        return WideToUtf8(AnsiToWide(s));
    }

    std::wstring PlatformUtils::Windows::AnsiToWide(std::string_view s)
    {
        return BytesToWide(s, CP_ACP);
    }

    std::string PlatformUtils::Windows::GetLastErrorMessage()
    {
        // https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code

        DWORD err = ::GetLastError();
        LPWSTR buf = nullptr;

        DWORD size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&buf),
            0,
            nullptr);

        if (size == 0)
        {
            return "Failed to retrieve error message: " + std::to_string(err);
        }

        std::wstring message(buf, size);
        LocalFree(buf);

        return WideToUtf8(message);
    }

    std::string PlatformUtils::Windows::GetHRErrorMessage(HRESULT hr)
    {
        _com_error err(hr);

#ifdef UNICODE
        return WideToUtf8(err.ErrorMessage());
#else
        return AnsiToUtf8(err.ErrorMessage());
#endif
    }
}

#endif
