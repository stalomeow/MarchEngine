#include "pch.h"
#include "Editor/Gizmos.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO Gizmos_GetIsGUIMode()
{
    retcs Gizmos::IsGUIMode();
}

NATIVE_EXPORT_AUTO Gizmos_Clear()
{
    Gizmos::Clear();
}

NATIVE_EXPORT_AUTO Gizmos_PushMatrix(cs_mat4 matrix)
{
    Gizmos::PushMatrix(matrix);
}

NATIVE_EXPORT_AUTO Gizmos_PopMatrix()
{
    Gizmos::PopMatrix();
}

NATIVE_EXPORT_AUTO Gizmos_PushColor(cs_color color)
{
    Gizmos::PushColor(color);
}

NATIVE_EXPORT_AUTO Gizmos_PopColor()
{
    Gizmos::PopColor();
}

NATIVE_EXPORT_AUTO Gizmos_GetGUIScale(cs_vec3 position)
{
    retcs Gizmos::GetGUIScale(position);
}

NATIVE_EXPORT_AUTO Gizmos_DrawLine(cs_vec3 p1, cs_vec3 p2)
{
    Gizmos::DrawLine(p1, p2);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireArc(cs_vec3 center, cs_vec3 normal, cs_vec3 startDir, cs_float radians, cs_float radius)
{
    Gizmos::DrawWireArc(center, normal, startDir, radians, radius);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireDisc(cs_vec3 center, cs_vec3 normal, cs_float radius)
{
    Gizmos::DrawWireDisc(center, normal, radius);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireSphere(cs_vec3 center, cs_float radius)
{
    Gizmos::DrawWireSphere(center, radius);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireCube(cs_vec3 center, cs_vec3 size)
{
    Gizmos::DrawWireCube(center, size);
}

NATIVE_EXPORT_AUTO Gizmos_DrawText(cs_vec3 center, cs_string text)
{
    Gizmos::DrawText(center, text);
}

NATIVE_EXPORT_AUTO Gizmos_InitResources()
{
    GizmosManagedOnlyAPI::InitResources();
}

NATIVE_EXPORT_AUTO Gizmos_ReleaseResources()
{
    GizmosManagedOnlyAPI::ReleaseResources();
}
