#include "pch.h"
#include "Engine/Scripting/InteropServices.h"
#include "imgui.h"
#include "imgui_internal.h"

static const char* DragDropType = "march_drag_drop";

NATIVE_EXPORT_AUTO DragDrop_BeginSource()
{
    retcs ImGui::BeginDragDropSource();
}

NATIVE_EXPORT_AUTO DragDrop_EndSource(cs_string tooltip)
{
    ImGui::TextUnformatted(tooltip.c_str()); // 显示 tooltip
    ImGui::SetDragDropPayload(DragDropType, nullptr, 0);
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
    DragDropResult res = result;

    if (res == DragDropResult::AcceptByRect)
    {
        ImGuiContext* context = ImGui::GetCurrentContext();
        ImGui::RenderDragDropTargetRect(context->DragDropTargetRect, context->DragDropTargetClipRect, /* render_as_line */ false);
    }
    else if (res == DragDropResult::AcceptByLine)
    {
        ImGuiContext* context = ImGui::GetCurrentContext();
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
