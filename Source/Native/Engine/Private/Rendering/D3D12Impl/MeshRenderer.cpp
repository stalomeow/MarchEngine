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

    bool MeshRendererBatch::DrawCall::operator<(const DrawCall& other) const
    {
        Shader* shader1 = Mat->GetShader();
        Shader* shader2 = other.Mat->GetShader();

        // 按 Shader / Material / Mesh / HasOddNegativeScaling / SubMeshIndex 排序

        if (shader1 != shader2) return shader1 < shader2;
        if (Mat != other.Mat) return Mat < other.Mat;
        if (Mesh != other.Mesh) return Mesh < other.Mesh;
        if (HasOddNegativeScaling != other.HasOddNegativeScaling) return HasOddNegativeScaling < other.HasOddNegativeScaling;
        if (SubMeshIndex != other.SubMeshIndex) return SubMeshIndex < other.SubMeshIndex;

        return false;
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
        XMVECTOR det{};
        XMStoreFloat4x4(&currMatrixIT, XMMatrixTranspose(XMMatrixInverse(&det, XMLoadFloat4x4(&currMatrix))));

        XMFLOAT4 params{};
        params.x = std::copysignf(1.0f, XMVectorGetX(det)); // det < 0 时，x = -1.0f，否则 x = 1.0f

        return MeshRendererBatch::InstanceData{ currMatrix, currMatrixIT, prevMatrix, params };
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
        m_DrawCalls.clear();

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

                InstanceData instanceData = InstanceData::Create(renderer);
                DrawCall drawCall{ material, renderer->Mesh, subMesh, instanceData.HasOddNegativeScaling() };
                m_DrawCalls[drawCall].push_back(instanceData);
            }
        }
    }
}
