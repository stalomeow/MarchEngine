#include "ColorTargetGuardPass.h"
#include "RenderGraphFrameData.h"

namespace march
{
    ColorTargetGuardPass::ColorTargetGuardPass() : base("ColorTargetGuard")
    {
    }

    void ColorTargetGuardPass::OnSetup(RenderGraphBuilder& builder)
    {
        builder.AllowPassCulling(false);
        builder.ReadTexture(RenderGraphFrameData::CameraColorTarget);
    }

    void ColorTargetGuardPass::OnExecute()
    {
    }
}
