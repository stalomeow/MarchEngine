#include "RenderObject.h"
#include "WinApplication.h"
#include "RenderPipeline.h"

namespace march
{
    void RenderObject::OnMount()
    {
        Component::OnMount();
        GetApp().GetEngine()->GetRenderPipeline()->AddRenderObject(this);
    }

    void RenderObject::OnUnmount()
    {
        GetApp().GetEngine()->GetRenderPipeline()->RemoveRenderObject(this);
        Component::OnUnmount();
    }
}
