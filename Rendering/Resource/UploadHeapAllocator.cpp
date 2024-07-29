#include "Rendering/Resource/UploadHeapAllocator.h"

namespace dx12demo
{
    UploadHeapPage::UploadHeapPage(const std::wstring& name, UINT size)
    {
        m_Buffer = std::make_unique<UploadBuffer>(name, size);
        Reset();
    }

    void UploadHeapPage::Reset()
    {
        m_FreeList.clear();
        m_FreeList.emplace_back(0, m_Buffer->GetSize());
    }

    UploadHeapAllocator::UploadHeapAllocator(UINT pageSize)
        : m_PageSize(pageSize)
    {

    }

    void UploadHeapAllocator::FlushPages(UINT64 fenceValue)
    {
        for (auto* page : m_ActivePages)
        {
            m_PendingPages.emplace(fenceValue, page);
        }

        m_ActivePages.clear();
    }
}
