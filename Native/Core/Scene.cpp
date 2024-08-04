#include "Core/Scene.h"

namespace dx12demo
{
    std::unique_ptr<Scene> Scene::s_Current = std::make_unique<Scene>();
}
