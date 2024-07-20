#include "app.h"
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <D3Dcompiler.h>
#include <WindowsX.h>
#include <imgui_impl_win32.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    // Win32 message handler
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        dx12demo::BaseWinApp* pthis = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pthis = reinterpret_cast<dx12demo::BaseWinApp*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pthis));
        }
        else
        {
            auto ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
            pthis = reinterpret_cast<dx12demo::BaseWinApp*>(ptr);
        }

        if (pthis)
        {
            return pthis->HandleMessage(hWnd, msg, wParam, lParam);
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

dx12demo::BaseWinApp::BaseWinApp(HINSTANCE hInstance)
    : m_InstanceHandle(hInstance), m_WindowHandle(NULL)
{

}

dx12demo::BaseWinApp::~BaseWinApp()
{
    // 等待 GPU 完成所有命令，防止崩溃
    if (m_Device)
    {
        FlushCommandQueue();

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

void dx12demo::BaseWinApp::ShowError(const std::wstring& message)
{
    MessageBoxW(NULL, message.c_str(), L"Error", MB_OK);
}

bool dx12demo::BaseWinApp::Initialize(int nCmdShow)
{
    try
    {
        if (!InitWindow(nCmdShow))
        {
            return false;
        }

        if (!InitDirect3D())
        {
            return false;
        }

        THROW_IF_FAILED(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

        auto mesh = std::make_shared<SimpleMesh>();
        mesh->AddSubMeshCube();
        mesh->BeginUploadToGPU(m_Device.Get(), m_CommandList.Get());
        ExecuteCommandList();
        FlushCommandQueue();
        mesh->EndUploadToGPU();
        m_Meshes.push_back(std::move(mesh));

        m_PerObjConstsBuffer = std::make_unique<UploadBuffer<ObjConsts>>(m_Device.Get(), UploadBufferType::Constant, 1);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_PerObjConstsBuffer->Resource()->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = m_PerObjConstsBuffer->Count() * m_PerObjConstsBuffer->Stride();
        m_Device->CreateConstantBufferView(&cbvDesc, m_CbvHeap->GetCPUDescriptorHandleForHeapStart());

        CreateRootSignature();
        CreateShaderAndPSO(m_Meshes[0]);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(WindowHandle());
        ImGui_ImplDX12_Init(m_Device.Get(), m_SwapChainBufferCount,
            m_BackBufferFormat, m_ImGUISrvHeap.Get(),
            m_ImGUISrvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_ImGUISrvHeap->GetGPUDescriptorHandleForHeapStart());

        OnResize();
        return true;
    }
    catch (const DxException& e)
    {
        ShowError(e.ToString());
        return false;
    }
}

bool dx12demo::BaseWinApp::InitWindow(int nCmdShow)
{
    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_InstanceHandle;
    wc.lpszClassName = TEXT("DX12DemoWindow");

    if (!RegisterClass(&wc))
    {
        ShowError(L"Register Window Class Failed");
        return false;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = { 0, 0, m_ClientWidth, m_ClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);

    m_WindowHandle = CreateWindowW(
        wc.lpszClassName,
        m_WindowTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        R.right - R.left,
        R.bottom - R.top,
        NULL,
        NULL,
        m_InstanceHandle,
        this);

    if (!m_WindowHandle)
    {
        ShowError(L"Create Window Failed");
        return false;
    }

    ShowWindow(m_WindowHandle, nCmdShow);
    UpdateWindow(m_WindowHandle);
    return true;
}

bool dx12demo::BaseWinApp::InitDirect3D()
{
    // 开启调试层
#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();
#endif

    THROW_IF_FAILED(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

    // 默认设备
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_Device));

    // 回退到 WARP 设备
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        THROW_IF_FAILED(m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        THROW_IF_FAILED(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_Device)));
    }

    THROW_IF_FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
    m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_CbvSrvUavDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = m_BackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    THROW_IF_FAILED(m_Device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)));
    m_MSAAQuality = msQualityLevels.NumQualityLevels - 1;

    CreateCommandObjects();
    CreateSwapChain();
    CreateDescriptorHeaps();

#if defined(DEBUG) || defined(_DEBUG)
    LogAdapters();
#endif

    return true;
}

