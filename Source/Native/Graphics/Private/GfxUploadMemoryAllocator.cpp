#include "GfxUploadMemoryAllocator.h"
#include "GfxDevice.h"
#include "GfxBuffer.h"
#include "GfxFence.h"
#include "MathHelper.h"
#include "Debug.h"
#include <stdexcept>

namespace march
{
    GfxUploadMemory::GfxUploadMemory(GfxUploadBuffer* buffer, uint32_t offset, uint32_t stride, uint32_t count)
        : m_Buffer(buffer), m_Offset(offset), m_Stride(stride), m_Count(count)
    {
    }

    uint8_t* GfxUploadMemory::GetMappedData(uint32_t index) const
    {
        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of range");
        }

        uint8_t* p = m_Buffer->GetMappedData(0);
        return &p[m_Offset + index * m_Stride];
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxUploadMemory::GetGpuVirtualAddress(uint32_t index) const
    {
        using Addr = D3D12_GPU_VIRTUAL_ADDRESS;

        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of range");
        }

        Addr addr = m_Buffer->GetGpuVirtualAddress(0);
        Addr offset = static_cast<Addr>(m_Offset) + static_cast<Addr>(index) * static_cast<Addr>(m_Stride);
        return addr + offset;
    }

    ID3D12Resource* GfxUploadMemory::GetResource() const
    {
        return m_Buffer->GetResource();
    }

    GfxUploadMemoryAllocator::GfxUploadMemoryAllocator(GfxDevice* device)
        : m_Device(device)
        , m_AllocateOffset(PageSize) // 初始化为最大值，保证第一次分配时会创建新的 page
        , m_PageCounter(0), m_UsedPages{}, m_ReleaseQueue{}
    {
    }

    void GfxUploadMemoryAllocator::BeginFrame()
    {
    }

    void GfxUploadMemoryAllocator::EndFrame(uint64_t fenceValue)
    {
        for (std::unique_ptr<GfxUploadBuffer>& p : m_UsedPages)
        {
            m_ReleaseQueue.emplace(fenceValue, std::move(p));
        }

        m_UsedPages.clear();
    }

    GfxUploadMemory GfxUploadMemoryAllocator::Allocate(uint32_t size, uint32_t count, uint32_t alignment)
    {
        uint32_t stride = MathHelper::AlignUp(size, alignment);
        uint32_t totalSize = stride * count;

        // m_UsedPages 中最后一个元素是当前正在分配的普通 page
        // 分配 large page 时，插入在最前面，避免干扰普通 page 的分配

        if (totalSize > PageSize)
        {
            std::string largePageName = "GfxUploadMemoryPage (Large)";
            m_UsedPages.emplace_front(std::make_unique<GfxUploadBuffer>(m_Device, largePageName, stride, count, true));

            DEBUG_LOG_INFO("Create %s; Size: %d", largePageName.c_str(), totalSize);
            return GfxUploadMemory(m_UsedPages.front().get(), 0, stride, count);
        }

        uint32_t offset = MathHelper::AlignUp(m_AllocateOffset, alignment);

        if (offset + totalSize > PageSize)
        {
            bool success = false;
            GfxFence* fence = m_Device->GetGraphicsFence();

            while (!success && !m_ReleaseQueue.empty() && fence->IsCompleted(m_ReleaseQueue.front().first))
            {
                std::unique_ptr<GfxUploadBuffer>& p = m_ReleaseQueue.front().second;

                // 如果 page 大小不对，之后 pop() 时会直接销毁它
                if (p->GetSize() == PageSize)
                {
                    m_UsedPages.emplace_back(std::move(p));
                    success = true;
                }

                m_ReleaseQueue.pop();
            }

            if (!success)
            {
                std::string pageName = "GfxUploadMemoryPage" + std::to_string(m_PageCounter++);
                m_UsedPages.emplace_back(std::make_unique<GfxUploadBuffer>(m_Device, pageName, PageSize, 1, true));
                success = true;

                DEBUG_LOG_INFO("Create %s; Size: %d", pageName.c_str(), PageSize);
            }

            offset = 0;
        }

        m_AllocateOffset = offset + totalSize;
        return GfxUploadMemory(m_UsedPages.back().get(), offset, stride, count);
    }
}
