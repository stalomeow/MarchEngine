#pragma once

#include <directx/d3dx12.h>
#include "Rendering/DxException.h"
#include "Rendering/Resource/UploadHeapAllocator.h"
#include "Rendering/GfxManager.h"
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <DirectXColors.h>

namespace dx12demo
{
    class Mesh
    {
    protected:
        struct SubMesh
        {
            INT BaseVertexLocation;
            UINT StartIndexLocation;
            UINT IndexCount;
        };

    protected:
        Mesh() = default;
        Mesh(const Mesh& rhs) = delete;
        Mesh& operator=(const Mesh& rhs) = delete;

    public:
        virtual ~Mesh() = default;

        // subMeshIndex = -1 means draw all sub-meshes
        virtual void Draw(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int subMeshIndex = -1) = 0;
    };

    template<typename TVertex, typename TIndex>
    class MeshImpl : public Mesh
    {
        static_assert(sizeof(TIndex) == 2 || sizeof(TIndex) == 4, "TIndex must be 2 or 4 bytes in size.");

    public:
        MeshImpl(D3D12_PRIMITIVE_TOPOLOGY topology);
        MeshImpl(const MeshImpl& rhs) = delete;
        MeshImpl& operator=(const MeshImpl& rhs) = delete;
        ~MeshImpl() override = default;

        void ClearSubMeshes();
        void AddSubMesh(const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices);

        void Draw(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int subMeshIndex = -1) override;

        static D3D12_INPUT_LAYOUT_DESC VertexInputLayout()
        {
            D3D12_INPUT_LAYOUT_DESC desc = {};
            desc.pInputElementDescs = TVertex::InputDesc;
            desc.NumElements = _countof(TVertex::InputDesc);
            return desc;
        }

        int SubMeshCount() const { return m_SubMeshes.size(); }

        D3D12_PRIMITIVE_TOPOLOGY Topology() const { return m_Topology; }

    private:
        D3D12_PRIMITIVE_TOPOLOGY m_Topology;

        std::vector<SubMesh> m_SubMeshes{};
        std::vector<TVertex> m_Vertices{};
        std::vector<TIndex> m_Indices{};
        bool m_IsDirty = false;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer = nullptr;
    };

    template<typename TVertex, typename TIndex>
    MeshImpl<TVertex, TIndex>::MeshImpl(D3D12_PRIMITIVE_TOPOLOGY topology)
        : m_Topology(topology)
    {
    }

    namespace
    {
        using namespace Microsoft::WRL;

        void RecreateBufferAndUpload(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* cmdList,
            ComPtr<ID3D12Resource>& dest,
            const void* pData,
            UINT64 size)
        {
            THROW_IF_FAILED(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(size),
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(dest.ReleaseAndGetAddressOf())));

            D3D12_SUBRESOURCE_DATA subResData;
            subResData.pData = pData;
            subResData.RowPitch = size;
            subResData.SlicePitch = size;

            // Vertex Buffer 和 Index Buffer 都放在默认堆，优化性能
            // 上传数据时，借助上传堆中转
            UploadHeapSpan span = GetGfxManager().AllocateUploadHeap(size);
            UpdateSubresources<1>(cmdList, dest.Get(), span.GetResource(), span.GetOffsetInResource(), 0, 1, &subResData);
            span.Free(GetGfxManager().GetNextFenceValue());

            cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dest.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
        }
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::ClearSubMeshes()
    {
        if (m_SubMeshes.size() > 0)
        {
            m_IsDirty = true;
        }

        m_SubMeshes.clear();
        m_Vertices.clear();
        m_Indices.clear();
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::AddSubMesh(const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices)
    {
        SubMesh subMesh;
        subMesh.BaseVertexLocation = m_Vertices.size();
        subMesh.IndexCount = indices.size();
        subMesh.StartIndexLocation = m_Indices.size();

        m_IsDirty = true;
        m_SubMeshes.push_back(subMesh);
        m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
        m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::Draw(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int subMeshIndex)
    {
        if (m_IsDirty)
        {
            m_IsDirty = false;

            UINT64 vertexBufferSize = sizeof(TVertex) * m_Vertices.size();
            RecreateBufferAndUpload(device, cmdList, m_VertexBuffer, m_Vertices.data(), vertexBufferSize);

            UINT64 indexBufferSize = sizeof(TIndex) * m_Indices.size();
            RecreateBufferAndUpload(device, cmdList, m_IndexBuffer, m_Indices.data(), indexBufferSize);
        }

        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        vbv.SizeInBytes = sizeof(TVertex) * m_Vertices.size();
        vbv.StrideInBytes = sizeof(TVertex);

        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = sizeof(TIndex) * m_Indices.size();
        ibv.Format = sizeof(TIndex) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);
        cmdList->IASetPrimitiveTopology(m_Topology);

        if (subMeshIndex == -1)
        {
            for (auto& subMesh : m_SubMeshes)
            {
                cmdList->DrawIndexedInstanced(subMesh.IndexCount, 1,
                    subMesh.StartIndexLocation, subMesh.BaseVertexLocation, 0);
            }
        }
        else
        {
            auto& subMesh = m_SubMeshes[subMeshIndex];
            cmdList->DrawIndexedInstanced(subMesh.IndexCount, 1,
                subMesh.StartIndexLocation, subMesh.BaseVertexLocation, 0);
        }
    }

    struct SimpleMeshVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT4 Color;

        static constexpr D3D12_INPUT_ELEMENT_DESC InputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
    };

    class SimpleMesh : public MeshImpl<SimpleMeshVertex, std::uint16_t>
    {
    public:
        SimpleMesh() : MeshImpl<SimpleMeshVertex, std::uint16_t>(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {}
        SimpleMesh(const SimpleMesh& rhs) = delete;
        SimpleMesh& operator=(const SimpleMesh& rhs) = delete;
        ~SimpleMesh() override = default;

        void AddSubMeshCube()
        {
            std::vector<SimpleMeshVertex> vertices =
            {
                { DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) },
                { DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) },
                { DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) },
                { DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) },
                { DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) },
                { DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) },
                { DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) },
                { DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) }
            };

            std::vector<std::uint16_t> indices =
            {
                // front face
                0, 1, 2,
                0, 2, 3,

                // back face
                4, 6, 5,
                4, 7, 6,

                // left face
                4, 5, 1,
                4, 1, 0,

                // right face
                3, 2, 6,
                3, 6, 7,

                // top face
                1, 5, 6,
                1, 6, 2,

                // bottom face
                4, 0, 3,
                4, 3, 7
            };

            AddSubMesh(vertices, indices);
        }
    };
}
