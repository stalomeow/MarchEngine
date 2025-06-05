#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Profiling/NsightAftermath.h"
#include "Engine/Misc/DeferFunc.h"
#include "Engine/Debug.h"
#include <pix3.h>

using Microsoft::WRL::ComPtr;

namespace march
{
    GfxCommandList::GfxCommandList(GfxCommandType type, GfxCommandQueue* queue)
        : m_Commands{}
        , m_SyncPointsToWait{}
        , m_FutureSyncPointsToWait{}
        , m_ResourceBarriers{}
        , m_ResourceBarrierFlushOffset(0)
        , m_ColorTargets{}
        , m_Viewports{}
        , m_ScissorRects{}
        , m_VertexBufferViews{}
        , m_SubresourceData{}
        , m_OfflineDescriptors{}
        , m_OfflineDescriptorTableData{}
        , m_ViewHeap(nullptr)
        , m_SamplerHeap(nullptr)
        , m_Type(type)
        , m_Queue(queue)
        , m_List(nullptr)
        , m_NsightAftermathHandle(nullptr)
    {
    }

    GfxCommandList::~GfxCommandList()
    {
        if (m_NsightAftermathHandle != nullptr)
        {
            NsightAftermath::ReleaseContextHandle(m_NsightAftermathHandle);
        }
    }

    GfxSyncPoint GfxCommandList::Execute(bool isImmediateMode)
    {
        // 清理临时资源
        DEFER_FUNC()
        {
            m_Commands.clear();
            m_SyncPointsToWait.clear();
            m_FutureSyncPointsToWait.clear();
            m_ResourceBarriers.clear();
            m_ResourceBarrierFlushOffset = 0;
            m_ColorTargets.clear();
            m_Viewports.clear();
            m_ScissorRects.clear();
            m_VertexBufferViews.clear();
            m_SubresourceData.clear();
            m_OfflineDescriptors.clear();
            m_OfflineDescriptorTableData.clear();
            m_ViewHeap = nullptr;
            m_SamplerHeap = nullptr;
        };

        if (!m_FutureSyncPointsToWait.empty())
        {
            LOG_WARNING("CommandList has {} unresolved future sync points. They will be ignored.", m_FutureSyncPointsToWait.size());
        }

        ComPtr<ID3D12CommandAllocator> allocator = m_Queue->RequestCommandAllocator();

        if (m_List == nullptr)
        {
            ID3D12Device4* device = m_Queue->GetDevice()->GetD3DDevice4();
            CHECK_HR(device->CreateCommandList(0, m_Queue->GetType(), allocator.Get(), nullptr, IID_PPV_ARGS(&m_List)));
            m_NsightAftermathHandle = NsightAftermath::CreateContextHandle(m_List.Get());
        }
        else
        {
            CHECK_HR(m_List->Reset(allocator.Get(), nullptr));
        }

        // 确保所有的命令都被记录，然后翻译
        FlushResourceBarriers();

        for (const auto& cmd : m_Commands)
        {
            std::visit([this, isImmediateMode](auto&& arg) { TranslateCommand(arg, isImmediateMode); }, cmd);
        }

        CHECK_HR(m_List->Close());

        // 等待其他 queue 上的异步操作，例如 async compute, async copy
        for (const GfxSyncPoint& syncPoint : m_SyncPointsToWait)
        {
            m_Queue->WaitOnGpu(syncPoint);
        }

        // 正式执行
        ID3D12CommandList* lists[] = { m_List.Get() };
        m_Queue->GetQueue()->ExecuteCommandLists(1, lists);
        return m_Queue->ReleaseCommandAllocator(allocator);
    }

