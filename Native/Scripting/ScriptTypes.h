#pragma once

#include "Core/StringUtility.h"
#include <stdint.h>
#include <string>
#include <DirectXMath.h>

#define CSHARP_API(returnType) returnType __stdcall
#define CSHARP_TEXT(text) L##text
#define CSHARP_MARSHAL_BOOL(value) static_cast<::dx12demo::CSharpBool>((value) ? 1 : 0)
#define CSHARP_UNMARSHAL_BOOL(value) (value != 0)

namespace dx12demo
{
    // https://learn.microsoft.com/en-us/dotnet/standard/native-interop/best-practices#common-windows-data-types

    typedef uint8_t CSharpByte;
    typedef int32_t CSharpInt;
    typedef uint32_t CSharpUInt;
    typedef float CSharpFloat;
    typedef double CSharpDouble;
    typedef uint8_t CSharpBool;

    static_assert(sizeof(wchar_t) == 2, "wchar_t is not 2 bytes wide.");
    typedef wchar_t CSharpChar;

    extern "C" typedef struct CSharpNativeStringData
    {
        CSharpInt Length;
        CSharpChar FirstChar; // 变长数组。在最后额外追加一个 '\0'，不计入 Length
    }*CSharpString;

    inline std::string CSharpString_ToUtf8(const CSharpChar* p, CSharpInt len)
    {
        return StringUtility::Utf16ToUtf8(p, len);
    }

    inline std::string CSharpString_ToUtf8(const CSharpString self)
    {
        return StringUtility::Utf16ToUtf8(&self->FirstChar, self->Length);
    }

    inline std::wstring CSharpString_ToUtf16(const CSharpString self)
    {
        return std::wstring(&self->FirstChar, self->Length);
    }

    inline std::string CSharpString_ToAnsi(const CSharpString& self)
    {
        return StringUtility::Utf16ToAnsi(&self->FirstChar, self->Length);
    }

    inline CSharpString CSharpString_FromUtf16(const wchar_t* ws, int32_t size)
    {
        CSharpString result = (CSharpString)malloc(sizeof(struct CSharpNativeStringData) + size * sizeof(CSharpChar));

        if (result != nullptr)
        {
            result->Length = static_cast<CSharpInt>(size);
            memcpy(&result->FirstChar, ws, size * sizeof(CSharpChar));
            (&result->FirstChar)[size] = CSHARP_TEXT('\0'); // 追加 '\0'
        }

        return result;
    }

    inline CSharpString CSharpString_FromUtf16(const std::wstring& ws)
    {
        return CSharpString_FromUtf16(ws.c_str(), ws.size());
    }

    inline CSharpString CSharpString_FromUtf8(const char* s, int32_t size)
    {
        return CSharpString_FromUtf16(StringUtility::Utf8ToUtf16(s, size));
    }

    inline CSharpString CSharpString_FromUtf8(const std::string& s)
    {
        return CSharpString_FromUtf16(StringUtility::Utf8ToUtf16(s));
    }

    inline void CSharpString_Free(CSharpString self)
    {
        if (self != nullptr)
        {
            free(self);
        }
    }

    extern "C" typedef struct
    {
        CSharpFloat X, Y;
    } CSharpVector2;

    extern "C" typedef struct
    {
        CSharpFloat X, Y, Z;
    } CSharpVector3;

    extern "C" typedef struct
    {
        CSharpFloat X, Y, Z, W;
    } CSharpVector4, CSharpQuaternion, CSharpColor;

    extern "C" typedef struct
    {
        CSharpFloat M11, M12, M13, M14;
        CSharpFloat M21, M22, M23, M24;
        CSharpFloat M31, M32, M33, M34;
        CSharpFloat M41, M42, M43, M44;
    } CSharpMatrix4x4;

    inline DirectX::XMFLOAT2 ToXMFLOAT2(const CSharpVector2& v)
    {
        return { v.X, v.Y };
    }

    inline DirectX::XMFLOAT3 ToXMFLOAT3(const CSharpVector3& v)
    {
        return { v.X, v.Y, v.Z };
    }

    //inline DirectX::XMFLOAT3 ToXMFLOAT3(const CSharpColor& v)
    //{
    //    return { v.X, v.Y, v.Z };
    //}

    inline DirectX::XMFLOAT4 ToXMFLOAT4(const CSharpVector4& v)
    {
        return { v.X, v.Y, v.Z, v.W };
    }

    //inline DirectX::XMFLOAT4 ToXMFLOAT4(const CSharpQuaternion& v)
    //{
    //    return { v.X, v.Y, v.Z, v.W };
    //}

    inline DirectX::XMFLOAT4X4 ToXMFLOAT4X4(const CSharpMatrix4x4& m)
    {
        return { m.M11, m.M12, m.M13, m.M14,
                 m.M21, m.M22, m.M23, m.M24,
                 m.M31, m.M32, m.M33, m.M34,
                 m.M41, m.M42, m.M43, m.M44 };
    }

    inline CSharpVector2 ToCSharpVector2(const DirectX::XMFLOAT2& v)
    {
        return { v.x, v.y };
    }

    inline CSharpColor ToCSharpColor(const DirectX::XMFLOAT4& v)
    {
        return { v.x, v.y, v.z, v.w };
    }

    inline CSharpVector3 ToCSharpVector3(const DirectX::XMFLOAT3& v)
    {
        return { v.x, v.y, v.z };
    }

    namespace binding
    {
        inline CSHARP_API(CSharpString) MarshalString(CSharpChar* p, CSharpInt len)
        {
            return CSharpString_FromUtf16(p, len);
        }

        inline CSHARP_API(void) UnmarshalString(CSharpString s, CSharpChar** ppOutData, CSharpInt* pOutLen)
        {
            static CSharpChar empty = CSHARP_TEXT('\0');

            if (s == nullptr)
            {
                *ppOutData = &empty; // '\0' 结尾的空字符串
                *pOutLen = 0;
            }
            else
            {
                *ppOutData = &s->FirstChar;
                *pOutLen = s->Length;
            }
        }

        inline CSHARP_API(void) FreeString(CSharpString s)
        {
            CSharpString_Free(s);
        }
    }
}
