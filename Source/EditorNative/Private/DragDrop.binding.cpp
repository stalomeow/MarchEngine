#include "pch.h"
#include "Engine/Scripting/InteropServices.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>

static const char* DragDropType = "march_drag_drop";

NATIVE_EXPORT_AUTO DragDrop_BeginSource(cs_bool isExternal)
{
    ImGuiDragDropFlags flags = ImGuiDragDropFlags_None;
    if (isExternal) flags |= ImGuiDragDropFlags_SourceExtern;

    if (!ImGui::BeginDragDropSource(flags))
    {
        retcs false;
    }

    if (ImGui::GetCurrentContext()->DragDropSourceFlags == flags)
    {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        // 如果之前已经设置了 payload，那么没必要再设置一次
        if (payload != nullptr && payload->IsDataType(DragDropType))
        {
            ImGui::TextUnformatted(static_cast<char*>(payload->Data)); // 显示 tooltip
            ImGui::EndDragDropSource();
            retcs false;
        }
    }

    retcs true;
}

NATIVE_EXPORT_AUTO DragDrop_EndSource(cs_string csTooltip)
{
    const std::string& tooltip = csTooltip;

    // 将 tooltip 保存进 payload，包括 '\0'
    ImGui::SetDragDropPayload(DragDropType, tooltip.c_str(), tooltip.size() + 1, ImGuiCond_Always);
    ImGui::TextUnformatted(tooltip.c_str()); // 显示 tooltip
    ImGui::EndDragDropSource();
}

NATIVE_EXPORT_AUTO DragDrop_BeginTarget(cs_bool useWindow)
{
    if (useWindow)
    {
        // https://github.com/ocornut/imgui/issues/1771
        const ImGuiWindow* w = ImGui::GetCurrentWindowRead();
        retcs ImGui::BeginDragDropTargetCustom(w->ContentRegionRect, w->ID);
    }

    retcs ImGui::BeginDragDropTarget();
}

NATIVE_EXPORT_AUTO DragDrop_CheckPayload(cs<cs_bool*> outIsDelivery)
{
    ImGuiDragDropFlags flags = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragDropType, flags);

    if (payload == nullptr)
    {
        outIsDelivery->assign(false);
        retcs false;
    }

    outIsDelivery->assign(payload->IsDelivery());
    retcs true;
}

enum class DragDropResult
{
    Ignore,
    Reject,
    AcceptByRect,
    AcceptByLine,
};

NATIVE_EXPORT_AUTO DragDrop_AcceptTarget(cs<DragDropResult> result)
{
    ImGuiContext* context = ImGui::GetCurrentContext();
    DragDropResult res = result;

    if (res == DragDropResult::AcceptByRect)
    {
        context->DragDropAcceptFrameCountActual = context->FrameCount; // 真正接受 payload 的帧
        ImGui::RenderDragDropTargetRect(context->DragDropTargetRect, context->DragDropTargetClipRect, /* render_as_line */ false);
    }
    else if (res == DragDropResult::AcceptByLine)
    {
        context->DragDropAcceptFrameCountActual = context->FrameCount; // 真正接受 payload 的帧
        ImGui::RenderDragDropTargetRect(context->DragDropTargetRect, context->DragDropTargetClipRect, /* render_as_line */ true);
    }
    else if (res == DragDropResult::Reject)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
    }
}

NATIVE_EXPORT_AUTO DragDrop_EndTarget()
{
    ImGui::EndDragDropTarget();
}

NATIVE_EXPORT_AUTO DragDrop_GetIsActive()
{
    retcs ImGui::IsDragDropActive();
}