void dx12demo::BaseWinApp::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    THROW_IF_FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
    THROW_IF_FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
    THROW_IF_FAILED(m_Device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_CommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&m_CommandList)));
    m_CommandList->Close();
}

void dx12demo::BaseWinApp::CreateSwapChain()
{
    m_SwapChain.Reset();

    // The ability to create an MSAA DXGI swap chain is only supported for the older "bit-blt" style presentation modes,
    // specifically DXGI_SWAP_EFFECT_DISCARD or DXGI_SWAP_EFFECT_SEQUENTIAL.
    // The newer "flip" style presentation modes DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD
    // required for Universal Windows Platform (UWP) apps and Direct3D 12 doesn't support
    // creating MSAA swap chains--attempts to create a swap chain with SampleDesc.Count > 1 will fail.
    // Instead, you create your own MSAA render target and explicitly resolve
    // to the DXGI back-buffer for presentation as shown here.

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = m_ClientWidth;
    swapChainDesc.BufferDesc.Height = m_ClientHeight;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = m_BackBufferFormat;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_SwapChainBufferCount;
    swapChainDesc.OutputWindow = m_WindowHandle;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    THROW_IF_FAILED(m_Factory->CreateSwapChain(m_CommandQueue.Get(), &swapChainDesc, m_SwapChain.GetAddressOf()));
}

void dx12demo::BaseWinApp::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = m_SwapChainBufferCount + 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CbvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC imguiSrvHeapDesc = {};
    imguiSrvHeapDesc.NumDescriptors = 2;
    imguiSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    imguiSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&imguiSrvHeapDesc, IID_PPV_ARGS(&m_ImGUISrvHeap)));
}

void dx12demo::BaseWinApp::CreateRootSignature()
{
    // Shader programs typically require resources as input (constant buffers,
    // textures, samplers).  The root signature defines the resources the shader
    // programs expect.  If we think of the shader programs as a function, and
    // the input resources as function parameters, then the root signature can be
    // thought of as defining the function signature.

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // Create a single descriptor table of CBVs.
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
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

ComPtr<ID3DBlob> CompileShader(
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

void dx12demo::BaseWinApp::CreateShaderAndPSO(std::shared_ptr<dx12demo::Mesh> mesh)
{
    m_VSByteCode = CompileShader(L"C:\\\Projects\\\Graphics\\\dx12-demo\\shaders\\test.hlsl", nullptr, "vert", "vs_5_0");
    m_PSByteCode = CompileShader(L"C:\\\Projects\\\Graphics\\\dx12-demo\\shaders\\test.hlsl", nullptr, "frag", "ps_5_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { 0 };
    psoDesc.InputLayout = mesh->VertexInputLayout();
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
    psoDesc.RTVFormats[0] = m_BackBufferFormat;
    psoDesc.SampleDesc.Count = m_EnableMSAA ? 4 : 1;
    psoDesc.SampleDesc.Quality = m_EnableMSAA ? m_MSAAQuality : 0;
    psoDesc.DSVFormat = m_DepthStencilFormat;
    THROW_IF_FAILED(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
}

void dx12demo::BaseWinApp::OnUpdate()
{
    // Convert Spherical to Cartesian coordinates.
    float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
    float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
    float y = m_Radius * cosf(m_Phi);

    // Build the view matrix.
    DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
    DirectX::XMVECTOR target = DirectX::XMVectorZero();
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_View, view);

    DirectX::XMMATRIX world = XMLoadFloat4x4(&m_World);
    DirectX::XMMATRIX proj = XMLoadFloat4x4(&m_Proj);
    DirectX::XMMATRIX worldViewProj = world * view * proj;

    // Update the constant buffer with the latest worldViewProj matrix.
    ObjConsts objConstants;
    DirectX::XMStoreFloat4x4(&objConstants.MatrixMVP, worldViewProj);
    m_PerObjConstsBuffer->SetData(0, objConstants);

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::BeginMainMenuBar();
    if (ImGui::MenuItem("Console"))
    {
        m_ShowConsoleWindow = true;
    }
    ImGui::EndMainMenuBar();
    ImGui::DockSpaceOverViewport();


    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (m_ShowDemoWindow)
        ImGui::ShowDemoWindow(&m_ShowDemoWindow);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &m_ShowDemoWindow);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &m_ShowAnotherWindow);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&m_ImGUIClearColor); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (m_ShowAnotherWindow)
    {
        auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_ImGUISrvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_CbvSrvUavDescriptorSize);
        ImGui::Begin("Game View", &m_ShowAnotherWindow);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        auto contextMin = ImGui::GetWindowContentRegionMin();
        auto contextMax = ImGui::GetWindowContentRegionMax();
        auto contextSize = ImVec2(contextMax.x - contextMin.x, contextMax.y - contextMin.y);
        ImGui::Image((ImTextureID)srv.ptr, contextSize);
        ImGui::End();
    }

    if (m_ShowConsoleWindow)
        ImGui::ShowDebugLogWindow(&m_ShowConsoleWindow);

    // Rendering
    ImGui::Render();
}

