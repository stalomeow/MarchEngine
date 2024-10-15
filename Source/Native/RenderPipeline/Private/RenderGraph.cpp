#include "RenderGraph.h"
#include "RenderGraphPass.h"
#include "RenderGraphResourcePool.h"
#include "Debug.h"
#include <utility>
#include <algorithm>
#include <stdexcept>

#undef min
#undef max

namespace march
{
    RenderGraphResourceData::RenderGraphResourceData(RenderGraphResourcePool* pool, const GfxRenderTextureDesc& desc)
        : ProducerPasses{}
        , IsLifeTimeRecordStarted(false)
        , LifeTimeMaxIndex(0)
        , ResourceType(RenderGraphResourceType::Texture)
        , ResourcePtr(nullptr)
        , TransientResourcePool(pool)
        , TransientTextureDesc(desc)
    {
    }

    RenderGraphResourceData::RenderGraphResourceData(GfxRenderTexture* texture)
        : ProducerPasses{}
        , IsLifeTimeRecordStarted(false)
        , LifeTimeMaxIndex(0)
        , ResourceType(RenderGraphResourceType::Texture)
        , ResourcePtr(texture)
        , TransientResourcePool(nullptr)
        , TransientTextureDesc{}
    {
    }

    bool RenderGraphResourceData::IsTransient() const
    {
        return TransientResourcePool != nullptr;
    }

    void RenderGraphResourceData::CreateTransientResource()
    {
        if (!IsTransient())
        {
            return;
        }

        if (ResourceType == RenderGraphResourceType::Texture)
        {
            ResourcePtr = TransientResourcePool->RentTexture(TransientTextureDesc);
        }
    }

    void RenderGraphResourceData::DestroyTransientResource()
    {
        if (!IsTransient())
        {
            return;
        }

        if (ResourceType == RenderGraphResourceType::Texture)
        {
            TransientResourcePool->ReturnTexture(static_cast<GfxRenderTexture*>(ResourcePtr));
        }
    }

    RenderGraph::RenderGraph()
        : m_Passes{}
        , m_SortedPasses{}
        , m_ResourceDataMap{}
    {
        m_ResourcePool = std::make_unique<RenderGraphResourcePool>();
    }

    void RenderGraph::AddPersistentTexture(int32_t id, GfxRenderTexture* texture)
    {
        m_ResourceDataMap[id] = RenderGraphResourceData(texture);
    }

    void RenderGraph::EnqueuePass(RenderGraphPass* pass)
    {
        m_Passes.push_back(pass);
    }

    void RenderGraph::ClearPasses()
    {
        m_Passes.clear();
    }

    void RenderGraph::CompileAndExecute()
    {
        SetupPasses();

        if (!CompilePasses())
        {
            return;
        }

        ExecutePasses();

        // Clean up
        m_Passes.clear();
        m_SortedPasses.clear();
        m_ResourceDataMap.clear();
    }

    void RenderGraph::SetupPasses()
    {
        for (RenderGraphPass* pass : m_Passes)
        {
            RenderGraphBuilder builder(this, pass);
            pass->OnSetup(builder);
        }
    }

    bool RenderGraph::CompilePasses()
    {
        for (RenderGraphPass* pass : m_Passes)
        {
            if (!RecordPassResourceCreation(pass) || !RecordPassRead(pass) || !RecordPassWrite(pass))
            {
                return false;
            }
        }

        if (!CullAndSortPasses())
        {
            return false;
        }

        if (!RecordResourceLifeTime())
        {
            return false;
        }

        return true;
    }

    void RenderGraph::ExecutePasses()
    {
        for (RenderGraphPass* pass : m_SortedPasses)
        {
            for (int32_t id : pass->m_ResourcesBorn)
            {
                if (!CreateOrDestroyResource(id, false))
                {
                    DEBUG_LOG_ERROR("Failed to create resource %d for pass %s", id, pass->m_Name.c_str());
                    return;
                }
            }

            pass->OnExecute();

            for (int32_t id : pass->m_ResourcesDead)
            {
                if (!CreateOrDestroyResource(id, true))
                {
                    DEBUG_LOG_ERROR("Failed to release resource %d for pass %s", id, pass->m_Name.c_str());
                    return;
                }
            }
        }
    }

    RenderGraphResourceData& RenderGraph::GetResourceData(int32_t id)
    {
        auto it = m_ResourceDataMap.find(id);

        if (it == m_ResourceDataMap.end())
        {
            throw std::runtime_error("Resource data not found");
        }

        return it->second;
    }

    bool RenderGraph::RecordPassResourceCreation(RenderGraphPass* pass)
    {
        // 记录 pass 创建的资源

        for (std::pair<const int32_t, GfxRenderTextureDesc>& kv : pass->m_TexturesCreated)
        {
            if (!RecordTextureResourceData(kv.first, kv.second))
            {
                DEBUG_LOG_ERROR("Failed to record texture resource data for pass %s", pass->m_Name.c_str());
                return false;
            }
        }

        return false;
    }

    bool RenderGraph::RecordTextureResourceData(int32_t id, const GfxRenderTextureDesc& desc)
    {
        if (m_ResourceDataMap.find(id) != m_ResourceDataMap.end())
        {
            return false;
        }

        m_ResourceDataMap.emplace(id, RenderGraphResourceData(m_ResourcePool.get(), desc));
        return true;
    }

