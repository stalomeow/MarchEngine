#pragma once

#include "RenderGraphPass.h"

namespace march
{
    class ColorTargetGuardPass : public RenderGraphPass
    {
        using base = typename RenderGraphPass;

    public:
        ColorTargetGuardPass();
        ~ColorTargetGuardPass() = default;

    protected:
        void OnSetup(RenderGraphBuilder& builder) override;
        void OnExecute() override;
    };
}
