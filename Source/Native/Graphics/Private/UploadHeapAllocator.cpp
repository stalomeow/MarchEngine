#include "UploadHeapAllocator.h"

namespace march
{
    UploadHeapPage::UploadHeapPage(const std::wstring& name, UINT size)
    {
        m_Buffer = std::make_unique<UploadBuffer>(name, size);
        Reset();
    }

    bool UploadHeapPage::Allocate(UINT alignment, UINT alignedSize, UINT* outOffset)
    {
        for (auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it)
        {
            UINT alignedStart = MathHelper::AlignUp(it->first, alignment);

            if (it->second - alignedStart >= alignedSize)
            {
                UINT alignedEnd = alignedStart + alignedSize;

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
        for (auto normalPage : m_ActiveNormalPages)
        {
            m_PendingNormalPages.emplace(fenceValue, normalPage);
        }
        m_ActiveNormalPages.clear();

        for (auto& largePage : m_ActiveLargePages)
        {
            m_PendingLargePages.emplace(fenceValue, std::move(largePage));
        }
        m_ActiveLargePages.clear();
    }

    UploadHeapPage* UploadHeapAllocator::RequestNormalPage(UINT64 completedFenceValue)
    {
        UploadHeapPage* page;

        if (!m_PendingNormalPages.empty() && m_PendingNormalPages.front().first <= completedFenceValue)
        {
            page = m_PendingNormalPages.front().second;
            m_PendingNormalPages.pop();
            page->Reset();
        }
        else
        {
            DEBUG_LOG_INFO("New upload heap page allocated, size: %d", m_PageSize);
            m_AllNormalPages.push_back(std::make_unique<UploadHeapPage>(L"UploadHeapAllocatorPage", m_PageSize));
            page = m_AllNormalPages.back().get();
        }

        m_ActiveNormalPages.push_back(page);
        return page;
    }

    void UploadHeapAllocator::FreeLargePages(UINT64 completedFenceValue)
    {
        while (!m_PendingLargePages.empty() && m_PendingLargePages.front().first <= completedFenceValue)
        {
            m_PendingLargePages.pop();
        }
    }
}
