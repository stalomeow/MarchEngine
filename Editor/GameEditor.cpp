#include "Editor/GameEditor.h"
#include "App/WinApplication.h"
#include "Rendering/DxException.h"
#include "Core/Debug.h"
#include "Core/StringUtility.h"
#include <DirectXMath.h>
#include <imgui_stdlib.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace dx12demo
{
    // Win32 message handler
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    bool GameEditor::OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult)
    {
        if (ImGui_ImplWin32_WndProcHandler(GetApp().GetHWND(), msg, wParam, lParam))
        {
            outResult = true;
            return true;
        }
        return false;
    }

    void GameEditor::OnAppStart()
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

        THROW_IF_FAILED(m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugInfoQueue)));
        DWORD cookie;
        THROW_IF_FAILED(m_DebugInfoQueue->RegisterMessageCallback([](
            D3D12_MESSAGE_CATEGORY Category,
            D3D12_MESSAGE_SEVERITY Severity,
            D3D12_MESSAGE_ID ID,
            LPCSTR pDescription,
            void* pContext)
            {
                switch (Severity)
                {
                case D3D12_MESSAGE_SEVERITY_INFO:
                case D3D12_MESSAGE_SEVERITY_MESSAGE:
                    DEBUG_LOG_INFO("%s", pDescription);
                    break;

                case D3D12_MESSAGE_SEVERITY_WARNING:
                    DEBUG_LOG_WARN("%s", pDescription);
                    break;

                case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                case D3D12_MESSAGE_SEVERITY_ERROR:
                    DEBUG_LOG_ERROR("%s", pDescription);
                    break;

                default:
                    DEBUG_LOG_WARN("Unknown D3D12_MESSAGE_SEVERITY: %d; %s", Severity, pDescription);
                    break;
                }
            }, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &cookie));

        m_GameObjects.push_back(std::make_unique<GameObject>());
        m_GameObjects.push_back(std::make_unique<GameObject>());
        m_GameObjects.push_back(std::make_unique<GameObject>());

        auto [width, height] = GetApp().GetClientWidthAndHeight();
        m_RenderPipeline = std::make_unique<RenderPipeline>(m_Device, width, height, m_GameObjects.size());

        CreateDescriptorHeaps();
        CreateSwapChain();
        InitImGui();
        LogAdapters();
    }

    void GameEditor::CreateSwapChain()
    {
        m_SwapChain.Reset();

        // The ability to create an MSAA DXGI swap chain is only supported for the older "bit-blt" style presentation modes,
        // specifically DXGI_SWAP_EFFECT_DISCARD or DXGI_SWAP_EFFECT_SEQUENTIAL.
        // The newer "flip" style presentation modes DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL or DXGI_SWAP_EFFECT_FLIP_DISCARD
        // required for Universal Windows Platform (UWP) apps and Direct3D 12 doesn't support
        // creating MSAA swap chains--attempts to create a swap chain with SampleDesc.Count > 1 will fail.
        // Instead, you create your own MSAA render target and explicitly resolve
        // to the DXGI back-buffer for presentation as shown here.

        auto [width, height] = GetApp().GetClientWidthAndHeight();

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferDesc.Width = width;
        swapChainDesc.BufferDesc.Height = height;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferDesc.Format = m_RenderPipeline->GetColorFormat();
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_SwapChainBufferCount;
        swapChainDesc.OutputWindow = GetApp().GetHWND();
        swapChainDesc.Windowed = true;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        THROW_IF_FAILED(m_Factory->CreateSwapChain(m_RenderPipeline->GetCommandQueue(), &swapChainDesc, m_SwapChain.GetAddressOf()));
    
        ResizeSwapChain();
    }

    void GameEditor::CreateDescriptorHeaps()
    {
        m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_CbvSrvUavDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_SwapChainBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC imguiSrvHeapDesc = {};
        imguiSrvHeapDesc.NumDescriptors = 2;
        imguiSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        imguiSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        THROW_IF_FAILED(m_Device->CreateDescriptorHeap(&imguiSrvHeapDesc, IID_PPV_ARGS(&m_SrvHeap)));

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_SrvHeap->GetCPUDescriptorHandleForHeapStart());
        srvHandle.Offset(1, m_CbvSrvUavDescriptorSize);
        m_Device->CreateShaderResourceView(m_RenderPipeline->GetResolvedColorTarget(), nullptr, srvHandle);
    }

    void GameEditor::InitImGui()
    {
        // Setup Dear ImGui context
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(GetApp().GetHWND());
        io.Fonts->AddFontFromFileTTF(m_FontPath, m_FontSize * GetApp().GetDisplayScale(),
            nullptr, io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
        ImGui::GetStyle().FrameBorderSize = 1.0f;
        ImGui::GetStyle().FrameRounding = 2.0f;

        ImGui_ImplDX12_Init(m_Device.Get(), m_RenderPipeline->GetFrameResourceCount(),
            m_RenderPipeline->GetColorFormat(), m_SrvHeap.Get(),
            m_SrvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_SrvHeap->GetGPUDescriptorHandleForHeapStart());
    }

    void GameEditor::SwapBackBuffer()
    {
        m_CurrentBackBufferIndex++;
        m_CurrentBackBufferIndex %= m_SwapChainBufferCount;
    }

    void GameEditor::OnAppQuit()
    {
        if (m_Device)
        {
            m_RenderPipeline->WaitForGPUIdle();
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
    }

    namespace
    {
        void DrawVec3(const std::string& label, float* values, float speed = 0.1f)
        {
            static float maxLabelWidth = 120.0f;

            ImGui::Text(label.c_str());
            ImGui::SameLine(maxLabelWidth);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat3(("##" + label).c_str(), values, speed);
            ImGui::PopItemWidth();
        }
    }

    void GameEditor::DrawImGui()
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static bool showStyleEditor = false;
        static bool showMetrics = false;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Windows"))
            {
                if (ImGui::BeginMenu("ImGui Tools"))
                {
                    if (ImGui::MenuItem("Style Editor"))
                    {
                        showStyleEditor = true;
                    }

                    if (ImGui::MenuItem("Metrics"))
                    {
                        showMetrics = true;
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Console"))
                {
                    m_ShowConsoleWindow = true;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::DockSpaceOverViewport();

        if (showStyleEditor)
        {
            ImGui::Begin("Style Editor", &showStyleEditor);
            ImGui::ShowStyleEditor();
            ImGui::End();
        }
        if (showMetrics) ImGui::ShowMetricsWindow(&showMetrics);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (m_ShowDemoWindow)
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            GameObject* go = m_GameObjects[m_SelectedGameObjectIndex].get();

            ImGui::Begin("Inspector");

            ImGui::Checkbox("##GameObjectActive", &go->IsActive);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##GameObjectName", &go->Name);
            ImGui::PopItemWidth();
            ImGui::SeparatorText("Components");

            static bool hasTrans = true;

            if (ImGui::CollapsingHeader("Transform", &hasTrans, ImGuiTreeNodeFlags_DefaultOpen))
            {
                Transform* trans = go->GetTransform();
                DrawVec3("Position", (float*)&trans->Position, 0.1f);
                DrawVec3("Rotation", (float*)&trans->RotationEuler, 0.1f);
                DrawVec3("Scale", (float*)&trans->Scale, 0.1f);

                auto euler = DirectX::XMLoadFloat3(&trans->RotationEuler);
                auto quaternion = DirectX::XMQuaternionRotationRollPitchYawFromVector(euler);
                DirectX::XMStoreFloat4(&trans->Rotation, quaternion);
            }

            ImGui::Spacing();

            auto windowWidth = ImGui::GetWindowSize().x;
            auto textWidth = ImGui::CalcTextSize("Add Component").x;
            float padding = 80.0f;
            ImGui::SetCursorPosX((windowWidth - textWidth - padding) * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding * 0.5f, ImGui::GetStyle().FramePadding.y));

            if (ImGui::Button("Add Component", ImVec2(0, 0)))
            {
                hasTrans = true;
            }

            ImGui::PopStyleVar();

            ImGui::End();
        }

        // 3. Show another simple window.
        if (m_ShowAnotherWindow)
        {
            ImGui::Begin("Scene", &m_ShowAnotherWindow, ImGuiWindowFlags_MenuBar);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Options"))
                {
                    bool msaa = m_RenderPipeline->GetEnableMSAA();
                    if (ImGui::MenuItem("MSAA", nullptr, &msaa))
                    {
                        m_RenderPipeline->SetEnableMSAA(msaa);
                    }

                    bool wireframe = m_RenderPipeline->GetIsWireframe();
                    if (ImGui::MenuItem("Wireframe", nullptr, &wireframe))
                    {
                        m_RenderPipeline->SetIsWireframe(wireframe);
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            auto contextSize = ImGui::GetContentRegionAvail();
            
            if (m_LastSceneViewWidth != contextSize.x || m_LastSceneViewHeight != contextSize.y)
            {
                m_LastSceneViewWidth = contextSize.x;
                m_LastSceneViewHeight = contextSize.y;
                ResizeRenderPipeline(m_LastSceneViewWidth, m_LastSceneViewHeight);
            }

            CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_SrvHeap->GetGPUDescriptorHandleForHeapStart());
            srvHandle.Offset(1, m_CbvSrvUavDescriptorSize);
            ImGui::Image((ImTextureID)srvHandle.ptr, contextSize);
            ImGui::End();
        }

        if (m_ShowHierarchyWindow)
        {
            ImGui::Begin("Hierarchy", &m_ShowHierarchyWindow);

            for (int i = 0; i < m_GameObjects.size(); i++)
            {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;

                if (i == m_SelectedGameObjectIndex)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::PushID("##GameObject");
                bool nodeOpen = ImGui::TreeNodeEx(m_GameObjects[i]->Name.c_str(), flags);
                ImGui::PopID();

                if (nodeOpen)
                {
                    if (ImGui::IsItemClicked())
                    {
                        m_SelectedGameObjectIndex = i;
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::End();
        }

        DrawConsoleWindow();

        // Rendering
        ImGui::Render();
    }

    void GameEditor::DrawConsoleWindow()
    {
        if (!m_ShowConsoleWindow)
        {
            return;
        }

        if (!ImGui::Begin("Console", &m_ShowConsoleWindow, ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear"))
        {
            Debug::ClearLogs();
        }

        static int logTypeFilter = 0;
        static ImGuiTextFilter logMsgFilter{};
        static int selectedLog = -1;

        ImGui::SameLine();

        if (ImGui::Button("Options"))
        {
            ImGui::OpenPopup("Options");
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::TextUnformatted("Filter (inc,-exc)");
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::Combo("##LogTypeFilter", &logTypeFilter, "All\0Info\0Warn\0Error\0\0");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        logMsgFilter.Draw("##LogMsgFilter", ImGui::GetContentRegionAvail().x);

        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto Scroll", &m_ConsoleWindowAutoScroll);
            ImGui::EndPopup();
        }

        ImGui::SeparatorText(StringUtility::Format("%d Info | %d Warn | %d Error",
            Debug::GetLogCount(Debug::LogType::Info),
            Debug::GetLogCount(Debug::LogType::Warn),
            Debug::GetLogCount(Debug::LogType::Error)).c_str());

        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_ResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_None))
        {
            for (int i = 0; i < Debug::s_Logs.size(); i++)
            {
                const auto& item = Debug::s_Logs[i];

                if ((logTypeFilter == 1 && item.Type != Debug::LogType::Info)  ||
                    (logTypeFilter == 2 && item.Type != Debug::LogType::Warn)  ||
                    (logTypeFilter == 3 && item.Type != Debug::LogType::Error) ||
                    (logMsgFilter.IsActive() && !logMsgFilter.PassFilter(item.Message.c_str())))
                {
                    if (selectedLog == i)
                    {
                        selectedLog = -1;
                    }
                    continue;
                }

                float width = ImGui::GetContentRegionMax().x;
                float height = ImGui::GetTextLineHeight();
                ImVec2 cursorPos = ImGui::GetCursorPos();
                std::string label = "##LogItem" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), i == selectedLog, ImGuiSelectableFlags_None, ImVec2(width, height)))
                {
                    selectedLog = i;
                }

                ImGui::SameLine();
                ImGui::SetCursorPos(cursorPos);

                ImVec4 timeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                timeColor.w = 0.6f;
                ImGui::PushStyleColor(ImGuiCol_Text, timeColor);
                ImGui::TextUnformatted(Debug::GetTimePrefix(item.Time).c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine();

                ImVec4 color;
                bool has_color = false;
                if (item.Type == Debug::LogType::Info) { color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); has_color = true; }
                else if (item.Type == Debug::LogType::Error) { color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); has_color = true; }
                else if (item.Type == Debug::LogType::Warn) { color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); has_color = true; }

                if (has_color) ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(Debug::GetTypePrefix(item.Type).c_str());
                if (has_color) ImGui::PopStyleColor();
                ImGui::SameLine();

                ImGui::TextUnformatted(item.Message.c_str());
            }

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (m_ConsoleWindowScrollToBottom || (m_ConsoleWindowAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            m_ConsoleWindowScrollToBottom = false;
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("DetailedRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None))
        {
            if (selectedLog >= 0 && selectedLog < Debug::s_Logs.size())
            {
                const auto& item = Debug::s_Logs[selectedLog];

                ImGui::PushTextWrapPos();
                ImGui::TextUnformatted(item.Message.c_str());
                ImGui::Spacing();
                ImGui::TextUnformatted(("File: " + item.File).c_str());
                ImGui::TextUnformatted(("Line: " + std::to_string(item.Line)).c_str());
                ImGui::PopTextWrapPos();

                if (ImGui::BeginPopupContextWindow())
                {
                    if (ImGui::MenuItem("Copy"))
                    {
                        ImGui::LogToClipboard();
                        ImGui::LogText("%s", item.Message.c_str());
                        ImGui::LogFinish();
                    }

                    ImGui::EndPopup();
                }
            }
            else
            {
                selectedLog = -1;
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void GameEditor::OnAppTick()
    {
        CalculateFrameStats();

        DrawImGui();
        m_RenderPipeline->Render(m_GameObjects, [this](ID3D12GraphicsCommandList* cmdList)
            {
                // Render Dear ImGui graphics
                cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(),
                    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
                cmdList->OMSetRenderTargets(1, &GetBackBufferView(), false, nullptr);

                const float clear_color_with_alpha[4] = { m_ImGUIClearColor.x * m_ImGUIClearColor.w, m_ImGUIClearColor.y * m_ImGUIClearColor.w, m_ImGUIClearColor.z * m_ImGUIClearColor.w, m_ImGUIClearColor.w };
                ID3D12DescriptorHeap* imguiDescriptorHeaps[] = { m_SrvHeap.Get() };
                cmdList->SetDescriptorHeaps(_countof(imguiDescriptorHeaps), imguiDescriptorHeaps);
                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

                cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(),
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
            });

        // swap the back and front buffers
        THROW_IF_FAILED(m_SwapChain->Present(0, 0)); // No vsync
        SwapBackBuffer();
    }

    void GameEditor::OnAppResized()
    {
        ResizeSwapChain();
    }

    void GameEditor::ResizeRenderPipeline(int width, int height)
    {
        m_RenderPipeline->Resize(width, height); // Wait for gpu idle

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_SrvHeap->GetCPUDescriptorHandleForHeapStart());
        srvHandle.Offset(1, m_CbvSrvUavDescriptorSize);
        m_Device->CreateShaderResourceView(m_RenderPipeline->GetResolvedColorTarget(), nullptr, srvHandle);
    }

    void GameEditor::ResizeSwapChain()
    {
        m_RenderPipeline->WaitForGPUIdle();
        auto [width, height] = GetApp().GetClientWidthAndHeight();

        // 先释放之前的 buffer 引用，然后才能 resize
        for (int i = 0; i < m_SwapChainBufferCount; i++)
        {
            m_SwapChainBuffers[i].Reset();
        }

        m_SwapChain->ResizeBuffers(
            m_SwapChainBufferCount,
            width,
            height,
            m_RenderPipeline->GetColorFormat(),
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
        m_CurrentBackBufferIndex = 0;

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (int i = 0; i < m_SwapChainBufferCount; i++)
        {
            THROW_IF_FAILED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffers[i])));
            m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
            rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
        }
    }

    void GameEditor::OnAppDisplayScaleChanged()
    {
        OutputDebugStringA(("DPI Changed: " + std::to_string(GetApp().GetDisplayScale()) + "\n").c_str());

        auto& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(m_FontPath, m_FontSize * GetApp().GetDisplayScale(),
            nullptr, io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        ImGui_ImplDX12_InvalidateDeviceObjects();
    }

    void GameEditor::OnAppPaint()
    {
        OnAppTick();
    }

    void GameEditor::CalculateFrameStats()
    {
        // Code computes the average frames per second, and also the 
        // average time it takes to render one frame.  These stats 
        // are appended to the window caption bar.

        static int frameCnt = 0;
        static float timeElapsed = 0.0f;

        frameCnt++;

        // Compute averages over one second period.
        if ((GetApp().GetElapsedTime() - timeElapsed) >= 1.0f)
        {
            float fps = (float)frameCnt; // fps = frameCnt / 1
            float mspf = 1000.0f / fps;

            std::wstring fpsStr = std::to_wstring(fps);
            std::wstring mspfStr = std::to_wstring(mspf);

            GetApp().SetTitle(std::wstring(L"DX12 Demo") +
                L"    fps: " + fpsStr +
                L"   mspf: " + mspfStr);

            // Reset for next average.
            frameCnt = 0;
            timeElapsed += 1.0f;
        }
    }

    void GameEditor::LogAdapters()
    {
        UINT i = 0;
        IDXGIAdapter* adapter = nullptr;

        while (m_Factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);

            DEBUG_LOG_INFO(L"***Adapter: %s", desc.Description);

            LogAdapterOutputs(adapter);
            adapter->Release();
            ++i;
        }
    }

    void GameEditor::LogAdapterOutputs(IDXGIAdapter* adapter)
    {
        UINT i = 0;
        IDXGIOutput* output = nullptr;
        while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_OUTPUT_DESC desc;
            output->GetDesc(&desc);

            DEBUG_LOG_INFO(L"***Output: %s", desc.DeviceName);

            LogOutputDisplayModes(output, m_RenderPipeline->GetColorFormat());
            output->Release();
            ++i;
        }
    }

    void GameEditor::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
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
            DEBUG_LOG_INFO(L"Width = %d, Height = %d, Refresh = %d/%d", x.Width, x.Height, n, d);
        }
    }
}
