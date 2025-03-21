#pragma once

#include "Engine/Ints.h"
#include "Engine/InlineArray.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderGraphImpl/RenderGraphResource.h"
#include <directx/d3d12.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <DirectXColors.h>

namespace march
{
    class RenderGraph;
    class RenderGraphContext;

    enum class RenderTargetInitMode
    {
        Load,    // 需要使用之前的内容
        Discard, // 不需要使用之前的内容
        Clear,   // 清除之前的内容
    };

    struct RenderGraphPassRenderTarget
    {
        size_t ResourceIndex = 0;
        bool IsSet = false;

        GfxCubemapFace Face = GfxCubemapFace::PositiveX;
        uint32_t WOrArraySlice = 0;
        uint32_t MipSlice = 0;

        RenderTargetInitMode InitMode = RenderTargetInitMode::Load;
    };

    struct RenderGraphPassColorTarget : public RenderGraphPassRenderTarget
    {
        float ClearColor[4]{};
    };

    struct RenderGraphPassDepthStencilTarget : public RenderGraphPassRenderTarget
    {
        float ClearDepthValue = GfxUtils::FarClipPlaneDepth;
        uint8_t ClearStencilValue = 0;
    };

    enum class RenderGraphPassResourceUsages
    {
        None = 0,
        RenderTarget = 1 << 0,
        Variable = 1 << 1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(RenderGraphPassResourceUsages);

    struct RenderGraphPass final
    {
        // -----------------------
        // Config Data
        // -----------------------

        std::string Name{};

        bool HasSideEffects = false; // 如果写入了 external resource，就有副作用
        bool AllowPassCulling = true;
        bool EnableAsyncCompute = false; // 只是一个建议，会根据实际情况决定是否启用 async-compute
        bool UseDefaultVariables = true;

        std::unordered_map<size_t, RenderGraphPassResourceUsages> ResourcesIn{};  // 所有输入的资源，包括 render target
        std::unordered_map<size_t, RenderGraphPassResourceUsages> ResourcesOut{}; // 所有输出的资源，包括 render target

        uint32_t NumColorTargets = 0;
        RenderGraphPassColorTarget ColorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        RenderGraphPassDepthStencilTarget DepthStencilTarget{};

        bool HasCustomViewport = false;
        D3D12_VIEWPORT CustomViewport{};

        bool HasCustomScissorRect = false;
        D3D12_RECT CustomScissorRect{};

        bool HasCustomDepthBias = false;
        int32_t DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        float DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        float SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

        bool Wireframe = false;

        std::function<void(RenderGraphContext&)> RenderFunc{};

        // -----------------------
        // Runtime Data
        // -----------------------

        bool IsVisited = false;
        bool IsCulled = false;

        std::unordered_set<size_t> NextPassIndices{}; // 后继结点
        std::vector<size_t> ResourcesBorn{};          // 生命周期从本结点开始的资源
        std::vector<size_t> ResourcesDead{};          // 生命周期到本结点结束的资源

        bool IsAsyncCompute = false;
        bool IsBatchedWithPrevious = false; // 两个连续的 async-compute pass 可以合并，后一个 pass 的 Resource Barrier 也会提前
        bool NeedSyncPoint = false;         // 如果当前 pass 是 async-compute，并且该值为 true，则会产生一个 sync point
        GfxSyncPoint SyncPoint{};
        std::optional<size_t> PassIndexToWait = std::nullopt; // 在执行当前 pass 前需要等待的 pass，利用 sync point 实现
    };

    class RenderGraphBuilder final
    {
        RenderGraph* m_Graph;
        size_t m_PassIndex;

        RenderGraphPass& GetPass() const;

        void InResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex, RenderGraphPassResourceUsages usages);
        void OutResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex, RenderGraphPassResourceUsages usages);

    public:
        RenderGraphBuilder(RenderGraph* graph, size_t passIndex) : m_Graph(graph), m_PassIndex(passIndex) {}

