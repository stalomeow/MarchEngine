#pragma once

#include "Rendering/Resource/GpuBuffer.h"
#include "Rendering/GfxManager.h"
#include "Core/Debug.h"
#include "Core/MathHelper.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <list>
#include <queue>
#include <tuple>
#include <vector>
#include <assert.h>

namespace dx12demo
{
    class UploadHeapPage
    {
    public:
        UploadHeapPage(const std::wstring& name, UINT size);
        ~UploadHeapPage() = default;

        template<typename T>
        bool Allocate(UINT count, UINT alignment, UINT* outStride, UINT* outOffset);
        void Reset();

        UploadBuffer* GetBuffer() const { return m_Buffer.get(); }

    private:
        std::unique_ptr<UploadBuffer> m_Buffer;
        std::list<std::pair<UINT, UINT>> m_FreeList{}; // <start, end-exclusive>
    };

    template<typename T>
    class UploadHeapSpan
    {
    public:
        UploadHeapSpan(UploadBuffer* buffer, UINT offset, UINT stride, UINT count)
            : m_Buffer(buffer)
            , m_Offset(offset)
            , m_Stride(stride)
            , m_Count(count)
        {

        }

        ID3D12Resource* GetResource() const { return m_Buffer->GetResource(); }
        UINT GetOffsetInResource() const { return m_Offset; }
        UINT GetStride() const { return m_Stride; }
        UINT GetCount() const { return m_Count; }
        UINT GetSize() const { return m_Stride * m_Count; }

        T& GetData(UINT index) const
        {
            BYTE* p = m_Buffer->GetPointer();
            return *reinterpret_cast<T*>(p[m_Offset + index * m_Stride]);
        }

        void SetData(UINT index, const T& data)
        {
            BYTE* p = m_Buffer->GetPointer();
            memcpy(&p[m_Offset + index * m_Stride], &data, sizeof(T));
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT index = 0) const
        {
            D3D12_GPU_VIRTUAL_ADDRESS addr = m_Buffer->GetGpuVirtualAddress();
            return addr + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_Offset + index * m_Stride);
        }

    private:
        UploadBuffer* m_Buffer;
        UINT m_Offset;
        UINT m_Stride;
        UINT m_Count;
    };

    class UploadHeapAllocator
    {
    public:
        UploadHeapAllocator(UINT pageSize);
        ~UploadHeapAllocator() = default;

        template<typename T>
        UploadHeapSpan<T> Allocate(UINT count, UINT alignment = 1);
        void FlushPages(UINT64 fenceValue);

    private:
        UINT m_PageSize;
        std::vector<std::unique_ptr<UploadHeapPage>> m_AllPages{};       // 所有分配过的 page
        std::queue<std::pair<UINT64, UploadHeapPage*>> m_PendingPages{}; // 等待使用结束的 page
        std::vector<UploadHeapPage*> m_ActivePages{};                    // 正在使用的 page
    };

    template<typename T>
    bool UploadHeapPage::Allocate(UINT count, UINT alignment, UINT* outStride, UINT* outOffset)
    {
        *outStride = MathHelper::AlignUp(sizeof(T), alignment);
        UINT size = (*outStride) * count;

        for (auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it)
        {
            UINT alignedStart = MathHelper::AlignUp(it->first, alignment);

            if (it->second - alignedStart >= size)
            {
                UINT alignedEnd = alignedStart + size;

                if (it->first == alignedStart)
                {
                    it->first = alignedEnd;
                }
                else if (it->second == alignedEnd)
                {
                    it->second = alignedStart;
                }
                else
                {
                    m_FreeList.insert(it, std::make_pair(it->first, alignedStart));
                    it->first = alignedEnd;
                }

                if (it->first == it->second)
                {
                    m_FreeList.erase(it);
                }

                *outOffset = alignedStart;
                return true;
            }
        }

        return false;
    }

    template<typename T>
    UploadHeapSpan<T> UploadHeapAllocator::Allocate(UINT count, UINT alignment)
    {
        UINT stride = 0;
        UINT offset = 0;

        for (UploadHeapPage* page : m_ActivePages)
        {
            if (page->Allocate<T>(count, alignment, &stride, &offset))
            {
                return UploadHeapSpan<T>(page->GetBuffer(), offset, stride, count);
            }
        }

        UploadHeapPage* newPage;

        if (!m_PendingPages.empty() && m_PendingPages.front().first <= GetGfxManager().GetCompletedFenceValue())
        {
            newPage = m_PendingPages.front().second;
            newPage->Reset();
            m_PendingPages.pop();
        }
        else
        {
            DEBUG_LOG_INFO("New upload heap page allocated, size: %d", m_PageSize);
            m_AllPages.push_back(std::make_unique<UploadHeapPage>(L"UploadHeapAllocatorPage", m_PageSize));
            newPage = m_AllPages.back().get();
        }

        m_ActivePages.push_back(newPage);
        assert(newPage->Allocate<T>(count, alignment, &stride, &offset));
        return UploadHeapSpan<T>(newPage->GetBuffer(), offset, stride, count);
    }
}
