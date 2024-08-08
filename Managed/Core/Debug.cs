using DX12Demo.Core.Binding;
using System.Buffers;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DX12Demo.Core
{
    public static unsafe partial class Debug
    {
        public static void LogInfo(object message)
        {
            StackTrace stackTrace = new(skipFrames: 1, fNeedFileInfo: true); // 需要跳过当前栈帧
            Log(message.ToString() ?? "Null", LogType.Info, stackTrace);
        }

        public static void LogWarning(object message)
        {
            StackTrace stackTrace = new(skipFrames: 1, fNeedFileInfo: true); // 需要跳过当前栈帧
            Log(message.ToString() ?? "Null", LogType.Warning, stackTrace);
        }

        public static void LogError(object message)
        {
            StackTrace stackTrace = new(skipFrames: 1, fNeedFileInfo: true); // 需要跳过当前栈帧
            Log(message.ToString() ?? "Null", LogType.Error, stackTrace);
        }

        public static void LogException(Exception exception)
        {
            // TODO: Handle inner exception?
            StackTrace stackTrace = new(exception, fNeedFileInfo: true);
            Log($"{exception.GetType().FullName}: {exception.Message}", LogType.Error, stackTrace);
        }

        public static void Assert(bool condition, string message, [CallerArgumentExpression(nameof(condition))] string expr = "")
        {
            if (condition)
            {
                return;
            }

            StackTrace stackTrace = new(skipFrames: 1, fNeedFileInfo: true); // 需要跳过当前栈帧
            Log($"Assertion failed ({expr}): {message}", LogType.Error, stackTrace);
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct LogStackFrame
        {
            public nint MethodName;
            public nint Filename;
            public int Line;
        }

        private enum LogType
        {
            Info,
            Warning,
            Error
        }

        private static void Log(string message, LogType type, StackTrace stackTrace)
        {
            int frameCount = stackTrace.FrameCount;
            LogStackFrame[] frames = ArrayPool<LogStackFrame>.Shared.Rent(frameCount);

            using NativeString messageNative = message;

            try
            {
                for (int i = 0; i < frameCount; i++)
                {
                    StackFrame frame = stackTrace.GetFrame(i)!;
                    string filename = frame.GetFileName() ?? "<unknown file>";

                    MethodBase? method = frame.GetMethod();
                    string methodName = method?.ToString() ?? "<unknown method>";

                    if (method?.DeclaringType != null)
                    {
                        methodName = $"{method.DeclaringType.FullName}: {methodName}";
                    }

                    frames[i].MethodName = NativeString.New(methodName);
                    frames[i].Filename = NativeString.New(filename);
                    frames[i].Line = frame.GetFileLineNumber();
                }

                fixed (LogStackFrame* pFrame = frames)
                {
                    switch (type)
                    {
                        case LogType.Info: Debug_Info(messageNative.Data, pFrame, frameCount); break;
                        case LogType.Warning: Debug_Warn(messageNative.Data, pFrame, frameCount); break;
                        case LogType.Error: Debug_Error(messageNative.Data, pFrame, frameCount); break;
                        default: throw new NotImplementedException($"Unknown log type: {type}");
                    }
                }
            }
            finally
            {
                for (int i = 0; i < frameCount; i++)
                {
                    NativeString.Free(frames[i].MethodName);
                    NativeString.Free(frames[i].Filename);
                }

                ArrayPool<LogStackFrame>.Shared.Return(frames);
            }
        }

        [NativeFunction]
        private static partial void Debug_Info(nint message, LogStackFrame* pFrames, int frameCount);

        [NativeFunction]
        private static partial void Debug_Warn(nint message, LogStackFrame* pFrames, int frameCount);

        [NativeFunction]
        private static partial void Debug_Error(nint message, LogStackFrame* pFrames, int frameCount);
    }
}
