#pragma once

#include <utility>
#include <stdexcept>

namespace march
{
    template <typename T, size_t _Capacity>
    class InlineArray
    {
    public:
        using ElementType = T;
        static constexpr size_t Capacity = _Capacity;

        size_t Num() const { return m_Num; }

        void Append(const ElementType& value)
        {
            if (m_Num >= Capacity)
            {
                throw std::runtime_error("InlineArray is full");
            }

            m_Data[m_Num++] = value;
        }

        void Append(ElementType&& value)
        {
            if (m_Num >= Capacity)
            {
                throw std::runtime_error("InlineArray is full");
            }

            m_Data[m_Num++] = std::move(value);
        }

        const ElementType& operator[](size_t index) const
        {
            if (index >= m_Num)
            {
                throw std::out_of_range("InlineArray index out of range");
            }

            return m_Data[index];
        }

        ElementType& operator[](size_t index)
        {
            if (index >= m_Num)
            {
                throw std::out_of_range("InlineArray index out of range");
            }

            return m_Data[index];
        }

    private:
        ElementType m_Data[Capacity]{};
        size_t m_Num = 0;
    };
}
