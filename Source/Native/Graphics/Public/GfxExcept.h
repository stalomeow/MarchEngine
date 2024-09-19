#pragma once

#include <Windows.h>
#include <string>
#include <exception>

namespace march
{
    class GfxException : public std::exception
    {
    public:
        explicit GfxException(const std::string& message);

        char const* what() const override;

    private:
        std::string m_Message;
    };

    class GfxHResultException : public std::exception
    {
    public:
        GfxHResultException(HRESULT hr, const std::string& expr, const std::string& filename, int line);

        char const* what() const override;

    private:
        std::string m_Message;
    };
}

#define GFX_HR(x) \
{ \
    HRESULT ___hr___ = (x); \
    if (FAILED(___hr___)) { throw ::march::GfxHResultException(___hr___, #x, __FILE__, __LINE__); } \
}