void dx12demo::BaseWinApp::OnRender()
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    THROW_IF_FAILED(m_CommandAllocator->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    THROW_IF_FAILED(m_CommandList->Reset(m_CommandAllocator.Get(), m_PSO.Get()));

    // Indicate a state transition on the resource usage.
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OffScreenRenderTargetBuffer.Get(),
        m_LastOffScreenRenderTargetBufferState, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
    m_CommandList->RSSetViewports(1, &m_Viewport);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

    // Clear the back buffer and depth buffer.
    m_CommandList->ClearRenderTargetView(OffScreenRenderTargetBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
    m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    m_CommandList->OMSetRenderTargets(1, &OffScreenRenderTargetBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvHeap.Get() };
    m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    m_CommandList->SetGraphicsRootDescriptorTable(0, m_CbvHeap->GetGPUDescriptorHandleForHeapStart());

    for (std::shared_ptr<Mesh> mesh : m_Meshes)
    {
        mesh->Draw(m_CommandList.Get());
    }

    // Render Dear ImGui graphics
    // m_CommandList->OMSetRenderTargets(1, &OffScreenRenderTargetBufferView(), false, nullptr);
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OffScreenRenderTargetBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
    m_CommandList->CopyResource(m_GameViewRenderTexture.Get(), m_OffScreenRenderTargetBuffer.Get());
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_GameViewRenderTexture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    m_CommandList->OMSetRenderTargets(1, &BackBufferView(), false, nullptr);
    const float clear_color_with_alpha[4] = { m_ImGUIClearColor.x * m_ImGUIClearColor.w, m_ImGUIClearColor.y * m_ImGUIClearColor.w, m_ImGUIClearColor.z * m_ImGUIClearColor.w, m_ImGUIClearColor.w };
    ID3D12DescriptorHeap* imguiDescriptorHeaps[] = { m_ImGUISrvHeap.Get() };
    m_CommandList->SetDescriptorHeaps(_countof(imguiDescriptorHeaps), imguiDescriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    m_LastOffScreenRenderTargetBufferState = D3D12_RESOURCE_STATE_COPY_SOURCE;

    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_GameViewRenderTexture.Get(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

    //if (m_EnableMSAA)
    //{
    //    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OffScreenRenderTargetBuffer.Get(),
    //        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));
    //    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
    //        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));
    //    m_CommandList->ResolveSubresource(BackBuffer(), 0, m_OffScreenRenderTargetBuffer.Get(), 0, m_BackBufferFormat);
    //    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
    //        D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));
    //    m_LastOffScreenRenderTargetBufferState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    //}
    //else
    //{
    //    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OffScreenRenderTargetBuffer.Get(),
    //        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
    //    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
    //        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));
    //    m_CommandList->CopyResource(BackBuffer(), m_OffScreenRenderTargetBuffer.Get());
    //    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffer(),
    //        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
    //    m_LastOffScreenRenderTargetBufferState = D3D12_RESOURCE_STATE_COPY_SOURCE;
    //}

    ExecuteCommandList();

    // swap the back and front buffers
    THROW_IF_FAILED(m_SwapChain->Present(0, 0)); // No vsync
    SwapBackBuffer();

    // Wait until frame commands are complete.  This waiting is inefficient and is
    // done for simplicity.  Later we will show how to organize our rendering code
    // so we do not have to wait per frame.
    FlushCommandQueue();
}

void dx12demo::BaseWinApp::OnResize()
{
    // 修改窗口大小前，先 Flush
    FlushCommandQueue();
    THROW_IF_FAILED(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

    // 先释放之前的 buffer 引用，然后才能 resize
    for (int i = 0; i < m_SwapChainBufferCount; i++)
    {
        m_SwapChainBuffers[i].Reset();
    }

    m_SwapChain->ResizeBuffers(
        m_SwapChainBufferCount,
        m_ClientWidth,
        m_ClientHeight,
        m_BackBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    m_CurrentBackBufferIndex = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < m_SwapChainBufferCount; i++)
    {
        THROW_IF_FAILED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffers[i])));
        m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC msaaDesc = {};
    msaaDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    msaaDesc.Width = m_ClientWidth;
    msaaDesc.Height = m_ClientHeight;
    msaaDesc.DepthOrArraySize = 1;
    msaaDesc.MipLevels = 1;
    msaaDesc.Format = m_BackBufferFormat;
    msaaDesc.SampleDesc.Count = m_EnableMSAA ? 4 : 1;
    msaaDesc.SampleDesc.Quality = m_EnableMSAA ? m_MSAAQuality : 0;
    msaaDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    msaaDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    m_LastOffScreenRenderTargetBufferState = m_EnableMSAA ?
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE :
        D3D12_RESOURCE_STATE_COPY_SOURCE;

    D3D12_CLEAR_VALUE msaaClearValue = {};
    msaaClearValue.Format = m_BackBufferFormat;
    memcpy(msaaClearValue.Color, DirectX::Colors::LightSteelBlue, sizeof(msaaClearValue.Color));
    THROW_IF_FAILED(m_Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &msaaDesc,
        m_LastOffScreenRenderTargetBufferState,
        &msaaClearValue,
        IID_PPV_ARGS(m_OffScreenRenderTargetBuffer.ReleaseAndGetAddressOf())));
    m_Device->CreateRenderTargetView(m_OffScreenRenderTargetBuffer.Get(), nullptr, rtvHeapHandle); // create at the end of rtv heap

    m_DepthStencilBuffer.Reset();

    D3D12_RESOURCE_DESC dsDesc = {};
    dsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsDesc.Width = m_ClientWidth;
    dsDesc.Height = m_ClientHeight;
    dsDesc.DepthOrArraySize = 1;
    dsDesc.MipLevels = 1;
    dsDesc.Format = m_DepthStencilFormat;
    dsDesc.SampleDesc.Count = m_EnableMSAA ? 4 : 1;
    dsDesc.SampleDesc.Quality = m_EnableMSAA ? m_MSAAQuality : 0;
    dsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = m_DepthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    THROW_IF_FAILED(m_Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &dsDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &clearValue,
        IID_PPV_ARGS(&m_DepthStencilBuffer)));

    m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(),
        nullptr, DepthStencilView());

    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        m_DepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // Game View RT
    m_Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(m_BackBufferFormat, m_ClientWidth, m_ClientHeight, 1, 1),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_GameViewRenderTexture));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_BackBufferFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_Device->CreateShaderResourceView(m_GameViewRenderTexture.Get(), &srvDesc,
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_ImGUISrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_CbvSrvUavDescriptorSize));

    ExecuteCommandList();
    FlushCommandQueue();

    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = static_cast<float>(m_ClientWidth);
    m_Viewport.Height = static_cast<float>(m_ClientHeight);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    m_ScissorRect = { 0, 0, m_ClientWidth, m_ClientHeight };

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&m_Proj, P);
}