        void AllowPassCulling(bool value);
        void EnableAsyncCompute(bool value);
        void UseDefaultVariables(bool value);

        // 表示需要读取前面 pass 写入 buffer 的数据
        void In(const BufferHandle& buffer);

        // 表示需要写入 buffer，当前和之后的 pass 可以读取
        void Out(const BufferHandle& buffer);

        // Shortcut for In and Out
        void InOut(const BufferHandle& buffer);

        // 表示需要读取前面 pass 写入 texture 的数据
        void In(const TextureHandle& texture);

        // 表示需要写入 texture，当前和之后的 pass 可以读取
        void Out(const TextureHandle& texture);

        // Shortcut for In and Out
        void InOut(const TextureHandle& texture);

        void SetColorTarget(const TextureSliceHandle& texture, RenderTargetInitMode initMode, const float color[4] = DirectX::Colors::Black);
        void SetColorTarget(const TextureSliceHandle& texture, uint32_t index = 0, RenderTargetInitMode initMode = RenderTargetInitMode::Load, const float color[4] = DirectX::Colors::Black);
        void SetDepthStencilTarget(const TextureSliceHandle& texture, RenderTargetInitMode initMode = RenderTargetInitMode::Load, float depth = GfxUtils::FarClipPlaneDepth, uint8_t stencil = 0);

        void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

        void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

        void SetDepthBias(int32_t bias, float slopeScaledBias, float clamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP);

        void SetWireframe(bool value);

        void SetRenderFunc(const std::function<void(RenderGraphContext&)>& func);

        template <size_t _Capacity>
        void In(const InlineArray<BufferHandle, _Capacity>& buffers)
        {
            for (size_t i = 0; i < buffers.Num(); i++)
            {
                In(buffers[i]);
            }
        }

        template <size_t _Capacity>
        void Out(const InlineArray<BufferHandle, _Capacity>& buffers)
        {
            for (size_t i = 0; i < buffers.Num(); i++)
            {
                Out(buffers[i]);
            }
        }

        template <size_t _Capacity>
        void InOut(const InlineArray<BufferHandle, _Capacity>& buffers)
        {
            for (size_t i = 0; i < buffers.Num(); i++)
            {
                InOut(buffers[i]);
            }
        }

        template <size_t _Capacity>
        void In(const InlineArray<TextureHandle, _Capacity>& textures)
        {
            for (size_t i = 0; i < textures.Num(); i++)
            {
                In(textures[i]);
            }
        }

        template <size_t _Capacity>
        void Out(const InlineArray<TextureHandle, _Capacity>& textures)
        {
            for (size_t i = 0; i < textures.Num(); i++)
            {
                Out(textures[i]);
            }
        }

