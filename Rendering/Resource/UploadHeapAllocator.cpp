#include "Rendering/Resource/UploadHeapAllocator.h"
#include "Core/Debug.h"
#include <utility>
#include <assert.h>

namespace dx12demo
{
    UploadHeapPage::UploadHeapPage(const std::wstring& name, UINT size)
    {
        m_Buffer = std::make_unique<UploadBuffer>(name, size);

        m_FreeList.emplace_back(-1, -1); // 头结点，简化代码编写
        m_FreeList.emplace_back(0, size);
        m_FreeList.emplace_back(size + 1, size + 1); // 尾结点，简化代码编写
    }

    bool UploadHeapPage::Allocate(UINT size, UINT64 completedFenceValue, UploadHeapSpan* outSpan)
    {
        RecycleUsed(completedFenceValue);

        for (auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it)
        {
            if (it->second - it->first >= size)
            {
                outSpan->m_Page = this;
                outSpan->m_Offset = it->first;
                outSpan->m_Size = size;

                it->first += size;
                if (it->first == it->second)
                {
                    m_FreeList.erase(it);
                }
                return true;
            }
        }

        return false;
    }

    void UploadHeapPage::Free(UploadHeapSpan& span, UINT64 fenceValue)
    {
        if (!m_Used.empty())
        {
            auto& [prevFenceValue, start, end] = m_Used.back();

            // 与之前的合并
            if (prevFenceValue == fenceValue && end == span.GetOffsetInResource())
            {
                end = span.GetOffsetInResource() + span.GetSize();
                return;
            }
        }

        m_Used.emplace(fenceValue, span.GetOffsetInResource(), span.GetOffsetInResource() + span.GetSize());
    }

    void UploadHeapPage::RecycleUsed(UINT64 completedFenceValue)
    {
        while (!m_Used.empty())
        {
            auto& [fenceValue, start, end] = m_Used.front();

            if (fenceValue > completedFenceValue)
            {
                break;
            }

            for (auto it = m_FreeList.begin(); it != m_FreeList.end(); ++it)
            {
                if (it->first >= end)
                {
                    auto prev = std::prev(it, 1);

                    // 与前后都相接
                    if (prev->second == start && it->first == end)
                    {
                        prev->second = it->second;
                        m_FreeList.erase(it);
                        break;
                    }

                    // 只与前面相接
                    if (prev->second == start)
                    {
                        prev->second = end;
                        break;
                    }

                    // 只与后面相接
                    if (it->first == end)
                    {
                        it->first = start;
                        break;
                    }

                    // 前后都不相接
                    m_FreeList.insert(it, std::make_pair(start, end));
                    break;
                }
            }

            m_Used.pop();
        }
    }

    void UploadHeapSpan::Free(UINT64 fenceValue)
    {
        m_Page->Free(*this, fenceValue);
    }

    UploadHeapAllocator::UploadHeapAllocator(UINT pageSize)
        : m_PageSize(pageSize)
    {

    }

    UploadHeapSpan UploadHeapAllocator::Allocate(UINT size, UINT64 completedFenceValue)
    {
        assert(size <= m_PageSize);
        UploadHeapSpan ret = {};

        for (auto& page : m_Pages)
        {
            if (page->Allocate(size, completedFenceValue, &ret))
            {
                return ret;
            }
        }

        m_Pages.push_back(std::make_unique<UploadHeapPage>(L"UploadHeapAllocatorPage", m_PageSize));
        assert(m_Pages.back()->Allocate(size, completedFenceValue, &ret));
        DEBUG_LOG_INFO("New upload heap page allocated, size: %d", m_PageSize);
        return ret;
    }
}
