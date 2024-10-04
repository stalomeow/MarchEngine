#pragma once

#include "DotNetTypeTraits.h"
#include "StringUtility.h"
#include <stdint.h>
#include <string>
#include <DirectXMath.h>

namespace march
{
    template<typename T, typename = void>
    struct cs_marshal;

    template<typename T, typename Marshal = cs_marshal<T>>
    struct cs final
    {
        using marshal = typename Marshal;
        using native_type = typename marshal::native_type;
        using managed_type = typename marshal::managed_type;

        typename managed_type Data;

        cs(const typename native_type& value) : Data(marshal::marshal(value)) {}
        operator typename native_type() const { return marshal::unmarshal(Data); }
        typename managed_type* operator &() const { return &Data; }
    };

    template<typename T>
    struct cs<T*> final
    {
        using marshal = typename cs_marshal<T*>;
        using native_type = T*;
        using managed_type = T*;

        typename managed_type Data;

        cs(typename native_type value) : Data(value) {}
        operator typename native_type() const { return Data; }
        typename managed_type* operator &() const { return &Data; }
        native_type operator ->() const { return Data; }
        std::remove_pointer_t<native_type>& operator *() const { return *Data; }
        bool operator ==(std::nullptr_t) const { return Data == nullptr; }
    };

    template<>
    struct cs<void> final
    {
        using marshal = typename cs_marshal<void>;
        using native_type = void;
        using managed_type = void;
    };

    template<typename T>
    static auto to_cs(const T& value)
    {
        return cs<T>(value).Data;
    }

    template<typename T>
    static auto to_cs<cs<T>>(const cs<T>& value)
    {
        return value.Data;
    }

    template<>
    struct cs_marshal<void>
    {
        using native_type = void;
        using managed_type = void;

        static constexpr managed_type marshal(native_type) {}
        static constexpr native_type unmarshal(managed_type) {}
    };

    template<typename T>
    struct cs_marshal<T, std::enable_if_t<is_blittable<T>::value>>
    {
        using native_type = T;
        using managed_type = T;

        static constexpr managed_type marshal(const native_type& value) { return value; }
        static constexpr native_type unmarshal(const managed_type& value) { return value; }
    };

    template<typename T>
    struct cs_marshal<T, std::enable_if_t<std::is_enum<T>::value>>
    {
        using native_type = T;
        using managed_type = cs<int32_t>;

        static constexpr managed_type marshal(const native_type& value)
        {
            return static_cast<int32_t>(value);
        }

        static constexpr native_type unmarshal(const managed_type& value)
        {
            return static_cast<native_type>(value.Data);
        }
    };

    struct cs_string_data
    {
        cs<int32_t> Length;
        cs<wchar_t> FirstChar; // 变长数组。在最后额外追加一个 '\0'，不计入 Length
    };

    template<>
    struct cs_marshal<std::wstring>
    {
        using native_type = std::wstring;
        using managed_type = cs_string_data*;

        static managed_type create(const wchar_t* pData, int32_t length)
        {
            auto result = (managed_type)malloc(sizeof(cs_string_data) + static_cast<size_t>(length) * sizeof(cs<wchar_t>));
            result->Length = length;
            memcpy(&result->FirstChar, pData, static_cast<size_t>(length) * sizeof(cs<wchar_t>));
            (&result->FirstChar)[length] = L'\0'; // 追加 '\0'
            return result;
        }

        static void destroy(const managed_type& value)
        {
            free(value);
        }

        static managed_type marshal(const native_type& value)
        {
            return create(value.data(), static_cast<int32_t>(value.size()));
        }

        static native_type unmarshal(const managed_type& value)
        {
            const wchar_t* firstChar = reinterpret_cast<const wchar_t*>(&value->FirstChar);
            size_t size = static_cast<size_t>(value->Length);
            return std::wstring(firstChar, size);
        }
    };

    template<>
    struct cs_marshal<std::string>
    {
        using native_type = std::string;
        using managed_type = cs_string_data*;

