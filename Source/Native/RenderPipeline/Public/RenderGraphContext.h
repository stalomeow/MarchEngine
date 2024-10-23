#pragma once

#include "GfxTexture.h"
#include "PipelineState.h"
#include <directx/d3d12.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <stdint.h>

namespace march
{
    class GfxDevice;
    class GfxCommandList;
    class GfxMesh;
    class RenderGraph;
    class Material;
    class ShaderPass;
    class RenderObject;

    enum class RenderTargetClearFlags
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        DepthStencil = Depth | Stencil,
        All = Color | Depth | Stencil,
    };

    RenderTargetClearFlags operator|(RenderTargetClearFlags lhs, RenderTargetClearFlags rhs);
    RenderTargetClearFlags& operator|=(RenderTargetClearFlags& lhs, RenderTargetClearFlags rhs);
    RenderTargetClearFlags operator&(RenderTargetClearFlags lhs, RenderTargetClearFlags rhs);
    RenderTargetClearFlags& operator&=(RenderTargetClearFlags& lhs, RenderTargetClearFlags rhs);

    struct MeshBufferDesc
    {
        D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
        D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    };

    class RenderGraphContext
    {
        friend RenderGraph;

    public:
        RenderGraphContext();
        ~RenderGraphContext() = default;

        RenderGraphContext(const RenderGraphContext&) = delete;
        RenderGraphContext& operator=(const RenderGraphContext&) = delete;
        RenderGraphContext(RenderGraphContext&&) = delete;
        RenderGraphContext& operator=(RenderGraphContext&&) = delete;

        GfxDevice* GetDevice() const;
        GfxCommandList* GetGraphicsCommandList() const;
        ID3D12GraphicsCommandList* GetD3D12GraphicsCommandList() const;

        RenderPipelineDesc GetRenderPipelineDesc(bool wireframe) const;

        void SetGlobalConstantBuffer(const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address);
        void SetGlobalConstantBuffer(int32_t id, D3D12_GPU_VIRTUAL_ADDRESS address);

        void SetTexture(const std::string& name, GfxTexture* texture);
        void SetTexture(int32_t id, GfxTexture* texture);

        // 如果 subMeshIndex 为 -1，则绘制所有子网格
        void DrawMesh(GfxMesh* mesh, Material* material, bool wireframe = false, int32_t subMeshIndex = -1, int32_t shaderPassIndex = 0);

        D3D12_VERTEX_BUFFER_VIEW CreateTransientVertexBuffer(size_t vertexCount, size_t vertexStride, size_t vertexAlignment, const void* verticesData);
        D3D12_INDEX_BUFFER_VIEW CreateTransientIndexBuffer(size_t indexCount, const uint16_t* indexData);
        D3D12_INDEX_BUFFER_VIEW CreateTransientIndexBuffer(size_t indexCount, const uint32_t* indexData);
        void DrawMesh(const D3D12_INPUT_LAYOUT_DESC* inputLayout, D3D12_PRIMITIVE_TOPOLOGY topology, const MeshBufferDesc* bufferDesc, Material* material, bool wireframe = false, int32_t shaderPassIndex = 0);

        void DrawObjects(size_t numObjects, const RenderObject* const* objects, bool wireframe = false, int32_t shaderPassIndex = 0);

    private:
        // 如果 viewport 为 nullptr，则使用默认 viewport
        // 如果 scissorRect 为 nullptr，则使用默认 scissorRect
        void SetRenderTargets(int32_t numColorTargets, GfxRenderTexture* const* colorTargets,
            GfxRenderTexture* depthStencilTarget, const D3D12_VIEWPORT* viewport, const D3D12_RECT* scissorRect);
        void ClearRenderTargets(RenderTargetClearFlags flags, const float color[4], float depth, uint8_t stencil);

        D3D12_VIEWPORT GetDefaultViewport() const;
        D3D12_RECT GetDefaultScissorRect() const;

        size_t GetPipelineStateHash(const MeshDesc& meshDesc, Material* material, int32_t shaderPassIndex);
        void SetPipelineStateAndRootSignature(const MeshDesc& meshDesc, Material* material, bool wireframe, int32_t shaderPassIndex);
        void BindResources(Material* material, int32_t shaderPassIndex, D3D12_GPU_VIRTUAL_ADDRESS perObjectConstantBufferAddress);

        void ClearPreviousPassData();
        void Reset();

        std::vector<GfxRenderTexture*> m_ColorTargets;
        GfxRenderTexture* m_DepthStencilTarget;
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;
        ID3D12PipelineState* m_CurrentPipelineState;
        ID3D12RootSignature* m_CurrentRootSignature;
        std::unordered_map<int32_t, D3D12_GPU_VIRTUAL_ADDRESS> m_GlobalConstantBuffers;
        std::unordered_map<int32_t, GfxTexture*> m_PassTextures;
    };
}
