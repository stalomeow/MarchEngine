#include "Rendering/RenderPipeline.h"
#include "Rendering/DxException.h"
#include "Core/GameObject.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace dx12demo
{
    RenderPipeline::RenderPipeline(ComPtr<ID3D12Device> device, int width, int height, int meshCount)
        : m_Device(device), m_MeshCount(meshCount)
    {
        THROW_IF_FAILED(m_Device->CreateFence(m_CurrentFence, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
        m_FenceEventHandle = CreateEventExW(NULL, NULL, NULL, EVENT_ALL_ACCESS);

        CheckMSAAQuailty();
        CreateCommandObjects();
        CreateFrameResources();
        CreateDescriptorHeaps();
        CreateRootSignature();
        CreateShaderAndPSO();
        Resize(width, height);
    }

    RenderPipeline::~RenderPipeline()
    {
        // 等待 GPU 完成所有命令，防止崩溃
        WaitForGPUIdle();
        CloseHandle(m_FenceEventHandle);
    }

    void RenderPipeline::CheckMSAAQuailty()
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
        msQualityLevels.Format = m_ColorFormat;
        msQualityLevels.SampleCount = m_MSAASampleCount;
        msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        THROW_IF_FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msQualityLevels, sizeof(msQualityLevels)));

        m_MSAAQuality = msQualityLevels.NumQualityLevels - 1;
    }

    void RenderPipeline::CreateCommandObjects()
    {
        const D3D12_COMMAND_LIST_TYPE cmdListType = D3D12_COMMAND_LIST_TYPE_DIRECT;

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = cmdListType;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        THROW_IF_FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
        THROW_IF_FAILED(m_Device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&m_CommandAllocator)));
        THROW_IF_FAILED(m_Device->CreateCommandList(0, cmdListType, m_CommandAllocator.Get(),
            nullptr, IID_PPV_ARGS(&m_CommandList)));

        m_CommandList->Close();
    }

    void RenderPipeline::CreateFrameResources()
    {
        for (int i = 0; i < 3; i++)
        {
            m_FrameResources.push_back(std::make_unique<FrameResource>(m_Device.Get(), m_CurrentFence, m_MeshCount, 1));
        }
    }

    void RenderPipeline::CreateDescriptorHeaps()
    {
        m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_DsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_CbvSrvUavDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.NumDescriptors = (m_MeshCount + 1) * static_cast<int>(m_FrameResources.size());
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CbvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvHeap)));

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_CbvHeap->GetCPUDescriptorHandleForHeapStart());
        for (int i = 0; i < m_FrameResources.size(); i++)
        {
            FrameResource* res = m_FrameResources[i].get();

            D3D12_CONSTANT_BUFFER_VIEW_DESC perObjCbvDesc = {};
            perObjCbvDesc.SizeInBytes = res->PerObjectConstBuffer->GetStride();
            perObjCbvDesc.BufferLocation = res->PerObjectConstBuffer->GetResource()->GetGPUVirtualAddress();

            for (int j = 0; j < m_MeshCount; j++)
            {
                m_Device->CreateConstantBufferView(&perObjCbvDesc, cbvHandle);
                perObjCbvDesc.BufferLocation += perObjCbvDesc.SizeInBytes;
                cbvHandle.Offset(1, m_CbvSrvUavDescriptorSize);
            }

            D3D12_CONSTANT_BUFFER_VIEW_DESC perDrawCbvDesc = {};
            perDrawCbvDesc.SizeInBytes = res->PerDrawConstBuffer->GetStride();
            perDrawCbvDesc.BufferLocation = res->PerDrawConstBuffer->GetResource()->GetGPUVirtualAddress();
            m_Device->CreateConstantBufferView(&perDrawCbvDesc, cbvHandle);
            cbvHandle.Offset(1, m_CbvSrvUavDescriptorSize);
        }
    }

    void RenderPipeline::CreateRootSignature()
    {
        CD3DX12_DESCRIPTOR_RANGE cbvTable0;
        cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

        CD3DX12_DESCRIPTOR_RANGE cbvTable1;
        cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

        // Root parameter can be a table, root descriptor or root constants.
        CD3DX12_ROOT_PARAMETER slotRootParameter[2];

        // Create root CBVs.
        slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
        slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

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
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        THROW_IF_FAILED(hr);

        THROW_IF_FAILED(m_Device->CreateRootSignature(
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
            OutputDebugStringA((char*)errors->GetBufferPointer());

        THROW_IF_FAILED(hr);

        return byteCode;
    }

    void RenderPipeline::CreateShaderAndPSO()
    {
        m_VSByteCode = CompileShader(L"C:\\\Projects\\\Graphics\\\dx12-demo\\shaders\\test.hlsl", nullptr, "vert", "vs_5_0");
        m_PSByteCode = CompileShader(L"C:\\\Projects\\\Graphics\\\dx12-demo\\shaders\\test.hlsl", nullptr, "frag", "ps_5_0");

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
        psoDesc.RTVFormats[0] = m_ColorFormat;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.DSVFormat = m_DepthStencilFormat;
        THROW_IF_FAILED(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSONormal)));

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        THROW_IF_FAILED(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSOWireframe)));

        psoDesc.SampleDesc.Count = m_MSAASampleCount;
        psoDesc.SampleDesc.Quality = m_MSAAQuality;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        THROW_IF_FAILED(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSONormalMSAA)));

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        THROW_IF_FAILED(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSOWireframeMSAA)));
    }

    void RenderPipeline::WaitForFence(UINT64 fence)
    {
        if (m_Fence->GetCompletedValue() < fence)
        {
            THROW_IF_FAILED(m_Fence->SetEventOnCompletion(fence, m_FenceEventHandle));
            WaitForSingleObject(m_FenceEventHandle, INFINITE);
        }
    }

    void RenderPipeline::WaitForGPUIdle()
    {
        m_CurrentFence++;
        THROW_IF_FAILED(m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence));
        WaitForFence(m_CurrentFence);
    }

    void RenderPipeline::ExecuteCommandList()
    {
        THROW_IF_FAILED(m_CommandList->Close());
        ID3D12CommandList* cmdsList = m_CommandList.Get();
        m_CommandQueue->ExecuteCommandLists(1, &cmdsList);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetColorRenderTargetView()
    {
        return m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetDepthStencilTargetView()
    {
        return m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    void RenderPipeline::Resize(int width, int height)
    {
        WaitForGPUIdle();

        width = max(width, 10);
        height = max(height, 10);

        D3D12_CLEAR_VALUE colorClearValue = {};
        colorClearValue.Format = m_ColorFormat;
        memcpy(colorClearValue.Color, DirectX::Colors::Black, sizeof(colorClearValue.Color));
        THROW_IF_FAILED(m_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(m_ColorFormat, width, height, 1, 1,
                m_EnableMSAA ? m_MSAASampleCount : 1,
                m_EnableMSAA ? m_MSAAQuality : 0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
            D3D12_RESOURCE_STATE_COMMON,
            &colorClearValue, IID_PPV_ARGS(&m_ColorTarget)));
        m_LastColorTargetState = D3D12_RESOURCE_STATE_COMMON;
        m_Device->CreateRenderTargetView(m_ColorTarget.Get(), nullptr,
            m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

        THROW_IF_FAILED(m_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(m_ColorFormat, width, height, 1, 1, 1, 0),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&m_ResolvedColorTarget)));
        m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_COMMON;
        m_Device->CreateShaderResourceView(m_ResolvedColorTarget.Get(), nullptr,
            m_SrvHeap->GetCPUDescriptorHandleForHeapStart());

        D3D12_CLEAR_VALUE dsClearValue = {};
        dsClearValue.Format = m_DepthStencilFormat;
        dsClearValue.DepthStencil.Depth = 1.0f;
        dsClearValue.DepthStencil.Stencil = 0;
        THROW_IF_FAILED(m_Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(m_DepthStencilFormat, width, height, 1, 1,
                m_EnableMSAA ? m_MSAASampleCount : 1,
                m_EnableMSAA ? m_MSAAQuality : 0,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &dsClearValue, IID_PPV_ARGS(&m_DepthStencilTarget)));
        m_Device->CreateDepthStencilView(m_DepthStencilTarget.Get(), nullptr,
            m_DsvHeap->GetCPUDescriptorHandleForHeapStart());

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

    void RenderPipeline::Render(const std::vector<std::unique_ptr<GameObject>>& gameObjects, std::function<void(ID3D12GraphicsCommandList*)> action)
    {
        m_CurrentFrameResourceIndex = (m_CurrentFrameResourceIndex + 1) % m_FrameResources.size();
        FrameResource* frameRes = m_FrameResources[m_CurrentFrameResourceIndex].get();
        WaitForFence(frameRes->FenceValue);

        // Free Temp res
        //while (!m_TempResources.empty())
        //{
        //    auto& [_, fence] = m_TempResources.front();

        //    if (m_Fence->GetCompletedValue() < fence)
        //    {
        //        break;
        //    }

        //    m_TempResources.pop();
        //    OutputDebugStringA("Free Temp Res\n");
        //}

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

        frameRes->PerDrawConstBuffer->SetData(0, drawConsts);

        for (int i = 0; i < gameObjects.size(); i++)
        {
            PerObjConstants consts = {};
            consts.WorldMatrix = gameObjects[i]->GetTransform()->GetWorldMatrix();
            frameRes->PerObjectConstBuffer->SetData(i, consts);
        }

        // Reuse the memory associated with command recording.
        // We can only reset when the associated command lists have finished execution on the GPU.
        THROW_IF_FAILED(frameRes->CommandAllocator->Reset());

        // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
        // Reusing the command list reuses memory.
        if (m_EnableMSAA)
        {
            ID3D12PipelineState* pso = m_IsWireframe ? m_PSOWireframeMSAA.Get() : m_PSONormalMSAA.Get();
            THROW_IF_FAILED(m_CommandList->Reset(frameRes->CommandAllocator.Get(), pso));
        }
        else
        {
            ID3D12PipelineState* pso = m_IsWireframe ? m_PSOWireframe.Get() : m_PSONormal.Get();
            THROW_IF_FAILED(m_CommandList->Reset(frameRes->CommandAllocator.Get(), pso));
        }

        // Indicate a state transition on the resource usage.
        if (m_LastColorTargetState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                m_LastColorTargetState, D3D12_RESOURCE_STATE_RENDER_TARGET));
        }

        // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
        m_CommandList->RSSetViewports(1, &m_Viewport);
        m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

        // Clear the back buffer and depth buffer.
        m_CommandList->ClearRenderTargetView(GetColorRenderTargetView(), DirectX::Colors::Black, 0, nullptr);
        m_CommandList->ClearDepthStencilView(GetDepthStencilTargetView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // Specify the buffers we are going to render to.
        m_CommandList->OMSetRenderTargets(1, &GetColorRenderTargetView(), true, &GetDepthStencilTargetView());

        ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
        m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
        CD3DX12_GPU_DESCRIPTOR_HANDLE perDrawCbvHandle(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
        perDrawCbvHandle.Offset((m_MeshCount + 1) * m_CurrentFrameResourceIndex + m_MeshCount, m_CbvSrvUavDescriptorSize);
        m_CommandList->SetGraphicsRootDescriptorTable(1, perDrawCbvHandle);

        std::vector<ComPtr<ID3D12Resource>> tempRes{};

        for (int i = 0; i < gameObjects.size(); i++)
        {
            if (!gameObjects[i]->IsActive)
            {
                continue;
            }

            CD3DX12_GPU_DESCRIPTOR_HANDLE perObjCbvHandle(m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
            perObjCbvHandle.Offset((m_MeshCount + 1) * m_CurrentFrameResourceIndex + i, m_CbvSrvUavDescriptorSize);
            m_CommandList->SetGraphicsRootDescriptorTable(0, perObjCbvHandle);

            tempRes.clear();
            gameObjects[i]->GetMesh()->Draw(m_Device.Get(), m_CommandList.Get(), tempRes);

            if (tempRes.size() > 0)
            {
                /*ExecuteCommandList();
                THROW_IF_FAILED(m_CommandList->Reset(frameRes->CommandAllocator.Get(), pso));

                m_CurrentFence++;
                THROW_IF_FAILED(m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence));*/

                for (auto& res : tempRes)
                {
                    m_TempResources.push(std::make_tuple(res, m_CurrentFence));
                }
            }
        }

        if (m_EnableMSAA)
        {
            m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

            if (m_LastResolvedColorTargetState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
            {
                m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                    m_LastResolvedColorTargetState, D3D12_RESOURCE_STATE_RESOLVE_DEST));
            }

            m_CommandList->ResolveSubresource(m_ResolvedColorTarget.Get(), 0, m_ColorTarget.Get(), 0, m_ColorFormat);
            m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

            m_LastColorTargetState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
            m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else
        {
            m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

            if (m_LastResolvedColorTargetState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                    m_LastResolvedColorTargetState, D3D12_RESOURCE_STATE_COPY_DEST));
            }

            m_CommandList->CopyResource(m_ResolvedColorTarget.Get(), m_ColorTarget.Get());
            m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

            m_LastColorTargetState = D3D12_RESOURCE_STATE_COPY_SOURCE;
            m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }

        action(m_CommandList.Get());

        ExecuteCommandList();

        m_CurrentFence++;
        frameRes->FenceValue = m_CurrentFence;
        THROW_IF_FAILED(m_CommandQueue->Signal(m_Fence.Get(), frameRes->FenceValue));
    }
}
