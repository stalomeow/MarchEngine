#include "InteropServices.h"

NATIVE_EXPORT_AUTO MarshalString(cs<cs_char_t*> p, cs_int len)
{
    return to_cs(cs_wstring::marshal::create(p, len));
}

NATIVE_EXPORT_AUTO UnmarshalString(cs_wstring s, cs<cs_char_t**> ppOutData, cs<cs_int_t*> pOutLen)
{
    *ppOutData = &s.Data->FirstChar;
    *pOutLen = s.Data->Length;
}

NATIVE_EXPORT_AUTO FreeString(cs_wstring s)
{
    cs_wstring::marshal::destroy(s.Data);
}

NATIVE_EXPORT_AUTO NewArray(cs_int byteCount)
{
    return to_cs(cs<cs_byte_t[]>::marshal::create(byteCount));
}

NATIVE_EXPORT_AUTO MarshalArray(cs<cs_byte_t*> p, cs_int byteCount)
{
    cs<cs_byte_t[]> result(byteCount);
    memcpy(&result.Data->FirstByte, p, static_cast<size_t>(byteCount));
    return to_cs(result);
}

NATIVE_EXPORT_AUTO UnmarshalArray(cs<cs_byte_t[]> array, cs<cs_byte_t**> ppOutData, cs<cs_int_t*> pOutByteCount)
{
    *ppOutData = &array.Data->FirstByte;
    *pOutByteCount = array.Data->ByteCount;
}

NATIVE_EXPORT_AUTO FreeArray(cs<cs_byte_t[]> array)
{
    cs<cs_byte_t[]>::marshal::destroy(array.Data);
}
