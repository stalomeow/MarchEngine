#include "pch.h"
#include "Engine/Rendering/D3D12Impl/MeshRenderer.h"
#include "Engine/Rendering/D3D12Impl/Material.h"
#include "Engine/Misc/MathUtils.h"
#include "Engine/Transform.h"
#include "Engine/JobManager.h"
#include <atomic>

using namespace DirectX;

namespace march
{
    MeshRenderer::MeshRenderer()
        : Mesh(nullptr)
        , Materials{}
        , m_PrevLocalToWorldMatrix(MathUtils::Identity4x4())
    {
    }

    BoundingBox MeshRenderer::GetBounds() const
    {
        BoundingBox result{};

        if (Mesh != nullptr)
        {
            Mesh->GetBounds().Transform(result, GetTransform()->LoadLocalToWorldMatrix());
        }

        return result;
    }

    XMFLOAT4X4 MeshRenderer::GetPrevLocalToWorldMatrix() const
    {
        return m_PrevLocalToWorldMatrix;
    }

    XMMATRIX MeshRenderer::LoadPrevLocalToWorldMatrix() const
    {
        return XMLoadFloat4x4(&m_PrevLocalToWorldMatrix);
    }

    void MeshRenderer::PrepareFrameData()
    {
        // 记录上一帧的 LocalToWorldMatrix
        m_PrevLocalToWorldMatrix = GetTransform()->GetLocalToWorldMatrix();
    }

    MeshRendererBatch::InstanceData MeshRendererBatch::InstanceData::Create(const MeshRenderer* renderer)
    {
        XMFLOAT4X4 currMatrix = renderer->GetTransform()->GetLocalToWorldMatrix();
        XMFLOAT4X4 prevMatrix = renderer->GetPrevLocalToWorldMatrix();
        return Create(currMatrix, prevMatrix);
    }

    MeshRendererBatch::InstanceData MeshRendererBatch::InstanceData::Create(const XMFLOAT4X4& currMatrix, const XMFLOAT4X4& prevMatrix)
    {
        XMFLOAT4X4 currMatrixIT{};
        XMStoreFloat4x4(&currMatrixIT, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&currMatrix))));
        return MeshRendererBatch::InstanceData{ currMatrix, currMatrixIT, prevMatrix };
    }

    static void CullMeshRenderers(std::vector<MeshRenderer*>& results, const MeshRendererBatch::FrustumType& frustum, size_t numRenderers, MeshRenderer* const* renderers)
    {
        std::atomic_size_t count = 0;
        results.resize(numRenderers);

        auto func = [&](size_t index)
        {
            // MeshRenderer 不是线程安全的，这里绝对不能修改 renderer
            const MeshRenderer* renderer = renderers[index];

            if (!renderer->GetIsActiveAndEnabled())
            {
                return;
            }

            if (renderer->Mesh == nullptr || renderer->Mesh->GetSubMeshCount() == 0 || renderer->Materials.empty())
            {
                return;
            }

            ContainmentType containment = std::visit([renderer](auto&& f)
            {
                return f.Contains(renderer->GetBounds());
            }, frustum);

            if (containment == ContainmentType::DISJOINT)
            {
                return;
            }

            results[count++] = const_cast<MeshRenderer*>(renderer);
        };

        constexpr size_t jobBatchSize = 4;

        if (numRenderers > jobBatchSize)
        {
            JobManager::Schedule(numRenderers, jobBatchSize, func).Complete();
        }
        else
        {
            for (size_t i = 0; i < numRenderers; i++)
            {
                func(i);
            }
        }

        results.resize(count);
    }

    void MeshRendererBatch::Rebuild(const FrustumType& frustum, size_t numRenderers, MeshRenderer* const* renderers)
    {
        m_Data.clear();

        static std::vector<MeshRenderer*> visibleRenderers{};
        CullMeshRenderers(visibleRenderers, frustum, numRenderers, renderers);

        for (MeshRenderer* renderer : visibleRenderers)
        {
            for (uint32_t subMesh = 0; subMesh < renderer->Mesh->GetSubMeshCount(); subMesh++)
            {
                Material* material = (subMesh < renderer->Materials.size())
                    ? renderer->Materials[subMesh] : renderer->Materials.back();

                if (material == nullptr || material->GetShader() == nullptr)
                {
                    continue;
                }

                DrawCall drawCall{ renderer->Mesh, subMesh };
                m_Data[material][drawCall].push_back(InstanceData::Create(renderer));
            }
        }
    }
}
