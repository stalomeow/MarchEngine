#include "Core/Debug.h"
#include "Scripting/ScriptTypes.h"

using namespace dx12demo;

struct CSharpLogStackFrame
{
    CSharpString MethodName;
    CSharpString Filename;
    CSharpInt Line;
};

NATIVE_EXPORT(void) Debug_Info(CSharpString message, CSharpLogStackFrame* pFrames, CSharpInt framCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (CSharpInt i = 0; i < framCount; i++)
    {
        stackTrace.push_back({ CSharpString_ToUtf8(pFrames[i].MethodName), CSharpString_ToUtf8(pFrames[i].Filename), pFrames[i].Line });
    }

    Debug::AddLog(stackTrace, CSharpString_ToUtf8(message), LogType::Info);
}

NATIVE_EXPORT(void) Debug_Warn(CSharpString message, CSharpLogStackFrame* pFrames, CSharpInt framCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (CSharpInt i = 0; i < framCount; i++)
    {
        stackTrace.push_back({ CSharpString_ToUtf8(pFrames[i].MethodName), CSharpString_ToUtf8(pFrames[i].Filename), pFrames[i].Line });
    }

    Debug::AddLog(stackTrace, CSharpString_ToUtf8(message), LogType::Warn);
}

NATIVE_EXPORT(void) Debug_Error(CSharpString message, CSharpLogStackFrame* pFrames, CSharpInt framCount)
{
    std::vector<LogStackFrame> stackTrace;

    for (CSharpInt i = 0; i < framCount; i++)
    {
        stackTrace.push_back({ CSharpString_ToUtf8(pFrames[i].MethodName), CSharpString_ToUtf8(pFrames[i].Filename), pFrames[i].Line });
    }

    Debug::AddLog(stackTrace, CSharpString_ToUtf8(message), LogType::Error);
}