        static managed_type marshal(const native_type& value)
        {
            return cs_marshal<std::wstring>::marshal(StringUtility::Utf8ToUtf16(value));
        }

        static native_type unmarshal(const managed_type& value)
        {
            const wchar_t* firstChar = reinterpret_cast<const wchar_t*>(&value->FirstChar);
            int32_t size = static_cast<size_t>(value->Length);
            return StringUtility::Utf16ToUtf8(firstChar, size);
        }
    };

    struct cs_array_data
    {
        cs<int32_t> ByteCount;
        cs<uint8_t> FirstByte;
    };

    template<typename T>
    struct cs_marshal<T[]>
    {
        using managed_type = cs_array_data*;

        static managed_type create(int32_t length)
        {
            size_t byteCount = static_cast<size_t>(length) * sizeof(T);
            auto result = (managed_type)malloc(sizeof(cs<int32_t>) + byteCount);
            result->ByteCount = static_cast<int32_t>(byteCount);
            return result;
        }

        static void destroy(const managed_type& value)
        {
            free(value);
        }
    };

    template <typename T>
    struct cs<T[]> final
    {
        using marshal = cs_marshal<T[]>;
        using managed_type = typename marshal::managed_type;

        typename marshal::managed_type Data;

        cs() : Data(nullptr) {}
        cs(int32_t length) : Data(marshal::create(length)) {}

        void recreate(int32_t length)
        {
            if (Data != nullptr)
            {
                marshal::destroy(Data);
            }

            Data = marshal::create(length);
        }

        int32_t length() const
        {
            return static_cast<int32_t>(Data->ByteCount / sizeof(T));
        }

        T& operator [](int32_t index) const
        {
            return reinterpret_cast<T*>(&Data->FirstByte)[index];
        }
    };

    using cs_void = cs<void>;
    using cs_byte = cs<uint8_t>;
    using cs_sbyte = cs<int8_t>;
    using cs_ushort = cs<uint16_t>;
    using cs_short = cs<int16_t>;
    using cs_uint = cs<uint32_t>;
    using cs_int = cs<int32_t>;
    using cs_ulong = cs<uint64_t>;
    using cs_long = cs<int64_t>;
    using cs_char = cs<wchar_t>;
    using cs_float = cs<float>;
    using cs_double = cs<double>;
    using cs_bool = cs<bool>;
    using cs_wstring = cs<std::wstring>;
    using cs_string = cs<std::string>;
    using cs_vector2 = cs<DirectX::XMFLOAT2>;
    using cs_vector3 = cs<DirectX::XMFLOAT3>;
    using cs_vector4 = cs<DirectX::XMFLOAT4>;
    using cs_matrix4x4 = cs<DirectX::XMFLOAT4X4>;
    using cs_quaternion = cs<DirectX::XMFLOAT4>;
    using cs_color = cs<DirectX::XMFLOAT4>;

    using cs_void_t = cs_void::managed_type;
    using cs_byte_t = cs_byte::managed_type;
    using cs_sbyte_t = cs_sbyte::managed_type;
    using cs_ushort_t = cs_ushort::managed_type;
    using cs_short_t = cs_short::managed_type;
    using cs_uint_t = cs_uint::managed_type;
    using cs_int_t = cs_int::managed_type;
    using cs_ulong_t = cs_ulong::managed_type;
    using cs_long_t = cs_long::managed_type;
    using cs_char_t = cs_char::managed_type;
    using cs_float_t = cs_float::managed_type;
    using cs_double_t = cs_double::managed_type;
    using cs_bool_t = cs_bool::managed_type;
    using cs_wstring_t = cs_wstring::managed_type;
    using cs_string_t = cs_string::managed_type;
    using cs_vector2_t = cs_vector2::managed_type;
    using cs_vector3_t = cs_vector3::managed_type;
    using cs_vector4_t = cs_vector4::managed_type;
    using cs_matrix4x4_t = cs_matrix4x4::managed_type;
    using cs_quaternion_t = cs_quaternion::managed_type;
    using cs_color_t = cs_color::managed_type;
}
