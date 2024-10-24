#include "Gizmos.h"
#include "InteropServices.h"

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

NATIVE_EXPORT_AUTO Gizmos_AddLine(cs_vec3 p1, cs_vec3 p2)
{
    Gizmos::AddLine(p1, p2);
}

NATIVE_EXPORT_AUTO Gizmos_FlushAndDrawLines()
{
    Gizmos::FlushAndDrawLines();
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireArc(cs_vec3 center, cs_vec3 normal, cs_vec3 startDir, cs_float radians, cs_float radius)
{
    Gizmos::DrawWireArc(center, normal, startDir, radians, radius);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireCircle(cs_vec3 center, cs_vec3 normal, cs_float radius)
{
    Gizmos::DrawWireCircle(center, normal, radius);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireSphere(cs_vec3 center, cs_float radius)
{
    Gizmos::DrawWireSphere(center, radius);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireCube(cs_vec3 center, cs_vec3 size)
{
    Gizmos::DrawWireCube(center, size);
}

NATIVE_EXPORT_AUTO GizmosGUI_PushMatrix(cs_mat4 matrix)
{
    GizmosGUI::PushMatrix(matrix);
}

NATIVE_EXPORT_AUTO GizmosGUI_PopMatrix()
{
    GizmosGUI::PopMatrix();
}

NATIVE_EXPORT_AUTO GizmosGUI_PushColor(cs_color color)
{
    GizmosGUI::PushColor(color);
}

NATIVE_EXPORT_AUTO GizmosGUI_PopColor()
{
    GizmosGUI::PopColor();
}

NATIVE_EXPORT_AUTO GizmosGUI_DrawText(cs_vec3 center, cs_string text)
{
    GizmosGUI::DrawText(center, text);
}
