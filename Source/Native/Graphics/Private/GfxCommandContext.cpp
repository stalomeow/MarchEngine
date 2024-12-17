#include "GfxCommand.h"
#include "GfxDevice.h"
#include "GfxResource.h"
#include "GfxTexture.h"
#include "Material.h"
#include <assert.h>

using namespace Microsoft::WRL;

namespace march
{
    GfxCommandContext::GfxCommandContext(GfxDevice* device, GfxCommandType type)
        : m_Device(device)
        , m_Type(type)
        , m_CommandAllocator(nullptr)
        , m_CommandList(nullptr)
        , m_ResourceBarriers{}
        , m_SyncPointsToWait{}
        , m_GraphicsSrvCbvBufferCache{}
        , m_GraphicsSrvUavCache{}
        , m_GraphicsSamplerCache{}
        , m_GraphicsViewResourceRequiredStates{}
        , m_ViewHeap(nullptr)
        , m_SamplerHeap(nullptr)
        , m_CurrentGraphicsRootSignature(nullptr)
        , m_GlobalTextures{}
        , m_GlobalBuffers{}
    {
    }

    void GfxCommandContext::Open()
    {
        assert(m_CommandAllocator == nullptr);

        GfxCommandQueue* queue = m_Device->GetCommandManager()->GetQueue(m_Type);
        m_CommandAllocator = queue->RequestCommandAllocator();

        if (m_CommandList == nullptr)
        {
            GFX_HR(m_Device->GetD3DDevice4()->CreateCommandList(0, queue->GetType(),
                m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
        }
        else
        {
            GFX_HR(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
        }
    }

    GfxSyncPoint GfxCommandContext::SubmitAndRelease()
    {
        GfxCommandManager* manager = m_Device->GetCommandManager();
        GfxCommandQueue* queue = manager->GetQueue(m_Type);

        // 准备好所有命令，然后关闭
        FlushResourceBarriers();
        GFX_HR(m_CommandList->Close());

        // 等待其他 queue 上的异步操作，例如 async compute, async copy
        for (const GfxSyncPoint& syncPoint : m_SyncPointsToWait)
        {
            queue->WaitOnGpu(syncPoint);
        }

        // 正式提交
        ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
        queue->GetQueue()->ExecuteCommandLists(static_cast<UINT>(std::size(commandLists)), commandLists);
        GfxSyncPoint syncPoint = queue->ReleaseCommandAllocator(m_CommandAllocator);

        // 释放临时资源
        m_CommandAllocator = nullptr;
        m_ResourceBarriers.clear();
        m_SyncPointsToWait.clear();

        // 回收
        manager->RecycleContext(this);
        return syncPoint;
    }

    void GfxCommandContext::TransitionResource(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter)
    {
        D3D12_RESOURCE_STATES stateBefore = resource->GetState();
        bool needTransition;

        if (stateAfter == D3D12_RESOURCE_STATE_COMMON)
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states
            // D3D12_RESOURCE_STATE_COMMON 为 0，要特殊处理
            needTransition = stateBefore != stateAfter;
        }
        else
        {
            needTransition = (stateBefore & stateAfter) != stateAfter;
        }

        if (needTransition)
        {
            ID3D12Resource* res = resource->GetD3DResource();
            m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(res, stateBefore, stateAfter));
            resource->SetState(stateAfter);
        }
    }

    void GfxCommandContext::FlushResourceBarriers()
    {
        // 尽量合并后一起提交

        if (!m_ResourceBarriers.empty())
        {
            UINT count = static_cast<UINT>(m_ResourceBarriers.size());
            m_CommandList->ResourceBarrier(count, m_ResourceBarriers.data());
            m_ResourceBarriers.clear();
        }
    }

    void GfxCommandContext::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        m_SyncPointsToWait.push_back(syncPoint);
    }

    GfxTexture* GfxCommandContext::FindTexture(int32_t id, Material* material)
    {
        GfxTexture* texture = nullptr;

        if (!material->GetTexture(id, &texture))
        {
            if (auto it = m_GlobalTextures.find(id); it != m_GlobalTextures.end())
            {
                texture = it->second;
            }
        }

        return texture;
    }

