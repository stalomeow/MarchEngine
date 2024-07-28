#pragma once

#include "Rendering/Resource/GpuBuffer.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <list>
#include <queue>
#include <tuple>
#include <vector>

namespace dx12demo
{
    class UploadHeapSpan;

    class UploadHeapPage
    {
    public:
        UploadHeapPage(const std::wstring& name, UINT size);
        ~UploadHeapPage() = default;

        bool Allocate(UINT size, UINT64 completedFenceValue, UploadHeapSpan* outSpan);
        void Free(UploadHeapSpan& span, UINT64 fenceValue);

        UploadBuffer* GetBuffer() const { return m_Buffer.get(); }

    private:
        void RecycleUsed(UINT64 completedFenceValue);

        std::unique_ptr<UploadBuffer> m_Buffer;
        std::list<std::pair<int, int>> m_FreeList{}; // <start, end-exclusive>
        std::queue<std::tuple<UINT64, int, int>> m_Used{}; // <fence, start, end-exclusive>
    };

    class UploadHeapSpan
    {
        friend class UploadHeapPage;

    public:
        void Free(UINT64 fenceValue);

        ID3D12Resource* GetResource() const { return m_Page->GetBuffer()->GetResource(); }
        UINT GetOffsetInResource() const { return m_Offset; }
        UINT GetSize() const { return m_Size; }

        template <typename T = BYTE>
        T* GetPointer() const
        {
            BYTE* p = m_Page->GetBuffer()->GetPointer();
            return reinterpret_cast<T*>(p + m_Offset);
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const
        {
            D3D12_GPU_VIRTUAL_ADDRESS addr = m_Page->GetBuffer()->GetGpuVirtualAddress();
            return addr + m_Offset;
        }

    private:
        UploadHeapPage* m_Page;
        UINT m_Offset;
        UINT m_Size;
    };

    class UploadHeapAllocator
    {
    public:
        UploadHeapAllocator(UINT pageSize);
        ~UploadHeapAllocator() = default;

        UploadHeapSpan Allocate(UINT size, UINT64 completedFenceValue);

    private:
        UINT m_PageSize;
        std::vector<std::unique_ptr<UploadHeapPage>> m_Pages{};
    };
}
