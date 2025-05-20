#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Profiling/NsightAftermath.h"
#include <pix3.h>

namespace march
{
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

    void GfxCommandList::ClearColorTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float color[4])
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

    void GfxCommandList::TranslateCommand(const GfxCommands::BeginEvent& cmd)
    {
        NsightAftermath::SetEventMarker(m_NsightAftermathHandle, cmd.Name);
        PIXBeginEvent(m_List.Get(), 0, cmd.Name.c_str());
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::EndEvent& cmd)
    {
        PIXEndEvent(m_List.Get());
        NsightAftermath::SetEventMarker(m_NsightAftermathHandle, "EndEvent");
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::FlushResourceBarriers& cmd)
    {
        m_List->ResourceBarrier(static_cast<UINT>(cmd.Num), m_ResourceBarriers.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetRenderTargets& cmd)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* pRtv = m_ColorTargets.data() + cmd.ColorTargetOffset;
        const D3D12_CPU_DESCRIPTOR_HANDLE* pDsv = cmd.DepthStencilTarget.has_value() ? &cmd.DepthStencilTarget.value() : nullptr;
        m_List->OMSetRenderTargets(static_cast<UINT>(cmd.ColorTargetCount), pRtv, FALSE, pDsv);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::ClearColorTarget& cmd)
    {
        m_List->ClearRenderTargetView(cmd.Target, cmd.Color, 0, nullptr);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::ClearDepthStencilTarget& cmd)
    {
        m_List->ClearDepthStencilView(cmd.Target, cmd.Flags, cmd.Depth, static_cast<UINT8>(cmd.Stencil), 0, nullptr);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetViewports& cmd)
    {
        m_List->RSSetViewports(static_cast<UINT>(cmd.Num), m_Viewports.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetScissorRects& cmd)
    {
        m_List->RSSetScissorRects(static_cast<UINT>(cmd.Num), m_ScissorRects.data() + cmd.Offset);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetPredication& cmd)
    {
        m_List->SetPredication(cmd.Buffer, static_cast<UINT64>(cmd.AlignedOffset), cmd.Operation);
    }

    void GfxCommandList::TranslateCommand(const GfxCommands::SetPipelineState& cmd)
    {
        m_List->SetPipelineState(cmd.State);
    }
}
