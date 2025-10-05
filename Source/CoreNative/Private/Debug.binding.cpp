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
    stl::string msg(*message.data, MemoryLabel::Debug);
    stl::vector<LogStackFrame> stackTrace(frameCount, MemoryLabel::Debug);

    for (cs_int_t i = 0; i < frameCount; i++)
    {
        LogStackFrame& frame = stackTrace[static_cast<size_t>(i)];
        frame.Function = stl::string(*pFrames[i].MethodName.data, MemoryLabel::Debug);
        frame.Filename = stl::string(*pFrames[i].Filename.data, MemoryLabel::Debug);
        frame.Line = pFrames[i].Line;
    }

    Log::Message(level, std::move(msg), std::move(stackTrace));
}
