#pragma once

#include "GpuBuffer.h"
#include "GfxManager.h"
#include "Debug.h"
#include "MathHelper.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <list>
#include <queue>
#include <tuple>
#include <vector>
#include <assert.h>

namespace march
{
    class UploadHeapPage
    {
    public:
        UploadHeapPage(const std::wstring& name, UINT size);
        ~UploadHeapPage() = default;

        bool Allocate(UINT alignment, UINT alignedSize, UINT* outOffset);
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
        UploadHeapPage* RequestNormalPage(UINT64 completedFenceValue);
        void FreeLargePages(UINT64 completedFenceValue);

        UINT m_PageSize;
        std::vector<std::unique_ptr<UploadHeapPage>> m_AllNormalPages{};                      // 所有分配过的 normal page
        std::queue<std::pair<UINT64, UploadHeapPage*>> m_PendingNormalPages{};                // 等待使用结束的 normal page
        std::vector<UploadHeapPage*> m_ActiveNormalPages{};                                   // 正在使用的 normal page

        std::queue<std::pair<UINT64, std::unique_ptr<UploadHeapPage>>> m_PendingLargePages{}; // 等待使用结束的 large page
        std::vector<std::unique_ptr<UploadHeapPage>> m_ActiveLargePages{};                    // 正在使用的 large page
    };

    template<typename T>
    UploadHeapSpan<T> UploadHeapAllocator::Allocate(UINT count, UINT alignment)
    {
        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();
        FreeLargePages(completedFenceValue);

        UINT stride = MathHelper::AlignUp(sizeof(T), alignment);
        UINT alignedSize = stride * count;
        UINT offset = 0;

        if (alignedSize > m_PageSize)
        {
            DEBUG_LOG_INFO("Large upload heap page allocated, size: %d", alignedSize);
            m_ActiveLargePages.push_back(std::make_unique<UploadHeapPage>(L"UploadHeapAllocatorLargePage", alignedSize));
            UploadHeapPage* largePage = m_ActiveLargePages.back().get();
            assert(largePage->Allocate(alignment, alignedSize, &offset));
            return UploadHeapSpan<T>(largePage->GetBuffer(), offset, stride, count);
        }

        // 尝试从已有的 page 中分配
        for (UploadHeapPage* page : m_ActiveNormalPages)
        {
            if (page->Allocate(alignment, alignedSize, &offset))
            {
                return UploadHeapSpan<T>(page->GetBuffer(), offset, stride, count);
            }
        }

        UploadHeapPage* newPage = RequestNormalPage(completedFenceValue);
        assert(newPage->Allocate(alignment, alignedSize, &offset));
        return UploadHeapSpan<T>(newPage->GetBuffer(), offset, stride, count);
    }
}
