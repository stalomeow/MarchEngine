#pragma once

#include "DotNetTypeTraits.h"
#include "Debug.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <stdint.h>
#include <string>
#include <utility>
#include <stdexcept>

namespace march
{
    template<typename T, typename = void>
    struct cs;

    // blittable 类型
    template<typename T>
    struct cs<T, std::enable_if_t<is_blittable_v<T>>> final
    {
        using managed_type = typename T;

        T data;

        constexpr void assign(const T& value) { data = value; }
        constexpr void assign(T&& value) { data = std::move(value); }

        constexpr T* operator &() { return &data; }
        constexpr const T* operator &() const { return &data; }
        constexpr operator T () const { return data; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr U operator ->() const { return data; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr U operator +(ptrdiff_t offset) const { return data + offset; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr U operator -(ptrdiff_t offset) const { return data - offset; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr std::remove_pointer_t<U>& operator *() const { return *data; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr std::remove_pointer_t<U>& operator [](ptrdiff_t offset) const { return data[offset]; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr bool operator ==(std::nullptr_t) const { return data == nullptr; }
    };

    // enum 类型
    template<typename T>
    struct cs<T, std::enable_if_t<std::is_enum_v<T>>> final
    {
        using managed_type = int32_t;

        managed_type data;

        constexpr void assign(const T& value) { data = static_cast<managed_type>(value); }
        constexpr void assign(T&& value) { data = static_cast<managed_type>(value); }

        constexpr operator T () const { return static_cast<T>(data); }
    };

    template<>
    struct cs<DirectX::XMFLOAT2> final
    {
        using managed_type = struct
        {
            float x;
            float y;
        };

        managed_type data;

        constexpr void assign(const DirectX::XMFLOAT2& value)
        {
            data.x = value.x;
            data.y = value.y;
        }

        constexpr void assign(DirectX::XMFLOAT2&& value)
        {
            data.x = value.x;
            data.y = value.y;
        }

        constexpr operator DirectX::XMFLOAT2() const
        {
            return { data.x, data.y };
        }
    };

    template<>
    struct cs<DirectX::XMFLOAT3> final
    {
        using managed_type = struct
        {
            float x;
            float y;
            float z;
        };

        managed_type data;

        constexpr void assign(const DirectX::XMFLOAT3& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
        }

        constexpr void assign(DirectX::XMFLOAT3&& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
        }

        constexpr operator DirectX::XMFLOAT3() const
        {
            return { data.x, data.y, data.z };
        }
    };

    template<>
    struct cs<DirectX::XMFLOAT4> final
    {
        using managed_type = struct
        {
            float x;
            float y;
            float z;
            float w;
        };

        managed_type data;

        constexpr void assign(const DirectX::XMFLOAT4& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
            data.w = value.w;
        }

        constexpr void assign(DirectX::XMFLOAT4&& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
            data.w = value.w;
        }

        constexpr operator DirectX::XMFLOAT4() const
        {
            return { data.x, data.y, data.z, data.w };
        }
    };

    template<>
    struct cs<DirectX::XMFLOAT4X4> final
    {
        using managed_type = struct
        {
            float m[4][4];
        };

        managed_type data;

        void assign(const DirectX::XMFLOAT4X4& value)
        {
            copy_float4x4(data.m, value.m);
        }

        void assign(DirectX::XMFLOAT4X4&& value)
        {
            copy_float4x4(data.m, value.m);
        }

        operator DirectX::XMFLOAT4X4() const
        {
            DirectX::XMFLOAT4X4 result{};
            copy_float4x4(result.m, data.m);
            return result;
        }

        static void copy_float4x4(float dst[4][4], const float src[4][4])
        {
            float* d = reinterpret_cast<float*>(dst);
            const float* s = reinterpret_cast<const float*>(src);
            std::copy_n(s, 16, d);
        }
    };

    template<>
    struct cs<DirectX::BoundingBox> final
    {
        using managed_type = struct
        {
            cs<DirectX::XMFLOAT3> center;
            cs<DirectX::XMFLOAT3> extents;
        };

        managed_type data;

        constexpr void assign(const DirectX::BoundingBox& value)
        {
            data.center.assign(value.Center);
            data.extents.assign(value.Extents);
        }

        constexpr void assign(DirectX::BoundingBox&& value)
        {
            data.center.assign(std::move(value.Center));
            data.extents.assign(std::move(value.Extents));
        }

        constexpr operator DirectX::BoundingBox() const
        {
            return DirectX::BoundingBox(data.center, data.extents);
        }
    };

    template<>
    struct cs<void> final
    {
        using managed_type = void;

        void assign(void) {}
    };

    template<>
    struct cs<std::string> final
    {
        using managed_type = std::string*;

        managed_type data;

        // 只能在当前对象未持有数据时调用
        void assign(const std::string& value)
        {
            data = create_data(value);
        }

        // 只能在当前对象未持有数据时调用
        void assign(std::string&& value)
        {
            data = create_data(std::move(value));
        }

        operator const std::string& () const { return *data; }

        std::string&& move() { return std::move(*data); }

        const char* c_str() { return data->c_str(); }

        static managed_type create_data()
        {
            return DBG_NEW typename std::remove_pointer_t<managed_type>();
        }

        static managed_type create_data(const std::string& value)
        {
            return DBG_NEW typename std::remove_pointer_t<managed_type>(value);
        }

        static managed_type create_data(std::string&& value)
        {
            return DBG_NEW typename std::remove_pointer_t<managed_type>(value);
        }

        static void destroy(cs value)
        {
            delete value.data;
        }
    };

    template<typename T>
    struct cs<T[]> final
    {
        using managed_type = struct
        {
            int32_t count;
            uint8_t b; // uint8_t b[];
        }*;

        managed_type data;

        int32_t size() const { return static_cast<int32_t>(data->count / sizeof(T)); }
        T* begin() const { return reinterpret_cast<T*>(&data->b); }
        T* end() const { return reinterpret_cast<T*>(&data->b + data->count); }

        void copy_from(const T* p)
        {
            std::copy(p, p + size(), begin());
        }

        // 只能在当前对象未持有数据时调用
        void assign(int32_t length)
        {
            data = create_data(length);
        }

        // 只能在当前对象未持有数据时调用
        void assign(int32_t length, const T* p)
        {
            assign(length);
            copy_from(p);
        }

        T& operator [](int32_t index) const
        {
            if (index < 0 || index >= size())
            {
                throw std::out_of_range("Index out of range");
            }

            return reinterpret_cast<T*>(&data->b)[index];
        }

        static managed_type create_data(int32_t length)
        {
            int32_t byteCount = sizeof(T) * length;
            auto result = static_cast<managed_type>(malloc(sizeof(int32_t) + static_cast<size_t>(byteCount)));

            if (result != nullptr)
            {
                result->count = byteCount;
            }

            return result;
        }

        static void destroy(cs value)
        {
            if (value.data != nullptr)
            {
                free(value.data);
            }
        }
    };

    template<typename T>
    constexpr bool is_cs_type_v = false;

    template<typename T>
    constexpr bool is_cs_type_v<cs<T>> = true;

    template<typename T>
    struct is_cs_type : std::bool_constant<is_cs_type_v<T>> {};

    template<typename T>
    constexpr bool is_valid_cs_type_v = is_cs_type_v<T> && is_blittable_v<T>;

    template<typename T>
    struct is_valid_cs_type : std::bool_constant<is_valid_cs_type_v<T>> {};

    struct cs_t_convert final
    {
        template<typename T>
        constexpr auto operator <<(T&& value)
        {
            using type = typename std::remove_cv_t<std::remove_reference_t<T>>;

            if constexpr (is_cs_type_v<type>)
            {
                return value.data;
            }
            else
            {
                cs<type> result{};
                result.assign(std::forward<T>(value));
                return result.data;
            }
        }
    };

    template<typename T, typename = void, typename = void>
    struct cs_defer_destroy;

    template<typename T>
    struct cs_defer_destroy<T, typename std::enable_if_t<is_cs_type_v<T>>, typename std::void_t<decltype(&T::destroy)>> final
    {
        T v;

        cs_defer_destroy() : v() {}
        ~cs_defer_destroy() { T::destroy(v); }

        cs_defer_destroy(const cs_defer_destroy&) = delete;
        cs_defer_destroy& operator=(const cs_defer_destroy&) = delete;
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
    using cs_nint = cs<void*>;
    using cs_string = cs<std::string>;
    using cs_vec2 = cs<DirectX::XMFLOAT2>;
    using cs_vec3 = cs<DirectX::XMFLOAT3>;
    using cs_vec4 = cs<DirectX::XMFLOAT4>;
    using cs_mat4 = cs<DirectX::XMFLOAT4X4>;
    using cs_quat = cs<DirectX::XMFLOAT4>;
    using cs_color = cs<DirectX::XMFLOAT4>;
    using cs_bounds = cs<DirectX::BoundingBox>;

    template<typename T>
    using cs_array = cs<T[]>;

    template<typename T>
    using cs_ptr = cs<T*>;

    template<typename T>
    using cs_pptr = cs<cs<T*>*>;

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
    using cs_nint_t = cs_nint::managed_type;
    using cs_string_t = cs_string::managed_type;
    using cs_vec2_t = cs_vec2::managed_type;
    using cs_vec3_t = cs_vec3::managed_type;
    using cs_vec4_t = cs_vec4::managed_type;
    using cs_mat4_t = cs_mat4::managed_type;
    using cs_quat_t = cs_quat::managed_type;
    using cs_color_t = cs_color::managed_type;
    using cs_bounds_t = cs_bounds::managed_type;

    template<typename T>
    using cs_array_t = typename cs_array<T>::managed_type;

    template<typename T>
    using cs_ptr_t = typename cs_ptr<T>::managed_type;

    template<typename T>
    using cs_pptr_t = typename cs_pptr<T>::managed_type;

    template<typename T>
    using cs_t = typename cs<T>::managed_type;

    static_assert(is_valid_cs_type_v<cs_void>);
    static_assert(is_valid_cs_type_v<cs_byte>);
    static_assert(is_valid_cs_type_v<cs_sbyte>);
    static_assert(is_valid_cs_type_v<cs_ushort>);
    static_assert(is_valid_cs_type_v<cs_short>);
    static_assert(is_valid_cs_type_v<cs_uint>);
    static_assert(is_valid_cs_type_v<cs_int>);
    static_assert(is_valid_cs_type_v<cs_ulong>);
    static_assert(is_valid_cs_type_v<cs_long>);
    static_assert(is_valid_cs_type_v<cs_char>);
    static_assert(is_valid_cs_type_v<cs_float>);
    static_assert(is_valid_cs_type_v<cs_double>);
    static_assert(is_valid_cs_type_v<cs_bool>);
    static_assert(is_valid_cs_type_v<cs_nint>);
    static_assert(is_valid_cs_type_v<cs_string>);
    static_assert(is_valid_cs_type_v<cs_vec2>);
    static_assert(is_valid_cs_type_v<cs_vec3>);
    static_assert(is_valid_cs_type_v<cs_vec4>);
    static_assert(is_valid_cs_type_v<cs_mat4>);
    static_assert(is_valid_cs_type_v<cs_quat>);
    static_assert(is_valid_cs_type_v<cs_color>);
    static_assert(is_valid_cs_type_v<cs_bounds>);
}
