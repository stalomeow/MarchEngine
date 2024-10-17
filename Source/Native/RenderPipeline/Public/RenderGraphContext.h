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

        // 如果 subMeshIndex 为 -1，则绘制所有子网格
        void DrawMesh(GfxMesh* mesh, Material* material, bool wireframe = false,
            int32_t subMeshIndex = -1, int32_t shaderPassIndex = 0,
            size_t numConstantBuffers = 0, int32_t* constantBufferIds = nullptr, D3D12_GPU_VIRTUAL_ADDRESS* constantBufferAddresses = nullptr);

        void DrawObjects(size_t numObjects, const RenderObject* const* objects, bool wireframe = false, int32_t shaderPassIndex = 0,
            size_t numConstantBuffers = 0, const int32_t* constantBufferIds = nullptr, const D3D12_GPU_VIRTUAL_ADDRESS* constantBufferAddresses = nullptr);

    private:
        // 如果 viewport 为 nullptr，则使用默认 viewport
        // 如果 scissorRect 为 nullptr，则使用默认 scissorRect
        void SetRenderTargets(size_t numColorTargets, GfxRenderTexture* const* colorTargets,
            GfxRenderTexture* depthStencilTarget, const D3D12_VIEWPORT* viewport, const D3D12_RECT* scissorRect);
        void ClearRenderTargets(RenderTargetClearFlags flags, const float color[4], float depth, uint8_t stencil);

        D3D12_VIEWPORT GetDefaultViewport() const;
        D3D12_RECT GetDefaultScissorRect() const;

        size_t GetPipelineStateHash(GfxMesh* mesh, Material* material, int32_t shaderPassIndex);
        void SetPipelineStateAndRootSignature(GfxMesh* mesh, Material* material, bool wireframe, int32_t shaderPassIndex);
        void BindResources(Material* material, int32_t shaderPassIndex, D3D12_GPU_VIRTUAL_ADDRESS perObjectConstantBufferAddress,
            size_t numExtraConstantBuffers, const int32_t* extraConstantBufferIds, const D3D12_GPU_VIRTUAL_ADDRESS* extraConstantBufferAddresses);

        void Reset();

        std::vector<GfxRenderTexture*> m_ColorTargets;
        GfxRenderTexture* m_DepthStencilTarget;
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;
        std::unordered_map<int32_t, D3D12_GPU_VIRTUAL_ADDRESS> m_GlobalConstantBuffers;
    };
}