        template <size_t _Capacity>
        void InOut(const InlineArray<TextureHandle, _Capacity>& textures)
        {
            for (size_t i = 0; i < textures.Num(); i++)
            {
                InOut(textures[i]);
            }
        }
    };

    class IRenderGraphCompiledEventListener
    {
    public:
        virtual void OnGraphCompiled(const std::vector<RenderGraphPass>& passes, const RenderGraphResourceManager* resourceManager) = 0;
    };

    class RenderGraph final
    {
        friend RenderGraphBuilder;

        std::vector<RenderGraphPass> m_Passes{};
        std::optional<size_t> m_PassIndexToWaitFallback = std::nullopt; // 用于等待不被任何 pass 依赖的 async compute 结束
        std::unique_ptr<RenderGraphResourceManager> m_ResourceManager = std::make_unique<RenderGraphResourceManager>();

        void CompilePasses();
        void CullPass(size_t passIndex, size_t& asyncComputeDeadlineIndexExclusive);
        void CompileAsyncCompute(size_t passIndex, size_t& deadlineIndexExclusive);
        size_t AvoidAsyncComputeResourceHazard(size_t passIndex, size_t& deadlineIndexExclusive);
        void BatchAsyncComputePasses();

        void RequestPassResources(const RenderGraphPass& pass);
        void EnsureAsyncComputePassResourceStates(RenderGraphContext& context, size_t passIndex);
        GfxCommandContext* EnsurePassContext(RenderGraphContext& context, size_t passIndex);
        GfxRenderTargetDesc ResolveRenderTarget(const RenderGraphPassRenderTarget& target);
        void SetPassRenderStates(GfxCommandContext* cmd, const RenderGraphPass& pass);
        void ReleasePassResources(const RenderGraphPass& pass);
        void SetPassDefaultVariables(GfxCommandContext* cmd, const RenderGraphPass& pass);
        void ExecutePasses();

        class DeferredCleanup
        {
            RenderGraph* m_Graph;

        public:
            DeferredCleanup(RenderGraph* graph) : m_Graph(graph) {}

            ~DeferredCleanup()
            {
                m_Graph->m_Passes.clear();
                m_Graph->m_PassIndexToWaitFallback = std::nullopt;
                m_Graph->m_ResourceManager->ClearResources();
            }
        };

    public:
        RenderGraphBuilder AddPass();
        RenderGraphBuilder AddPass(const std::string& name);
        void CompileAndExecute();

        BufferHandle ImportBuffer(const std::string& name, GfxBuffer* buffer);
        BufferHandle ImportBuffer(int32 id, GfxBuffer* buffer);

        BufferHandle RequestBuffer(const std::string& name, const GfxBufferDesc& desc);
        BufferHandle RequestBuffer(int32 id, const GfxBufferDesc& desc);

        BufferHandle RequestBufferWithContent(const std::string& name, const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter = std::nullopt);
        BufferHandle RequestBufferWithContent(int32 id, const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter = std::nullopt);

        TextureHandle ImportTexture(const std::string& name, GfxTexture* texture);
        TextureHandle ImportTexture(int32 id, GfxTexture* texture);

        TextureHandle RequestTexture(const std::string& name, const GfxTextureDesc& desc);
        TextureHandle RequestTexture(int32 id, const GfxTextureDesc& desc);

        static void AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
        static void RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
    };

    class RenderGraphContext final
    {
        friend RenderGraph;

        GfxCommandContext* m_Cmd = nullptr;

        void New(GfxCommandType type, bool waitPreviousOneOnGpu);
        void Ensure(GfxCommandType type);

        GfxSyncPoint UncheckedSubmit();
        void Submit();

    public:
        void SetVariable(const TextureHandle& texture, GfxTextureElement element = GfxTextureElement::Default, std::optional<uint32_t> mipSlice = std::nullopt);
        void SetVariable(const TextureHandle& texture, const std::string& aliasName, GfxTextureElement element = GfxTextureElement::Default, std::optional<uint32_t> mipSlice = std::nullopt);
        void SetVariable(const TextureHandle& texture, int32 aliasId, GfxTextureElement element = GfxTextureElement::Default, std::optional<uint32_t> mipSlice = std::nullopt);

        void SetVariable(const BufferHandle& buffer, GfxBufferElement element = GfxBufferElement::StructuredData);
        void SetVariable(const BufferHandle& buffer, const std::string& aliasName, GfxBufferElement element = GfxBufferElement::StructuredData);
        void SetVariable(const BufferHandle& buffer, int32 aliasId, GfxBufferElement element = GfxBufferElement::StructuredData);

        void UnsetVariables();

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex)
        {
            m_Cmd->DrawMesh(geometry, material, shaderPassIndex);
        }

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix)
        {
            m_Cmd->DrawMesh(geometry, material, shaderPassIndex, matrix);
        }

        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex)
        {
            m_Cmd->DrawMesh(mesh, subMeshIndex, material, shaderPassIndex);
        }

        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix)
        {
            m_Cmd->DrawMesh(mesh, subMeshIndex, material, shaderPassIndex, matrix);
        }

        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex)
        {
            m_Cmd->DrawMesh(subMesh, material, shaderPassIndex);
        }

        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix)
        {
            m_Cmd->DrawMesh(subMesh, material, shaderPassIndex, matrix);
        }

        void DrawMeshRenderers(size_t numRenderers, MeshRenderer* const* renderers, const std::string& lightMode)
        {
            m_Cmd->DrawMeshRenderers(numRenderers, renderers, lightMode);
        }

        void DispatchCompute(ComputeShader* shader, const std::string& kernelName, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
        {
            m_Cmd->DispatchCompute(shader, kernelName, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        void DispatchCompute(ComputeShader* shader, size_t kernelIndex, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
        {
            m_Cmd->DispatchCompute(shader, kernelIndex, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        void DispatchComputeByThreadCount(ComputeShader* shader, const std::string& kernelName, uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ)
        {
            m_Cmd->DispatchComputeByThreadCount(shader, kernelName, threadCountX, threadCountY, threadCountZ);
        }

        void DispatchComputeByThreadCount(ComputeShader* shader, size_t kernelIndex, uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ)
        {
            m_Cmd->DispatchComputeByThreadCount(shader, kernelIndex, threadCountX, threadCountY, threadCountZ);
        }

        void ResolveTexture(GfxTexture* source, GfxTexture* destination)
        {
            m_Cmd->ResolveTexture(source, destination);
        }

        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement)
        {
            m_Cmd->CopyBuffer(sourceBuffer, sourceElement, destinationBuffer, destinationElement);
        }

        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, uint32_t sourceOffsetInBytes, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement, uint32_t destinationOffsetInBytes, uint32_t sizeInBytes)
        {
            m_Cmd->CopyBuffer(sourceBuffer, sourceElement, sourceOffsetInBytes, destinationBuffer, destinationElement, destinationOffsetInBytes, sizeInBytes);
        }

        void UpdateSubresources(RefCountPtr<GfxResource> destination, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData)
        {
            m_Cmd->UpdateSubresources(destination, firstSubresource, numSubresources, srcData);
        }

        void CopyTexture(GfxTexture* sourceTexture, GfxTextureElement sourceElement, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, GfxTextureElement destinationElement, uint32_t destinationArraySlice, uint32_t destinationMipSlice)
        {
            m_Cmd->CopyTexture(sourceTexture, sourceElement, sourceArraySlice, sourceMipSlice, destinationTexture, destinationElement, destinationArraySlice, destinationMipSlice);
        }

        void CopyTexture(GfxTexture* sourceTexture, GfxTextureElement sourceElement, GfxCubemapFace sourceFace, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, GfxTextureElement destinationElement, GfxCubemapFace destinationFace, uint32_t destinationArraySlice, uint32_t destinationMipSlice)
        {
            m_Cmd->CopyTexture(sourceTexture, sourceElement, sourceFace, sourceArraySlice, sourceMipSlice, destinationTexture, destinationElement, destinationFace, destinationArraySlice, destinationMipSlice);
        }

        void CopyTexture(GfxTexture* sourceTexture, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, uint32_t destinationArraySlice, uint32_t destinationMipSlice)
        {
            m_Cmd->CopyTexture(sourceTexture, sourceArraySlice, sourceMipSlice, destinationTexture, destinationArraySlice, destinationMipSlice);
        }

        void CopyTexture(GfxTexture* sourceTexture, GfxCubemapFace sourceFace, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, GfxCubemapFace destinationFace, uint32_t destinationArraySlice, uint32_t destinationMipSlice)
        {
            m_Cmd->CopyTexture(sourceTexture, sourceFace, sourceArraySlice, sourceMipSlice, destinationTexture, destinationFace, destinationArraySlice, destinationMipSlice);
        }

        GfxDevice* GetDevice() const { return m_Cmd->GetDevice(); }
        GfxCommandContext* GetCommandContext() const { return m_Cmd; }
    };
}
