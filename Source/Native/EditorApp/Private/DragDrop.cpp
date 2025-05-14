#include "pch.h"
#include "Editor/DragDrop.h"
#include "Engine/Object.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "imgui.h"
#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace march
{
    // Ref: https://github.com/ocornut/imgui/issues/2602

    DropManager::DropManager(HWND hWnd)
        : m_RefCount(1)
        , m_WindowHandle(hWnd)
    {
    }

    HRESULT DropManager::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (riid == IID_IUnknown || riid == IID_IDropTarget)
        {
            AddRef();
            *ppvObject = this;
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG DropManager::AddRef()
    {
        return InterlockedIncrement(&m_RefCount);
    }

    ULONG DropManager::Release()
    {
        ULONG count = InterlockedDecrement(&m_RefCount);
        if (count == 0) delete this;
        return count;
    }

    static bool IsDropAccepted()
    {
        ImGuiContext* context = ImGui::GetCurrentContext();
        int acceptFrameCount = context->DragDropAcceptFrameCountActual;
        int frameCount = context->FrameCount;
        return (acceptFrameCount == frameCount) || (acceptFrameCount == frameCount - 1);
    }

    HRESULT DropManager::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
    {
        FORMATETC fmt{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg{};

        // 添加鼠标左键按下事件，参考 imgui_impl_win32.cpp
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(0, true);

        if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
        {
            if (HDROP hDrop = reinterpret_cast<HDROP>(GlobalLock(stg.hGlobal)))
            {
                DotNet::RuntimeInvoke<void>(ManagedMethod::DragDrop_HandleExternalFiles, hDrop);
                GlobalUnlock(stg.hGlobal);
            }

            ReleaseStgMedium(&stg);
        }

        *pdwEffect = IsDropAccepted() ? DROPEFFECT_COPY : DROPEFFECT_NONE;
        return S_OK;
    }

    HRESULT DropManager::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
    {
        // 添加鼠标移动事件，参考 imgui_impl_win32.cpp
        ImGuiIO& io = ImGui::GetIO();
        POINT pos = { pt.x, pt.y };
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
        {
            ScreenToClient(m_WindowHandle, &pos);
        }
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        io.AddMousePosEvent((float)pos.x, (float)pos.y);

        *pdwEffect = IsDropAccepted() ? DROPEFFECT_COPY : DROPEFFECT_NONE;
        return S_OK;
    }

    HRESULT DropManager::DragLeave()
    {
        ImGui::ClearDragDrop(); // 取消上次的拖拽
        return S_OK;
    }

    HRESULT DropManager::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
    {
        // 添加鼠标左键抬起事件，参考 imgui_impl_win32.cpp
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(0, false);

        *pdwEffect = IsDropAccepted() ? DROPEFFECT_COPY : DROPEFFECT_NONE;
        return S_OK;
    }

    bool DropManager::Initialize(HWND hWnd)
    {
        ComPtr<DropManager> manager;
        manager.Attach(MARCH_NEW DropManager(hWnd));
        return SUCCEEDED(RegisterDragDrop(hWnd, manager.Get()));
    }

    bool DropManager::Uninitialize(HWND hWnd)
    {
        return SUCCEEDED(RevokeDragDrop(hWnd));
    }
}
