#include "RenderObject.h"
#include "Application.h"
#include "RenderPipeline.h"
#include "Transform.h"

using namespace DirectX;

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

    BoundingBox RenderObject::GetBounds() const
    {
        BoundingBox result = {};

        if (Mesh != nullptr)
        {
            Mesh->GetBounds().Transform(result, GetTransform()->LoadLocalToWorldMatrix());
        }

        return result;
    }
}