    void GfxCommandList::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        m_SyncPointsToWait.push_back(syncPoint);
    }

    void GfxCommandList::WaitOnGpu(const GfxFutureSyncPoint& syncPoint)
    {
        m_FutureSyncPointsToWait.push_back(syncPoint);
    }

    void GfxCommandList::BeginEvent(const std::string& name)
    {
        m_Commands.emplace_back(GfxCommands::BeginEvent{ name });
    }

    void GfxCommandList::BeginEvent(std::string&& name)
    {
        m_Commands.emplace_back(GfxCommands::BeginEvent{ std::move(name) });
    }

    void GfxCommandList::EndEvent()
    {
        m_Commands.emplace_back(GfxCommands::EndEvent{});
    }

    void GfxCommandList::AddResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier)
    {
        m_ResourceBarriers.push_back(barrier);
    }

    void GfxCommandList::FlushResourceBarriers()
    {
        // 尽量合并后一起提交

        uint32_t num = static_cast<uint32_t>(m_ResourceBarriers.size() - m_ResourceBarrierFlushOffset);

        if (num > 0)
        {
            m_Commands.emplace_back(GfxCommands::FlushResourceBarriers{ m_ResourceBarrierFlushOffset, num });
            m_ResourceBarrierFlushOffset = m_ResourceBarriers.size();
        }
    }

    void GfxCommandList::SetRenderTargets(uint32_t numColorTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* colorTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilTarget)
    {
        GfxCommands::SetRenderTargets cmd{};

        // Color Target
        cmd.ColorTargetOffset = m_ColorTargets.size();
        cmd.ColorTargetCount = numColorTargets;
        m_ColorTargets.insert(m_ColorTargets.end(), colorTargets, colorTargets + numColorTargets);

        // Depth Stencil Target
        if (depthStencilTarget == nullptr)
        {
            cmd.DepthStencilTarget = std::nullopt;
        }
        else
        {
            cmd.DepthStencilTarget = *depthStencilTarget;
        }

        m_Commands.emplace_back(cmd);
    }

    void GfxCommandList::ClearColorTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, const float color[4])
    {
        m_Commands.emplace_back(GfxCommands::ClearColorTarget{ target, { color[0], color[1], color[2], color[3] } });
    }

    void GfxCommandList::ClearDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, D3D12_CLEAR_FLAGS flags, float depth, uint8_t stencil)
    {
        m_Commands.emplace_back(GfxCommands::ClearDepthStencilTarget{ target, flags, depth, stencil });
    }

    void GfxCommandList::SetViewports(uint32_t num, const D3D12_VIEWPORT* viewports)
    {
        m_Commands.emplace_back(GfxCommands::SetViewports{ m_Viewports.size(), num });
        m_Viewports.insert(m_Viewports.end(), viewports, viewports + num);
    }

    void GfxCommandList::SetScissorRects(uint32_t num, const D3D12_RECT* rects)
    {
        m_Commands.emplace_back(GfxCommands::SetScissorRects{ m_ScissorRects.size(), num });
        m_ScissorRects.insert(m_ScissorRects.end(), rects, rects + num);
    }

    void GfxCommandList::SetPredication(ID3D12Resource* buffer, uint32_t alignedOffset, D3D12_PREDICATION_OP operation)
    {
        m_Commands.emplace_back(GfxCommands::SetPredication{ buffer, alignedOffset, operation });
    }

    void GfxCommandList::SetPipelineState(ID3D12PipelineState* state)
    {
        m_Commands.emplace_back(GfxCommands::SetPipelineState{ state });
    }

    void GfxCommandList::SetStencilRef(uint8_t value)
    {
        m_Commands.emplace_back(GfxCommands::SetStencilRef{ value });
    }

    void GfxCommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        m_Commands.emplace_back(GfxCommands::SetPrimitiveTopology{ topology });
    }

    void GfxCommandList::SetVertexBuffers(uint32_t startSlot, uint32_t numViews, const D3D12_VERTEX_BUFFER_VIEW* views)
    {
        m_Commands.emplace_back(GfxCommands::SetVertexBuffers{ startSlot, m_VertexBufferViews.size(), numViews });
        m_VertexBufferViews.insert(m_VertexBufferViews.end(), views, views + numViews);
    }

    void GfxCommandList::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view)
    {
        if (view == nullptr)
        {
            m_Commands.emplace_back(GfxCommands::SetIndexBuffer{ std::nullopt });
        }
        else
        {
            m_Commands.emplace_back(GfxCommands::SetIndexBuffer{ *view });
        }
    }

    void GfxCommandList::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
    {
        m_Commands.emplace_back(GfxCommands::DrawIndexedInstanced{ indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation });
    }

    void GfxCommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
    {
        m_Commands.emplace_back(GfxCommands::Dispatch{ threadGroupCountX, threadGroupCountY, threadGroupCountZ });
    }

    void GfxCommandList::ResolveSubresource(ID3D12Resource* dstResource, uint32_t dstSubresource, ID3D12Resource* srcResource, uint32_t srcSubresource, DXGI_FORMAT format)
    {
        m_Commands.emplace_back(GfxCommands::ResolveSubresource{ dstResource, dstSubresource, srcResource, srcSubresource, format });
    }

    void GfxCommandList::UpdateSubresources(ID3D12Resource* destination, ID3D12Resource* intermediate, uint32_t intermediateOffset, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData)
    {
        m_Commands.emplace_back(GfxCommands::UpdateSubresources{ destination, intermediate, intermediateOffset, firstSubresource, numSubresources, m_SubresourceData.size() });
        m_SubresourceData.insert(m_SubresourceData.end(), srcData, srcData + numSubresources);
    }

    void GfxCommandList::CopyBufferRegion(ID3D12Resource* dstBuffer, uint32_t dstOffset, ID3D12Resource* srcBuffer, uint32_t srcOffset, uint32_t numBytes)
    {
        m_Commands.emplace_back(GfxCommands::CopyBufferRegion{ dstBuffer, dstOffset, srcBuffer, srcOffset, numBytes });
    }

    void GfxCommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION& dst, const D3D12_TEXTURE_COPY_LOCATION& src)
    {
        m_Commands.emplace_back(GfxCommands::CopyTextureRegion{ dst, src });
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::BeginEvent& cmd, bool isImmediateMode)
    {
        NsightAftermath::SetEventMarker(m_NsightAftermathHandle, cmd.Name);
        PIXBeginEvent(m_List.Get(), 0, cmd.Name.c_str());
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::EndEvent& cmd, bool isImmediateMode)
    {
        PIXEndEvent(m_List.Get());
        NsightAftermath::SetEventMarker(m_NsightAftermathHandle, "EndEvent");
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::FlushResourceBarriers& cmd, bool isImmediateMode)
    {
        m_List->ResourceBarrier(static_cast<UINT>(cmd.Num), m_ResourceBarriers.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetRenderTargets& cmd, bool isImmediateMode)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* pRtv = m_ColorTargets.data() + cmd.ColorTargetOffset;
        const D3D12_CPU_DESCRIPTOR_HANDLE* pDsv = cmd.DepthStencilTarget.has_value() ? &cmd.DepthStencilTarget.value() : nullptr;
        m_List->OMSetRenderTargets(static_cast<UINT>(cmd.ColorTargetCount), pRtv, FALSE, pDsv);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::ClearColorTarget& cmd, bool isImmediateMode)
    {
        m_List->ClearRenderTargetView(cmd.Target, cmd.Color, 0, nullptr);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::ClearDepthStencilTarget& cmd, bool isImmediateMode)
    {
        m_List->ClearDepthStencilView(cmd.Target, cmd.Flags, cmd.Depth, static_cast<UINT8>(cmd.Stencil), 0, nullptr);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetViewports& cmd, bool isImmediateMode)
    {
        m_List->RSSetViewports(static_cast<UINT>(cmd.Num), m_Viewports.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetScissorRects& cmd, bool isImmediateMode)
    {
        m_List->RSSetScissorRects(static_cast<UINT>(cmd.Num), m_ScissorRects.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetPredication& cmd, bool isImmediateMode)
    {
        m_List->SetPredication(cmd.Buffer, static_cast<UINT64>(cmd.AlignedOffset), cmd.Operation);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetPipelineState& cmd, bool isImmediateMode)
    {
        m_List->SetPipelineState(cmd.State);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetStencilRef& cmd, bool isImmediateMode)
    {
        m_List->OMSetStencilRef(static_cast<UINT>(cmd.StencilRef));
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetPrimitiveTopology& cmd, bool isImmediateMode)
    {
        m_List->IASetPrimitiveTopology(cmd.Topology);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetVertexBuffers& cmd, bool isImmediateMode)
    {
        m_List->IASetVertexBuffers(static_cast<UINT>(cmd.StartSlot), static_cast<UINT>(cmd.Num), m_VertexBufferViews.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetIndexBuffer& cmd, bool isImmediateMode)
    {
        if (cmd.View.has_value())
        {
            m_List->IASetIndexBuffer(&cmd.View.value());
        }
        else
        {
            m_List->IASetIndexBuffer(nullptr);
        }
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::DrawIndexedInstanced& cmd, bool isImmediateMode)
    {
        m_List->DrawIndexedInstanced(
            static_cast<UINT>(cmd.IndexCountPerInstance),
            static_cast<UINT>(cmd.InstanceCount),
            static_cast<UINT>(cmd.StartIndexLocation),
            static_cast<INT>(cmd.BaseVertexLocation),
            static_cast<UINT>(cmd.StartInstanceLocation));
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::Dispatch& cmd, bool isImmediateMode)
    {
        m_List->Dispatch(
            static_cast<UINT>(cmd.ThreadGroupCountX),
            static_cast<UINT>(cmd.ThreadGroupCountY),
            static_cast<UINT>(cmd.ThreadGroupCountZ));
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::ResolveSubresource& cmd, bool isImmediateMode)
    {
        m_List->ResolveSubresource(
            cmd.DstResource,
            static_cast<UINT>(cmd.DstSubresource),
            cmd.SrcResource,
            static_cast<UINT>(cmd.SrcSubresource),
            cmd.Format);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::UpdateSubresources& cmd, bool isImmediateMode)
    {
        ::UpdateSubresources(
            m_List.Get(),
            cmd.Destination,
            cmd.Intermediate,
            static_cast<UINT64>(cmd.IntermediateOffset),
            static_cast<UINT>(cmd.FirstSubresource),
            static_cast<UINT>(cmd.NumSubresources),
            m_SubresourceData.data() + cmd.SrcDataOffset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::CopyBufferRegion& cmd, bool isImmediateMode)
    {
        m_List->CopyBufferRegion(
            cmd.DstBuffer,
            static_cast<UINT64>(cmd.DstOffset),
            cmd.SrcBuffer,
            static_cast<UINT64>(cmd.SrcOffset),
            static_cast<UINT64>(cmd.NumBytes));
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::CopyTextureRegion& cmd, bool isImmediateMode)
    {
        m_List->CopyTextureRegion(&cmd.Dst, 0, 0, 0, &cmd.Src, nullptr);
    }
}
