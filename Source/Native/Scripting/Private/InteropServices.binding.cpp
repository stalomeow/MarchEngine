#include "InteropServices.h"

NATIVE_EXPORT_AUTO MarshalString(cs<cs_char_t*> p, cs_int len)
{
    retcs Utf16ToUtf8(p, len);
}

NATIVE_EXPORT_AUTO UnmarshalString(cs_string s, cs<cs_byte_t**> ppOutData, cs<cs_int_t*> pOutLen)
{
    *ppOutData = reinterpret_cast<cs_byte_t*>(s.data->data());
    *pOutLen = static_cast<cs_int_t>(s.data->size());
}

NATIVE_EXPORT_AUTO FreeString(cs_string s)
{
    cs_string::destroy(s);
}

NATIVE_EXPORT_AUTO NewArray(cs_int byteCount)
{
    cs_array<cs_byte_t> result;
    result.assign(byteCount);
    retcs result;
}

NATIVE_EXPORT_AUTO MarshalArray(cs<cs_byte_t*> p, cs_int byteCount)
{
    cs_array<cs_byte_t> result;
    result.assign(byteCount, p);
    retcs result;
}

NATIVE_EXPORT_AUTO UnmarshalArray(cs_array<cs_byte_t> array, cs<cs_byte_t**> ppOutData, cs<cs_int_t*> pOutByteCount)
{
    *ppOutData = array.begin();
    *pOutByteCount = array.size();
}

NATIVE_EXPORT_AUTO FreeArray(cs_array<cs_byte_t> array)
{
    cs_array<cs_byte_t>::destroy(array);
}
