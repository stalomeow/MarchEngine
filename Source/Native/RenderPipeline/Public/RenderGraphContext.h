#pragma once

#include "GfxTexture.h"
#include "GfxPipelineState.h"
#include "Shader.h"
#include <directx/d3d12.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <stdint.h>
#include <functional>

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

    struct MeshDesc
    {
        const GfxInputDesc* InputDesc;
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

        void BeginEvent(const std::string& name) const;
        void EndEvent() const;

        void SetGlobalConstantBuffer(const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address);
        void SetGlobalConstantBuffer(int32_t id, D3D12_GPU_VIRTUAL_ADDRESS address);

        void SetTexture(const std::string& name, GfxTexture* texture);
        void SetTexture(int32_t id, GfxTexture* texture);

        D3D12_VERTEX_BUFFER_VIEW CreateTransientVertexBuffer(size_t vertexCount, size_t vertexStride, size_t vertexAlignment, const void* verticesData);
        D3D12_INDEX_BUFFER_VIEW CreateTransientIndexBuffer(size_t indexCount, const uint16_t* indexData);
        D3D12_INDEX_BUFFER_VIEW CreateTransientIndexBuffer(size_t indexCount, const uint32_t* indexData);

        // 如果 subMeshIndex 为 -1，则绘制所有子网格
        void DrawMesh(GfxMesh* mesh, Material* material, int32_t shaderPassIndex = 0, int32_t subMeshIndex = -1);
        void DrawMesh(const MeshDesc* meshDesc, Material* material, int32_t shaderPassIndex = 0);
        void DrawObjects(size_t numObjects, const RenderObject* const* objects, int32_t shaderPassIndex = 0);

        // 如果 subMeshIndex 为 -1，则绘制所有子网格
        void DrawMesh(GfxMesh* mesh, Material* material, const std::string& lightMode, int32_t subMeshIndex = -1);
        void DrawMesh(const MeshDesc* meshDesc, Material* material, const std::string& lightMode);
        void DrawObjects(size_t numObjects, const RenderObject* const* objects, const std::string& lightMode);

    private:
        void DrawObjects(size_t numObjects, const RenderObject* const* objects, std::function<int32_t(const Shader*)> getPassIndex);

        // 如果 viewport 为 nullptr，则使用默认 viewport
        // 如果 scissorRect 为 nullptr，则使用默认 scissorRect
        void SetRenderTargets(int32_t numColorTargets, GfxRenderTexture* const* colorTargets,
            GfxRenderTexture* depthStencilTarget, const D3D12_VIEWPORT* viewport, const D3D12_RECT* scissorRect);
        void ClearRenderTargets(RenderTargetClearFlags flags, const float color[4], float depth, uint8_t stencil);
        void SetWireframe(bool value);

        D3D12_VIEWPORT GetDefaultViewport() const;
        D3D12_RECT GetDefaultScissorRect() const;

        ID3D12PipelineState* GetPipelineState(Material* material, int32_t passIndex, const GfxInputDesc& inputDesc);
        void SetPipelineStateAndRootSignature(ID3D12PipelineState* pso, ShaderPass* pass, Material* material);
        void BindResources(Material* material, int32_t shaderPassIndex, D3D12_GPU_VIRTUAL_ADDRESS perObjectConstantBufferAddress);

        void ClearPreviousPassData();
        void Reset();

        std::vector<GfxRenderTexture*> m_ColorTargets;
        GfxRenderTexture* m_DepthStencilTarget;
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;

        GfxOutputDesc m_OutputDesc;

        ID3D12PipelineState* m_CurrentPipelineState;
        ID3D12RootSignature* m_CurrentRootSignature;
        D3D12_PRIMITIVE_TOPOLOGY m_CurrentPrimitiveTopology;
        uint8_t m_CurrentStencilRef;
        bool m_IsStencilRefSet;
        std::unordered_map<int32_t, D3D12_GPU_VIRTUAL_ADDRESS> m_GlobalConstantBuffers;
        std::unordered_map<int32_t, GfxTexture*> m_PassTextures;
    };
}
