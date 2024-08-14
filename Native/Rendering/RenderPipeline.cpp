#include "Rendering/RenderPipeline.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"
#include "Core/Debug.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>

using Microsoft::WRL::ComPtr;

namespace dx12demo
{
    namespace
    {
        std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers()
        {
            // Applications usually only need a handful of samplers.  So just define them all up front
            // and keep them available as part of the root signature.

            const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
                0, // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

            const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
                1, // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

            const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
                2, // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

            const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
                3, // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

            const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
                4, // shaderRegister
                D3D12_FILTER_ANISOTROPIC, // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
                0.0f,                             // mipLODBias
                8);                               // maxAnisotropy

            const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
                5, // shaderRegister
                D3D12_FILTER_ANISOTROPIC, // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
                0.0f,                              // mipLODBias
                8);                                // maxAnisotropy

            return {
                pointWrap, pointClamp,
                linearWrap, linearClamp,
                anisotropicWrap, anisotropicClamp };
        }
    }

    RenderPipeline::RenderPipeline(int width, int height)
    {
        CheckMSAAQuailty();
        CreateDescriptorHeaps();
        CreateRootSignature();
        CreateShaderAndPSO();
        Resize(width, height);

        std::ifstream input(L"C:\\Users\\10247\\Desktop\\TestProj\\Assets\\Textures\\WoodCrate01.dds", std::ios::binary | std::ios::ate);
        auto size = input.tellg();
        std::vector<char> bytes(size);
        input.seekg(0);
        input.read(bytes.data(), size);
        m_Tex = std::make_unique<Texture>();
        m_Tex->SetDDSData(L"WoodCrate01.dds", bytes.data(), size);
        m_Tex->SetFilterAndWrapMode(FilterMode::Bilinear, WrapMode::Clamp);
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
        m_SrvHeap = std::make_unique<DescriptorHeap>(L"Srv Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SamplerHeap = std::make_unique<DescriptorHeap>(L"Sampler Heap", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20);
    }

    void RenderPipeline::CreateRootSignature()
    {
        CD3DX12_DESCRIPTOR_RANGE texTable;
        texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_DESCRIPTOR_RANGE samplerTable;
        samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 6);

        // Root parameter can be a table, root descriptor or root constants.
        CD3DX12_ROOT_PARAMETER slotRootParameter[5] = {};

        // Perfomance TIP: Order from most frequent to least frequent.
        slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[1].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);
        slotRootParameter[2].InitAsConstantBufferView(0);
        slotRootParameter[3].InitAsConstantBufferView(1);
        slotRootParameter[4].InitAsConstantBufferView(2);

        auto staticSamplers = GetStaticSamplers();

        // A root signature is an array of root parameters.
        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
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
        m_VSByteCode = CompileShader(L"C:\\Projects\\Graphics\\dx12-demo\\Shaders\\Default.hlsl", nullptr, "vert", "vs_5_0");
        m_PSByteCode = CompileShader(L"C:\\Projects\\Graphics\\dx12-demo\\Shaders\\Default.hlsl", nullptr, "frag", "ps_5_0");

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

    void RenderPipeline::Render(CommandBuffer* cmd)
    {
        if (m_RenderObjects.empty())
        {
            return;
        }

        PerPassConstants passConsts = {};

        // Convert Spherical to Cartesian coordinates.
        float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
        float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
        float y = m_Radius * cosf(m_Phi);

        // Build the view matrix.
        DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
        DirectX::XMVECTOR target = DirectX::XMVectorZero();
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
        DirectX::XMStoreFloat4x4(&passConsts.ViewMatrix, view);
        DirectX::XMStoreFloat4x4(&passConsts.InvViewMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(view), view));

        // The window resized, so update the aspect ratio and recompute the projection matrix.
        float asp = static_cast<float>(m_RenderTargetWidth) / static_cast<float>(m_RenderTargetHeight);
        DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, asp, 1.0f, 1000.0f);
        DirectX::XMStoreFloat4x4(&passConsts.ProjectionMatrix, proj);
        DirectX::XMStoreFloat4x4(&passConsts.InvProjectionMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(proj), proj));

        DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
        DirectX::XMStoreFloat4x4(&passConsts.ViewProjectionMatrix, viewProj);
        DirectX::XMStoreFloat4x4(&passConsts.InvViewProjectionMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj));

        DirectX::XMStoreFloat4(&passConsts.CameraPositionWS, pos);

        passConsts.LightCount = m_Lights.size();
        for (int i = 0; i < m_Lights.size(); i++)
        {
            if (!m_Lights[i]->IsActive)
            {
                continue;
            }

            m_Lights[i]->FillLightData(passConsts.Lights[i]);
        }

        auto cbPass = cmd->AllocateTempUploadHeap<PerPassConstants>(1, ConstantBufferAlignment);
        cbPass.SetData(0, passConsts);

        auto cbPerObj = cmd->AllocateTempUploadHeap<PerObjConstants>(m_RenderObjects.size(), ConstantBufferAlignment);

        for (int i = 0; i < m_RenderObjects.size(); i++)
        {
            PerObjConstants consts = {};
            consts.WorldMatrix = m_RenderObjects[i]->GetWorldMatrix();
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

        // ??? 不是 PSO 里已经有了吗
        cmd->GetList()->SetGraphicsRootSignature(m_RootSignature.Get());
        cmd->GetList()->SetGraphicsRootConstantBufferView(4, cbPass.GetGpuVirtualAddress());

        ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvHeap->GetHeapPointer(), m_SamplerHeap->GetHeapPointer() };
        cmd->GetList()->SetDescriptorHeaps(2, descriptorHeaps);

        for (int i = 0; i < m_RenderObjects.size(); i++)
        {
            if (!m_RenderObjects[i]->IsActive || m_RenderObjects[i]->Mesh == nullptr)
            {
                continue;
            }

            m_SrvHeap->Clear();
            m_SrvHeap->Append(m_Tex->GetTextureCpuDescriptorHandle());

            m_SamplerHeap->Clear();
            m_SamplerHeap->Append(m_Tex->GetSamplerCpuDescriptorHandle());

            cmd->GetList()->SetGraphicsRootDescriptorTable(0, m_SrvHeap->GetGpuHandleForDynamicDescriptor(0));
            cmd->GetList()->SetGraphicsRootDescriptorTable(1, m_SamplerHeap->GetGpuHandleForDynamicDescriptor(0));
            cmd->GetList()->SetGraphicsRootConstantBufferView(2, cbPerObj.GetGpuVirtualAddress(i));
            cmd->GetList()->SetGraphicsRootConstantBufferView(3, m_RenderObjects[i]->GetMaterialBuffer()->GetGpuVirtualAddress());
            m_RenderObjects[i]->Mesh->Draw(cmd);
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
