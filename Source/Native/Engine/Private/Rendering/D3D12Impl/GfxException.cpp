#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Application.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Profiling/NsightAftermath.h"
#include <assert.h>
#include <comdef.h>
#include <sstream>

namespace march
{
    static std::string GetErrorMessage(HRESULT hr, const std::string& expr, const std::string& filename, int line)
    {
        std::ostringstream ss{};
        ss << StringUtils::Utf16ToUtf8(_com_error(hr).ErrorMessage()) << "\n\n";
        ss << "Expression: " << expr << "\n";
        ss << "File: " << filename << "\n";
        ss << "Line: " << line << "\n";
        return ss.str();
    }

    void HandleD3D12FailureAndTerminateProcess(HRESULT hr, const std::string& expr, const std::string& filename, int line)
    {
        assert(FAILED(hr));

        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-getdeviceremovedreason
        // Gets the reason that the device was removed, or S_OK if the device isn't removed.
        ID3D12Device* device = GetGfxDevice()->GetD3DDevice4();
        HRESULT reason = device->GetDeviceRemovedReason();

        if (FAILED(reason))
        {
            std::string message = GetErrorMessage(reason, expr, filename, line);

            // 走到这个分支的话，说明 GPU 崩溃了，问题来自于 GPU，需要生成 dump
            // 问题与 CPU 无关，没必要加 CPU 断点

            if (NsightAftermath::OnGpuCrash())
            {
                GetApp()->CrashWithMessage("GPU Crash - A crash dump has been generated", message);
            }
            else
            {
                GetApp()->CrashWithMessage("GPU Crash - Failed to generate crash dump", message);
            }
        }
        else
        {
            // 走到这个分支的话，错误只来自 CPU，添加断点，方便调试

            std::string message = GetErrorMessage(hr, expr, filename, line);
            GetApp()->CrashWithMessage("D3D12 Error", message, /* debugBreak */ true);
        }
    }
}
