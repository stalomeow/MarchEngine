#pragma once

#include "EditorWindow.h"
#include "Engine/Ints.h"
#include "Engine/Rendering/RenderGraph.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace march
{
    class RenderGraphViewerWindow : public EditorWindow, public IRenderGraphCompiledEventListener
    {
        using base = EditorWindow;

        enum class PassStatus
        {
            Normal,
            Culled,
            AsyncCompute,
            Deadline,
        };

        struct PassData
        {
            std::string FullName{};
            std::string ShortName{};
            PassStatus Status = PassStatus::Normal;
            bool IsAsyncComputeBatchedWithPrevious = false;
            std::string DeadlineOwnerPassName{};
        };

        enum class ResourceAccessFlags
        {
            None = 0,
            Read = 1 << 0,
            Write = 1 << 1,
            ReadWrite = Read | Write,
        };

        friend ResourceAccessFlags& operator|=(ResourceAccessFlags& a, ResourceAccessFlags b)
        {
            return a = static_cast<ResourceAccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        struct ResourceData
        {
            std::string Name{};
            bool IsExternal = false;

            bool HasLifetime = false;
            size_t LifetimeMinIndex{};
            size_t LifetimeMaxIndex{};

            std::unordered_map<size_t, ResourceAccessFlags> PassAccessFlags{}; // key is pass index
        };

        std::vector<PassData> m_Passes{};
        std::vector<ResourceData> m_Resources{};

        void DrawAccessSquare(ResourceAccessFlags accessFlags);

    public:
        void OnGraphCompiled(const std::vector<RenderGraphPass>& passes, const RenderGraphResourceManager* resourceManager) override;

    protected:
        bool Begin() override;
        void OnOpen() override;
        void OnClose() override;
        void OnDraw() override;
    };
}
