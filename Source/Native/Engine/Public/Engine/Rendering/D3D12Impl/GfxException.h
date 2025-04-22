#pragma once

#include <exception>
#include <string>
#include <Windows.h>

namespace march
{
    class GfxException : public std::exception
    {
    public:
        explicit GfxException(const std::string& message) : m_Message(message) {}

        char const* what() const override { return m_Message.c_str(); }

    private:
        std::string m_Message;
    };

    void HandleHResultFailureAndTerminateProcess(HRESULT hr, const std::string& expr, const std::string& filename, int line);
}

#define CHECK_HR(x) \
{ \
    HRESULT ___hr___ = (x); \
    if (FAILED(___hr___)) { ::march::HandleHResultFailureAndTerminateProcess(___hr___, #x, __FILE__, __LINE__); } \
}
