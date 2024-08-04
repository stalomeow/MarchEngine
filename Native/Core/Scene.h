#pragma once

#include "Core/GameObject.h"
#include <vector>
#include <string>
#include <memory>

namespace dx12demo
{
    class Scene
    {
    public:
        static Scene* GetCurrent() { return s_Current.get(); }

    public:
        std::string Name = u8"New Scene";
        std::vector<std::unique_ptr<GameObject>> GameObjects{};

    private:
        static std::unique_ptr<Scene> s_Current;
    };
}
