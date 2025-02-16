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

    class GfxHResultException : public std::exception
    {
    public:
        GfxHResultException(HRESULT hr, const std::string& expr, const std::string& filename, int line);

        char const* what() const override { return m_Message.c_str(); }

    private:
        std::string m_Message;
    };
}

#define CHECK_HR(x) \
{ \
    HRESULT ___hr___ = (x); \
    if (FAILED(___hr___)) { throw ::march::GfxHResultException(___hr___, #x, __FILE__, __LINE__); } \
}