    bool RenderGraph::RecordPassRead(RenderGraphPass* pass)
    {
        // 设置 pass 为资源的 consumer

        for (int32_t id : pass->m_ResourcesRead)
        {
            if (pass->m_ResourcesWritten.count(id) > 0)
            {
                DEBUG_LOG_ERROR("Resource %d is both read and written in pass %s", id, pass->m_Name.c_str());
                return false;
            }

            auto it = m_ResourceDataMap.find(id);

            if (it == m_ResourceDataMap.end() || it->second.ProducerPasses.empty())
            {
                DEBUG_LOG_ERROR("Failed to find producer pass for resource %d in pass %s", id, pass->m_Name.c_str());
                return false;
            }

            RenderGraphPass* producerPass = it->second.ProducerPasses.back();
            producerPass->m_NextPasses.push_back(pass);
        }
    }

    bool RenderGraph::RecordPassWrite(RenderGraphPass* pass)
    {
        // 设置 pass 为资源的 producer

        for (int32_t id : pass->m_ResourcesWritten)
        {
            if (pass->m_ResourcesRead.count(id) > 0)
            {
                DEBUG_LOG_ERROR("Resource %d is both read and written in pass %s", id, pass->m_Name.c_str());
                return false;
            }

            auto it = m_ResourceDataMap.find(id);

            if (it == m_ResourceDataMap.end())
            {
                DEBUG_LOG_ERROR("Failed to set producer pass for resource %d in pass %s; resource is not created", id, pass->m_Name.c_str());
                return false;
            }

            it->second.ProducerPasses.push_back(pass);
        }

        return true;
    }

    bool RenderGraph::CullAndSortPasses()
    {
        // 资源是从零入度 pass 开始向后传递的，所以从零入度 pass 开始做 dfs 拓扑排序，尽可能减少资源的生命周期

        for (RenderGraphPass* pass : m_Passes)
        {
            if (pass->m_ResourcesRead.empty() && pass->m_SortState == RenderGraphPassSortState::None)
            {
                if (!CullAndSortPassesDFS(pass))
                {
                    return false;
                }
            }
        }

        std::reverse(m_SortedPasses.begin(), m_SortedPasses.end());
        return true;
    }

    bool RenderGraph::CullAndSortPassesDFS(RenderGraphPass* pass)
    {
        pass->m_SortState = RenderGraphPassSortState::Visiting;
        int32_t outdegree = 0;

        for (RenderGraphPass* adj : pass->m_NextPasses)
        {
            if (adj->m_SortState == RenderGraphPassSortState::Visiting)
            {
                DEBUG_LOG_ERROR("Cycle detected in render graph");
                return false;
            }

            if (adj->m_SortState == RenderGraphPassSortState::None)
            {
                if (!CullAndSortPassesDFS(adj))
                {
                    return false;
                }
            }

            if (adj->m_SortState != RenderGraphPassSortState::Culled)
            {
                outdegree++;
            }
        }

        if (outdegree < 0 || outdegree > static_cast<int32_t>(pass->m_NextPasses.size()))
        {
            DEBUG_LOG_ERROR("Invalid outdegree %d for pass %s", outdegree, pass->m_Name.c_str());
            return false;
        }

        if (outdegree == 0 && pass->m_AllowPassCulling)
        {
            pass->m_SortState = RenderGraphPassSortState::Culled;
            return true;
        }
        else
        {
            pass->m_SortState = RenderGraphPassSortState::Visited;
            m_SortedPasses.push_back(pass);
            return true;
        }
    }

    bool RenderGraph::RecordResourceLifeTime()
    {
        for (int32_t i = 0; i < m_SortedPasses.size(); i++)
        {
            if (!UpdateResourceLifeTime(i, m_SortedPasses[i]->m_ResourcesWritten))
            {
                return false;
            }

            if (!UpdateResourceLifeTime(i, m_SortedPasses[i]->m_ResourcesRead))
            {
                return false;
            }
        }

        for (auto& kv : m_ResourceDataMap)
        {
            RenderGraphResourceData& data = kv.second;

            if (!data.IsTransient())
            {
                continue;
            }

            if (!data.IsLifeTimeRecordStarted)
            {
                DEBUG_LOG_ERROR("Resource %d is not used in any pass", kv.first);
                return false;
            }

            int32_t id = kv.first;
            m_SortedPasses[data.LifeTimeMaxIndex]->m_ResourcesDead.push_back(id);
        }

        return true;
    }

    bool RenderGraph::UpdateResourceLifeTime(int32_t sortedPassIndex, const std::unordered_set<int32_t>& resourceIds)
    {
        for (int32_t id : resourceIds)
        {
            auto it = m_ResourceDataMap.find(id);

            if (it == m_ResourceDataMap.end())
            {
                DEBUG_LOG_ERROR("Failed to find resource data for resource %d", id);
                return false;
            }

            RenderGraphResourceData& data = it->second;

            if (!data.IsTransient())
            {
                continue;
            }

            if (data.IsLifeTimeRecordStarted)
            {
                data.LifeTimeMaxIndex = sortedPassIndex; // 已经假定 sortedPassIndex 是递增的
            }
            else
            {
                data.IsLifeTimeRecordStarted = true;
                data.LifeTimeMaxIndex = sortedPassIndex;
                m_SortedPasses[sortedPassIndex]->m_ResourcesBorn.push_back(id);
            }
        }
    }

    bool RenderGraph::CreateOrDestroyResource(int32_t id, bool isDestroy)
    {
        auto it = m_ResourceDataMap.find(id);

        if (it == m_ResourceDataMap.end())
        {
            DEBUG_LOG_ERROR("Failed to find resource data for resource %d", id);
            return false;
        }

        RenderGraphResourceData& data = it->second;

        if (isDestroy)
        {
            data.DestroyTransientResource();
        }
        else
        {
            data.CreateTransientResource();
        }

        return true;
    }
}
