#pragma once

#include <Windows.h>
#include <oleidl.h>

namespace march
{
    class DropManager final : public IDropTarget
    {
    public:
        DropManager(HWND hWnd);

        HRESULT QueryInterface(REFIID riid, void** ppvObject) override;

        ULONG AddRef() override;
        ULONG Release() override;

        HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
        HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
        HRESULT DragLeave() override;
        HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

        static bool Initialize(HWND hWnd);
        static bool Uninitialize(HWND hWnd);

    private:
        ULONG m_RefCount;
        const HWND m_WindowHandle;
    };
}
