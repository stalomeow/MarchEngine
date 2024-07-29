#include "Rendering/RenderPipeline.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"
#include "Core/GameObject.h"
#include "Core/Debug.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace dx12demo
{
    RenderPipeline::RenderPipeline(int width, int height)
    {
        CheckMSAAQuailty();
        CreateDescriptorHeaps();
        CreateRootSignature();
        CreateShaderAndPSO();
        Resize(width, height);
    }

    void RenderPipeline::CheckMSAAQuailty()
    {
        auto device = GetGfxManager().GetDevice();

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
        msQualityLevels.Format = GetGfxManager().GetBackBufferFormat();
        msQualityLevels.SampleCount = m_MSAASampleCount;
        msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        THROW_IF_FAILED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msQualityLevels, sizeof(msQualityLevels)));

        m_MSAAQuality = msQualityLevels.NumQualityLevels - 1;
    }

    void RenderPipeline::CreateDescriptorHeaps()
    {
        m_RtvHeap = std::make_unique<DescriptorHeap>(L"Rtv Heap", D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 0, 1, false);
        m_DsvHeap = std::make_unique<DescriptorHeap>(L"Dsv Heap", D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0, 1, false);
    }

    void RenderPipeline::CreateRootSignature()
    {
        // Root parameter can be a table, root descriptor or root constants.
        CD3DX12_ROOT_PARAMETER slotRootParameter[2];

        // Create root CBVs.
        slotRootParameter[0].InitAsConstantBufferView(0);
        slotRootParameter[1].InitAsConstantBufferView(1);

        // A root signature is an array of root parameters.
        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

        if (errorBlob != nullptr)
        {
            DEBUG_LOG_ERROR(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        }
        THROW_IF_FAILED(hr);

        THROW_IF_FAILED(GetGfxManager().GetDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&m_RootSignature)));
    }

    static ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entrypoint,
        const std::string& target)
    {
        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = S_OK;

        ComPtr<ID3DBlob> byteCode = nullptr;
        ComPtr<ID3DBlob> errors;
        hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

        if (errors != nullptr)
            DEBUG_LOG_ERROR(reinterpret_cast<char*>(errors->GetBufferPointer()));

        THROW_IF_FAILED(hr);

        return byteCode;
    }

    void RenderPipeline::CreateShaderAndPSO()
    {
        m_VSByteCode = CompileShader(L"C:\\\Projects\\\Graphics\\\dx12-demo\\shaders\\test.hlsl", nullptr, "vert", "vs_5_0");
        m_PSByteCode = CompileShader(L"C:\\\Projects\\\Graphics\\\dx12-demo\\shaders\\test.hlsl", nullptr, "frag", "ps_5_0");

        auto device = GetGfxManager().GetDevice();

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { 0 };
        psoDesc.InputLayout = SimpleMesh::VertexInputLayout();
        psoDesc.pRootSignature = m_RootSignature.Get();
        psoDesc.VS =
        {
            reinterpret_cast<BYTE*>(m_VSByteCode->GetBufferPointer()),
            m_VSByteCode->GetBufferSize()
        };
        psoDesc.PS =
        {
            reinterpret_cast<BYTE*>(m_PSByteCode->GetBufferPointer()),
            m_PSByteCode->GetBufferSize()
        };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = GetGfxManager().GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.DSVFormat = m_DepthStencilFormat;
        THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSONormal)));

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSOWireframe)));

        psoDesc.SampleDesc.Count = m_MSAASampleCount;
        psoDesc.SampleDesc.Quality = m_MSAAQuality;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSONormalMSAA)));

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSOWireframeMSAA)));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetColorRenderTargetView()
    {
        return m_RtvHeap->GetCpuHandleForFixedDescriptor(0);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetDepthStencilTargetView()
    {
        return m_DsvHeap->GetCpuHandleForFixedDescriptor(0);
    }

    void RenderPipeline::SetEnableMSAA(bool value)
    {
        m_EnableMSAA = value;

        GetGfxManager().WaitForGpuIdle();
        CreateColorAndDepthStencilTarget(m_RenderTargetWidth, m_RenderTargetHeight);
    }

    void RenderPipeline::CreateColorAndDepthStencilTarget(int width, int height)
    {
        auto device = GetGfxManager().GetDevice();

        D3D12_CLEAR_VALUE colorClearValue = {};
        colorClearValue.Format = GetGfxManager().GetBackBufferFormat();
        memcpy(colorClearValue.Color, DirectX::Colors::Black, sizeof(colorClearValue.Color));
        THROW_IF_FAILED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(GetGfxManager().GetBackBufferFormat(), width, height, 1, 1,
                m_EnableMSAA ? m_MSAASampleCount : 1,
                m_EnableMSAA ? m_MSAAQuality : 0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
            D3D12_RESOURCE_STATE_COMMON,
            &colorClearValue, IID_PPV_ARGS(&m_ColorTarget)));
        m_LastColorTargetState = D3D12_RESOURCE_STATE_COMMON;
        device->CreateRenderTargetView(m_ColorTarget.Get(), nullptr,
            m_RtvHeap->GetCpuHandleForFixedDescriptor(0));

        D3D12_CLEAR_VALUE dsClearValue = {};
        dsClearValue.Format = m_DepthStencilFormat;
        dsClearValue.DepthStencil.Depth = 1.0f;
        dsClearValue.DepthStencil.Stencil = 0;
        THROW_IF_FAILED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(m_DepthStencilFormat, width, height, 1, 1,
                m_EnableMSAA ? m_MSAASampleCount : 1,
                m_EnableMSAA ? m_MSAAQuality : 0,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &dsClearValue, IID_PPV_ARGS(&m_DepthStencilTarget)));
        device->CreateDepthStencilView(m_DepthStencilTarget.Get(), nullptr,
            m_DsvHeap->GetCpuHandleForFixedDescriptor(0));
    }

    void RenderPipeline::Resize(int width, int height)
    {
        GetGfxManager().WaitForGpuIdle();

        width = max(width, 10);
        height = max(height, 10);
        CreateColorAndDepthStencilTarget(width, height);

        THROW_IF_FAILED(GetGfxManager().GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(GetGfxManager().GetBackBufferFormat(), width, height, 1, 1, 1, 0),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&m_ResolvedColorTarget)));
        m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_COMMON;

        m_RenderTargetWidth = width;
        m_RenderTargetHeight = height;

        m_Viewport.TopLeftX = 0;
        m_Viewport.TopLeftY = 0;
        m_Viewport.Width = static_cast<float>(width);
        m_Viewport.Height = static_cast<float>(height);
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;

        m_ScissorRect = { 0, 0, width, height };
    }

    void RenderPipeline::Render(CommandBuffer* cmd, const std::vector<std::unique_ptr<GameObject>>& gameObjects)
    {
        if (gameObjects.empty())
        {
            return;
        }

        PerDrawConstants drawConsts = {};

        // Convert Spherical to Cartesian coordinates.
        float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
        float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
        float y = m_Radius * cosf(m_Phi);

        // Build the view matrix.
        DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
        DirectX::XMVECTOR target = DirectX::XMVectorZero();
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
        DirectX::XMStoreFloat4x4(&drawConsts.ViewMatrix, view);
        DirectX::XMStoreFloat4x4(&drawConsts.InvViewMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(view), view));

        // The window resized, so update the aspect ratio and recompute the projection matrix.
        float asp = static_cast<float>(m_RenderTargetWidth) / static_cast<float>(m_RenderTargetHeight);
        DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, asp, 1.0f, 1000.0f);
        DirectX::XMStoreFloat4x4(&drawConsts.ProjectionMatrix, proj);
        DirectX::XMStoreFloat4x4(&drawConsts.InvProjectionMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(proj), proj));

        DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
        DirectX::XMStoreFloat4x4(&drawConsts.ViewProjectionMatrix, viewProj);
        DirectX::XMStoreFloat4x4(&drawConsts.InvViewProjectionMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj));

        auto cbPerDraw = cmd->AllocateTempUploadHeap<PerDrawConstants>(1, ConstantBufferAlignment);
        cbPerDraw.SetData(0, drawConsts);

        auto cbPerObj = cmd->AllocateTempUploadHeap<PerObjConstants>(gameObjects.size(), ConstantBufferAlignment);

        for (int i = 0; i < gameObjects.size(); i++)
        {
            PerObjConstants consts = {};
            consts.WorldMatrix = gameObjects[i]->GetTransform()->GetWorldMatrix();
            cbPerObj.SetData(i, consts);
        }

        // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
        // Reusing the command list reuses memory.
        if (m_EnableMSAA)
        {
            ID3D12PipelineState* pso = m_IsWireframe ? m_PSOWireframeMSAA.Get() : m_PSONormalMSAA.Get();
            cmd->GetList()->SetPipelineState(pso);
        }
        else
        {
            ID3D12PipelineState* pso = m_IsWireframe ? m_PSOWireframe.Get() : m_PSONormal.Get();
            cmd->GetList()->SetPipelineState(pso);
        }

        // Indicate a state transition on the resource usage.
        if (m_LastColorTargetState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                m_LastColorTargetState, D3D12_RESOURCE_STATE_RENDER_TARGET));
        }

        // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
        cmd->GetList()->RSSetViewports(1, &m_Viewport);
        cmd->GetList()->RSSetScissorRects(1, &m_ScissorRect);

        // Clear the back buffer and depth buffer.
        cmd->GetList()->ClearRenderTargetView(GetColorRenderTargetView(), DirectX::Colors::Black, 0, nullptr);
        cmd->GetList()->ClearDepthStencilView(GetDepthStencilTargetView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // Specify the buffers we are going to render to.
        cmd->GetList()->OMSetRenderTargets(1, &GetColorRenderTargetView(), true, &GetDepthStencilTargetView());

        cmd->GetList()->SetGraphicsRootSignature(m_RootSignature.Get());
        cmd->GetList()->SetGraphicsRootConstantBufferView(1, cbPerDraw.GetGpuVirtualAddress());

        for (int i = 0; i < gameObjects.size(); i++)
        {
            if (!gameObjects[i]->IsActive)
            {
                continue;
            }

            cmd->GetList()->SetGraphicsRootConstantBufferView(0, cbPerObj.GetGpuVirtualAddress(i));
            gameObjects[i]->GetMesh()->Draw(cmd);
        }

        if (m_EnableMSAA)
        {
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

            if (m_LastResolvedColorTargetState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
            {
                cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                    m_LastResolvedColorTargetState, D3D12_RESOURCE_STATE_RESOLVE_DEST));
            }

            cmd->GetList()->ResolveSubresource(m_ResolvedColorTarget.Get(), 0, m_ColorTarget.Get(), 0, GetGfxManager().GetBackBufferFormat());
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

            m_LastColorTargetState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
            m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else
        {
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

            if (m_LastResolvedColorTargetState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                    m_LastResolvedColorTargetState, D3D12_RESOURCE_STATE_COPY_DEST));
            }

            cmd->GetList()->CopyResource(m_ResolvedColorTarget.Get(), m_ColorTarget.Get());
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

            m_LastColorTargetState = D3D12_RESOURCE_STATE_COPY_SOURCE;
            m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
    }
}