void dx12demo::BaseWinApp::FlushCommandQueue()
{
    m_FenceValue++;
    THROW_IF_FAILED(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue));

    if (m_Fence->GetCompletedValue() < m_FenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        THROW_IF_FAILED(m_Fence->SetEventOnCompletion(m_FenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void dx12demo::BaseWinApp::ExecuteCommandList()
{
    THROW_IF_FAILED(m_CommandList->Close());
    ID3D12CommandList* cmdsList = m_CommandList.Get();
    m_CommandQueue->ExecuteCommandLists(1, &cmdsList);
}

void dx12demo::BaseWinApp::SwapBackBuffer()
{
    m_CurrentBackBufferIndex++;
    m_CurrentBackBufferIndex %= m_SwapChainBufferCount;
}

int dx12demo::BaseWinApp::Run()
{
    MSG msg = { 0 };
    m_Timer.Restart();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        if (!m_Timer.Tick())
        {
            Sleep(100);
            continue;
        }

        try
        {
            CalculateFrameStats();
            OnUpdate();
            OnRender();
        }
        catch (const DxException& e)
        {
            ShowError(e.ToString());
            return 0;
        }
    }

    return (int)msg.wParam;
}

LRESULT dx12demo::BaseWinApp::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // WM_ACTIVATE is sent when the window is activated or deactivated.
    // We pause the game when the window is deactivated and unpause it
    // when it becomes active.
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            m_Timer.Stop();
        }
        else
        {
            m_Timer.Start();
        }
        return 0;

    // WM_SIZE is sent when the user resizes the window.  
    case WM_SIZE:
        // Save the new client area dimensions.
        m_ClientWidth = LOWORD(lParam);
        m_ClientHeight = HIWORD(lParam);

        if (m_Device)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                m_Timer.Stop();
                m_IsMinimized = true;
                m_IsMaximized = false;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                m_Timer.Start();
                m_IsMinimized = false;
                m_IsMaximized = true;
                OnResize();
            }
            else if (wParam == SIZE_RESTORED)
            {
                // Restoring from minimized state?
                if (m_IsMinimized)
                {
                    m_Timer.Start();
                    m_IsMinimized = false;
                    OnResize();
                }
                // Restoring from maximized state?
                else if (m_IsMaximized)
                {
                    m_Timer.Start();
                    m_IsMaximized = false;
                    OnResize();
                }
                else if (m_IsResizing)
                {
                    // If user is dragging the resize bars, we do not resize 
                    // the buffers here because as the user continuously 
                    // drags the resize bars, a stream of WM_SIZE messages are
                    // sent to the window, and it would be pointless (and slow)
                    // to resize for each WM_SIZE message received from dragging
                    // the resize bars.  So instead, we reset after the user is 
                    // done resizing the window and releases the resize bars, which 
                    // sends a WM_EXITSIZEMOVE message.
                }
                else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
                {
                    OnResize();
                }
            }
        }
        return 0;

    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        m_IsResizing = true;
        m_Timer.Stop();
        return 0;

    // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        m_IsResizing = false;
        m_Timer.Start();
        OnResize();
        return 0;

    // The WM_MENUCHAR message is sent when a menu is active and the user presses 
    // a key that does not correspond to any mnemonic or accelerator key. 
    case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

    // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
        return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if ((int)wParam == VK_F2)
        {
            SetMSAAState(!GetMSAAState());
        }
        return 0;

    // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void dx12demo::BaseWinApp::CalculateFrameStats()
{
    // Code computes the average frames per second, and also the 
    // average time it takes to render one frame.  These stats 
    // are appended to the window caption bar.

    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    // Compute averages over one second period.
    if ((m_Timer.ElapsedTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // fps = frameCnt / 1
        float mspf = 1000.0f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring mspfStr = std::to_wstring(mspf);

        std::wstring windowText = m_WindowTitle +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(WindowHandle(), windowText.c_str());

        // Reset for next average.
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

void dx12demo::BaseWinApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while (m_Factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);

        ++i;
    }

    for (size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        adapterList[i]->Release();
    }
}

void dx12demo::BaseWinApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, m_BackBufferFormat);
        output->Release();
        ++i;
    }
}

void dx12demo::BaseWinApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        OutputDebugString(text.c_str());
    }
}
