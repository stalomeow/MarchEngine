#pragma once

#include "Core/GameObject.h"
#include <vector>
#include <memory>

namespace dx12demo
{
    class Scene
    {
    public:
        std::vector<std::unique_ptr<GameObject>> GameObjects{};
    };
}
