#include "RenderObject.h"
#include "Application.h"
#include "RenderPipeline.h"

namespace march
{
    void RenderObject::OnMount()
    {
        Component::OnMount();
        GetApp()->GetRenderPipeline()->AddRenderObject(this);
    }

    void RenderObject::OnUnmount()
    {
        GetApp()->GetRenderPipeline()->RemoveRenderObject(this);
        Component::OnUnmount();
    }
}
