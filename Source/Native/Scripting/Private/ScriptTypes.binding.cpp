#include "ScriptTypes.h"

using namespace march;

NATIVE_EXPORT(CSharpString) MarshalString(CSharpChar* p, CSharpInt len)
{
    return CSharpString_FromUtf16(p, len);
}

NATIVE_EXPORT(void) UnmarshalString(CSharpString s, CSharpChar** ppOutData, CSharpInt* pOutLen)
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

NATIVE_EXPORT(void) FreeString(CSharpString s)
{
    CSharpString_Free(s);
}

NATIVE_EXPORT(CSharpArray) NewArray(CSharpInt byteCount)
{
    return CSharpArray_New<CSharpByte>(byteCount);
}

NATIVE_EXPORT(CSharpArray) MarshalArray(CSharpByte* p, CSharpInt byteCount)
{
    CSharpArray array = CSharpArray_New<CSharpByte>(byteCount);
    CSharpArray_CopyFrom(array, p);
    return array;
}

NATIVE_EXPORT(void) UnmarshalArray(CSharpArray array, CSharpByte** ppOutData, CSharpInt* pOutByteCount)
{
    if (array == nullptr)
    {
        *ppOutData = nullptr;
        *pOutByteCount = 0;
    }
    else
    {
        *ppOutData = &array->FirstByte;
        *pOutByteCount = array->Length;
    }
}

NATIVE_EXPORT(void) FreeArray(CSharpArray array)
{
    CSharpArray_Free(array);
}
