#include "Debug.h"
#include "InteropServices.h"

struct CSharpLogStackFrame
{
    cs_string MethodName;
    cs_string Filename;
    cs_int Line;
};

NATIVE_EXPORT_AUTO Debug_Info(cs_string message, cs<CSharpLogStackFrame*> pFrames, cs_int frameCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (cs_int_t i = 0; i < frameCount; i++)
    {
        stackTrace.push_back({ pFrames[i].MethodName, pFrames[i].Filename, pFrames[i].Line });
    }

    Debug::AddLog(stackTrace, message, LogType::Info);
}

NATIVE_EXPORT_AUTO Debug_Warn(cs_string message, cs<CSharpLogStackFrame*> pFrames, cs_int frameCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (cs_int_t i = 0; i < frameCount; i++)
    {
        stackTrace.push_back({ pFrames[i].MethodName, pFrames[i].Filename, pFrames[i].Line });
    }

    Debug::AddLog(stackTrace, message, LogType::Warn);
}

NATIVE_EXPORT_AUTO Debug_Error(cs_string message, cs<CSharpLogStackFrame*> pFrames, cs_int frameCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (cs_int_t i = 0; i < frameCount; i++)
    {
        stackTrace.push_back({ pFrames[i].MethodName, pFrames[i].Filename, pFrames[i].Line });
    }

    Debug::AddLog(stackTrace, message, LogType::Error);
}
