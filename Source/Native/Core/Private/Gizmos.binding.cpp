#include "Gizmos.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO Gizmos_DrawText(cs_vec3 centerWS, cs_string text, cs_color color)
{
    Gizmos::DrawText(centerWS, text, color);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireArc(cs_vec3 centerWS, cs_vec3 normalWS, cs_vec3 startDirWS, float angleDeg, float radius, cs_color color)
{
    Gizmos::DrawWireArc(centerWS, normalWS, startDirWS, angleDeg, radius, color);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireCircle(cs_vec3 centerWS, cs_vec3 normalWS, float radius, cs_color color)
{
    Gizmos::DrawWireCircle(centerWS, normalWS, radius, color);
}

NATIVE_EXPORT_AUTO Gizmos_DrawWireSphere(cs_vec3 centerWS, float radius, cs_color color)
{
    Gizmos::DrawWireSphere(centerWS, radius, color);
}

NATIVE_EXPORT_AUTO Gizmos_DrawLine(cs_vec3 posWS1, cs_vec3 posWS2, cs_color color)
{
    Gizmos::DrawLine(posWS1, posWS2, color);
}
