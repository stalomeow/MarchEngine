#include "pch.h"
#include "Engine/Debug.h"
#include "Engine/Scripting/InteropServices.h"
#include <utility>

struct CSharpLogStackFrame
{
    cs_string MethodName;
    cs_string Filename;
    cs_int Line;
};

NATIVE_EXPORT_AUTO Log_GetMinimumLevel()
{
    retcs Log::GetMinimumLevel();
}

NATIVE_EXPORT_AUTO Log_SetMinimumLevel(cs<LogLevel> level)
{
    Log::SetMinimumLevel(level);
}

NATIVE_EXPORT_AUTO Log_GetCount(cs<LogLevel> level)
{
    retcs static_cast<int32_t>(Log::GetCount(level));
}

NATIVE_EXPORT_AUTO Log_Clear()
{
    Log::Clear();
}

NATIVE_EXPORT_AUTO Log_Message(cs<LogLevel> level, cs_string message, cs<CSharpLogStackFrame*> pFrames, cs_int frameCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (cs_int_t i = 0; i < frameCount; i++)
    {
        LogStackFrame& frame = stackTrace.emplace_back();
        frame.Function = pFrames[i].MethodName.move();
        frame.Filename = pFrames[i].Filename.move();
        frame.Line = pFrames[i].Line;
    }

    Log::Message(level, message.move(), std::move(stackTrace));
}