    GfxBuffer* GfxCommandContext::FindBuffer(int32_t id, bool isConstantBuffer, Material* material, int32_t passIndex)
    {
        if (isConstantBuffer && id == Shader::GetMaterialConstantBufferId())
        {
            return material->GetConstantBuffer(passIndex);
        }

        if (auto it = m_GlobalBuffers.find(id); it != m_GlobalBuffers.end())
        {
            return it->second;
        }

        return nullptr;
    }

    void GfxCommandContext::SetGraphicsSrvCbvBuffer(ShaderProgramType type, uint32_t index, std::shared_ptr<GfxResource> resource, D3D12_GPU_VIRTUAL_ADDRESS address, bool isConstantBuffer)
    {
        m_GraphicsSrvCbvBufferCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), address, isConstantBuffer);

        if (isConstantBuffer)
        {
            m_GraphicsViewResourceRequiredStates[resource] |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        else if (type == ShaderProgramType::Pixel)
        {
            m_GraphicsViewResourceRequiredStates[resource] |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        else
        {
            m_GraphicsViewResourceRequiredStates[resource] |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }
    }

    void GfxCommandContext::SetGraphicsSrv(ShaderProgramType type, uint32_t index, std::shared_ptr<GfxResource> resource, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor)
    {
        m_GraphicsSrvUavCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);

        // 记录状态，之后会统一使用 ResourceBarrier
        if (type == ShaderProgramType::Pixel)
        {
            m_GraphicsViewResourceRequiredStates[resource] |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        else
        {
            m_GraphicsViewResourceRequiredStates[resource] |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }
    }

    void GfxCommandContext::SetGraphicsUav(ShaderProgramType type, uint32_t index, std::shared_ptr<GfxResource> resource, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor)
    {
        m_GraphicsSrvUavCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);

        // 记录状态，之后会统一使用 ResourceBarrier
        m_GraphicsViewResourceRequiredStates[resource] |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    void GfxCommandContext::SetGraphicsSampler(ShaderProgramType type, uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor)
    {
        m_GraphicsSamplerCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);
    }

    void GfxCommandContext::SetGraphicsRootSignatureAndParameters(GfxRootSignature* rootSignature, Material* material, int32_t passIndex)
    {
        // GfxRootSignature 本身不复用，但内部的 ID3D12RootSignature 是复用的
        // 如果 ID3D12RootSignature 变了，说明根签名发生了结构上的变化
        if (m_CurrentGraphicsRootSignature != rootSignature->GetD3DRootSignature())
        {
            // 删掉旧的 view
            for (auto& cache : m_GraphicsSrvCbvBufferCache) cache.Reset();
            for (auto& cache : m_GraphicsSrvUavCache) cache.Reset();
            for (auto& cache : m_GraphicsSamplerCache) cache.Reset();
            m_GraphicsViewResourceRequiredStates.clear();

            // 设置 root signature
            m_CurrentGraphicsRootSignature = rootSignature->GetD3DRootSignature();
            m_CommandList->SetGraphicsRootSignature(m_CurrentGraphicsRootSignature);
        }

        for (int32_t i = 0; i < ShaderProgram::NumTypes; i++)
        {
            ShaderProgramType programType = static_cast<ShaderProgramType>(i);

            for (const GfxRootSignature::BufferBinding& buf : rootSignature->GetSrvCbvBufferRootParamIndices(programType))
            {
                if (GfxBuffer* buffer = FindBuffer(buf.Id, buf.IsConstantBuffer, material, passIndex))
                {
                    SetGraphicsSrvCbvBuffer(programType, buf.BindPoint, buffer->GetResource(), buffer->GetGpuVirtualAddress(), buf.IsConstantBuffer);
                }
            }

            for (const GfxRootSignature::TextureBinding& tex : rootSignature->GetSrvTextureTableSlots(programType))
            {
                if (GfxTexture* texture = FindTexture(tex.Id, material))
                {
                    SetGraphicsSrv(programType, tex.BindPointTexture, texture->GetResource(), texture->GetSrv());

                    if (tex.BindPointSampler.has_value())
                    {
                        SetGraphicsSampler(programType, *tex.BindPointSampler, texture->GetSampler());
                    }
                }
            }

            // TODO uav buffer
            //for (const GfxRootSignature::UavBinding& buf : rootSignature->GetUavBufferTableSlots(programType))
            //{

            //}

            for (const GfxRootSignature::UavBinding& tex : rootSignature->GetUavTextureTableSlots(programType))
            {
                if (GfxTexture* texture = FindTexture(tex.Id, material))
                {
                    SetGraphicsUav(programType, tex.BindPoint, texture->GetResource(), texture->GetUav());
                }
            }
        }

        TransitionGraphicsViewResources();
        SetGraphicsRootDescriptorTablesAndHeaps(rootSignature);
        SetGraphicsRootSrvCbvBuffers();
    }

    void GfxCommandContext::SetGraphicsRootDescriptorTablesAndHeaps(GfxRootSignature* rootSignature)
    {
        // ------------------------------------------------------------
        // SRV & UAV
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE srvUavTables[ShaderProgram::NumTypes]{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* offlineSrvUav[ShaderProgram::NumTypes]{};
        uint32_t numSrvUav[ShaderProgram::NumTypes]{};

        GfxOnlineDescriptorMultiAllocator* viewAllocator = m_Device->GetOnlineViewDescriptorAllocator();
        GfxDescriptorHeap* viewHeap = nullptr;
        bool hasSrvUav = false;

        for (uint32_t numTry = 0; numTry < 2; numTry++)
        {
            uint32_t totalNumSrvUav = 0;

            for (int32_t i = 0; i < ShaderProgram::NumTypes; i++)
            {
                ShaderProgramType programType = static_cast<ShaderProgramType>(i);
                std::optional<uint32_t> srvUavTableRootParamIndex = rootSignature->GetSrvUavTableRootParamIndex(programType);
                const auto& srvUavCache = m_GraphicsSrvUavCache[i];

                if (srvUavTableRootParamIndex.has_value() && srvUavCache.IsDirty() && !srvUavCache.IsEmpty())
                {
                    offlineSrvUav[i] = srvUavCache.GetDescriptors();
                    numSrvUav[i] = static_cast<uint32_t>(srvUavCache.GetNum());
                }
                else
                {
                    offlineSrvUav[i] = nullptr;
                    numSrvUav[i] = 0;
                }

                totalNumSrvUav += numSrvUav[i];
            }

            if (totalNumSrvUav > 0)
            {
                if (viewAllocator->AllocateMany(std::size(srvUavTables), offlineSrvUav, numSrvUav, srvUavTables, &viewHeap))
                {
                    hasSrvUav = true;
                    break;
                }

                // 当前的 heap 不够，切换 heap
                viewAllocator->Rollover();

                // 因为 heap 变了，所以 table 要全部重新分配
                for (auto& cache : m_GraphicsSrvUavCache)
                {
                    cache.SetDirty(true);
                }
            }
            else
            {
                // 没有 SRV & UAV，不需要分配
                break;
            }
        }

        // ------------------------------------------------------------
        // SAMPLER
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE samplerTables[ShaderProgram::NumTypes]{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* offlineSamplers[ShaderProgram::NumTypes]{};
        uint32_t numSamplers[ShaderProgram::NumTypes]{};

        GfxOnlineDescriptorMultiAllocator* samplerAllocator = m_Device->GetOnlineSamplerDescriptorAllocator();
        GfxDescriptorHeap* samplerHeap = nullptr;
        bool hasSampler = false;

        for (uint32_t numTry = 0; numTry < 2; numTry++)
        {
            uint32_t totalNumSamplers = 0;

            for (int32_t i = 0; i < ShaderProgram::NumTypes; i++)
            {
                ShaderProgramType programType = static_cast<ShaderProgramType>(i);
                std::optional<uint32_t> samplerTableRootParamIndex = rootSignature->GetSamplerTableRootParamIndex(programType);
                const auto& samplerCache = m_GraphicsSamplerCache[i];

                if (samplerTableRootParamIndex.has_value() && samplerCache.IsDirty() && !samplerCache.IsEmpty())
                {
                    offlineSamplers[i] = samplerCache.GetDescriptors();
                    numSamplers[i] = static_cast<uint32_t>(samplerCache.GetNum());
                }
                else
                {
                    offlineSamplers[i] = nullptr;
                    numSamplers[i] = 0;
                }

                totalNumSamplers += numSamplers[i];
            }

            if (totalNumSamplers > 0)
            {
                if (samplerAllocator->AllocateMany(std::size(samplerTables), offlineSamplers, numSamplers, samplerTables, &samplerHeap))
                {
                    hasSampler = true;
                    break;
                }

                // 当前的 heap 不够，切换 heap
                samplerAllocator->Rollover();

                // 因为 heap 变了，所以 table 要全部重新分配
                for (auto& cache : m_GraphicsSamplerCache)
                {
                    cache.SetDirty(true);
                }
            }
            else
            {
                // 没有 SAMPLER，不需要分配
                break;
            }
        }

        // ------------------------------------------------------------
        // Apply
        // ------------------------------------------------------------

        if (!hasSrvUav && !hasSampler)
        {
            return;
        }

        bool isHeapChanged = false;

        if (hasSrvUav && viewHeap != m_ViewHeap)
        {
            m_ViewHeap = viewHeap;
            isHeapChanged = true;
        }

        if (hasSampler && samplerHeap != m_SamplerHeap)
        {
            m_SamplerHeap = samplerHeap;
            isHeapChanged = true;
        }

        if (isHeapChanged)
        {
            SetDescriptorHeaps();
        }

        for (int32_t i = 0; i < ShaderProgram::NumTypes; i++)
        {
            ShaderProgramType programType = static_cast<ShaderProgramType>(i);

            if (hasSrvUav && numSrvUav[i] > 0)
            {
                uint32_t rootParamIndex = *rootSignature->GetSrvUavTableRootParamIndex(programType);
                m_CommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(rootParamIndex), srvUavTables[i]);
            }

            if (hasSampler && numSamplers[i] > 0)
            {
                uint32_t rootParamIndex = *rootSignature->GetSamplerTableRootParamIndex(programType);
                m_CommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(rootParamIndex), samplerTables[i]);
            }
        }

        if (hasSrvUav)
        {
            for (auto& cache : m_GraphicsSrvUavCache)
            {
                cache.SetDirty(false);
            }
        }

        if (hasSampler)
        {
            for (auto& cache : m_GraphicsSamplerCache)
            {
                cache.SetDirty(false);
            }
        }
    }

    void GfxCommandContext::SetGraphicsRootSrvCbvBuffers()
    {
        for (auto& cache : m_GraphicsSrvCbvBufferCache)
        {
            for (size_t i = 0; i < cache.GetNum(); i++)
            {
                if (!cache.IsDirty(i))
                {
                    continue;
                }

                bool isConstantBuffer = false;
                D3D12_GPU_VIRTUAL_ADDRESS address = cache.Get(i, &isConstantBuffer);

                if (isConstantBuffer)
                {
                    m_CommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(i), address);
                }
                else
                {
                    m_CommandList->SetGraphicsRootShaderResourceView(static_cast<UINT>(i), address);
                }
            }

            cache.Apply();
        }
    }

    void GfxCommandContext::TransitionGraphicsViewResources()
    {
        for (const auto& [resource, state] : m_GraphicsViewResourceRequiredStates)
        {
            TransitionResource(resource.get(), state);
        }

        m_GraphicsViewResourceRequiredStates.clear();
    }

    void GfxCommandContext::SetDescriptorHeaps()
    {
        if (m_ViewHeap != nullptr && m_SamplerHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { m_ViewHeap->GetD3DDescriptorHeap(), m_SamplerHeap->GetD3DDescriptorHeap() };
            m_CommandList->SetDescriptorHeaps(2, heaps);
        }
        else if (m_ViewHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { m_ViewHeap->GetD3DDescriptorHeap() };
            m_CommandList->SetDescriptorHeaps(1, heaps);
        }
        else if (m_SamplerHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { m_SamplerHeap->GetD3DDescriptorHeap() };
            m_CommandList->SetDescriptorHeaps(1, heaps);
        }
    }
}
