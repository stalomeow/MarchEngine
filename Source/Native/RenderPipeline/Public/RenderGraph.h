#pragma once

#include "GfxTexture.h"
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace march
{
    class GfxResource;
    class RenderGraphPass;
    class RenderGraphResourcePool;

    enum class RenderGraphResourceType
    {
        Texture
    };

    struct RenderGraphResourceData final
    {
        std::vector<RenderGraphPass*> ProducerPasses;
        bool IsLifeTimeRecordStarted;
        int32_t LifeTimeMaxIndex;

        RenderGraphResourceType ResourceType;
        GfxResource* ResourcePtr;
        RenderGraphResourcePool* TransientResourcePool;
        GfxRenderTextureDesc TransientTextureDesc;

        RenderGraphResourceData(RenderGraphResourcePool* pool, const GfxRenderTextureDesc& desc);
        RenderGraphResourceData(GfxRenderTexture* texture);
        ~RenderGraphResourceData() = default;

        bool IsTransient() const;
        void CreateTransientResource();
        void DestroyTransientResource();
    };

    class RenderGraph
    {
    public:
        RenderGraph();
        ~RenderGraph() = default;

        void AddPersistentTexture(int32_t id, GfxRenderTexture* texture);
        void EnqueuePass(RenderGraphPass* pass);
        void ClearPasses();
        void CompileAndExecute();
        RenderGraphResourceData& GetResourceData(int32_t id);

    private:
        void SetupPasses();
        bool CompilePasses();
        void ExecutePasses();
        bool RecordPassResourceCreation(RenderGraphPass* pass);
        bool RecordTextureResourceData(int32_t id, const GfxRenderTextureDesc& desc);
        bool RecordPassRead(RenderGraphPass* pass);
        bool RecordPassWrite(RenderGraphPass* pass);
        bool CullAndSortPasses();
        bool CullAndSortPassesDFS(RenderGraphPass* pass);
        bool RecordResourceLifeTime();

        // 多次调用时 sortedPassIndex 必须是递增顺序
        bool UpdateResourceLifeTime(int32_t sortedPassIndex, const std::unordered_set<int32_t>& resourceIds);

        bool CreateOrDestroyResource(int32_t id, bool isDestroy);
        void AddPassResourceBarriers(RenderGraphPass* pass);

        std::vector<RenderGraphPass*> m_Passes;
        std::vector<RenderGraphPass*> m_SortedPasses;
        std::unordered_map<int32_t, RenderGraphResourceData> m_ResourceDataMap;
        std::unique_ptr<RenderGraphResourcePool> m_ResourcePool;
    };
}
