using System.Buffers;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;

namespace DX12Demo.Binding
{
    internal unsafe partial class EntryPoint
    {
        public static event Action? OnTick = OnTickPrint;
        private static bool s_Ticked = false;

        [NativeFunction]
        public static partial void Debug_LogInfo(int stackTraceFrameCount, FixedString* methods, FixedString* filenames, int* lines, string message);

        [NativeFunction]
        public static partial void Debug_LogWarn(int stackTraceFrameCount, FixedString* methods, FixedString* filenames, int* lines, string message);

        [NativeFunction]
        public static partial void Debug_LogError(int stackTraceFrameCount, FixedString* methods, FixedString* filenames, int* lines, string message);

        [UnmanagedCallersOnly]
        private static void OnNativeTick()
        {
            OnTick?.Invoke();
        }

        private static void OnTickPrint()
        {
            if (!s_Ticked)
            {
                Log("Hello C#");
                s_Ticked = true;
            }
        }

        private static void Log(string format, params object[] args)
        {
            // 需要跳过当前栈帧
            StackTrace stackTrace = new(skipFrames: 1, fNeedFileInfo: true);

            int frameCount = stackTrace.FrameCount;
            FixedString[] methods = ArrayPool<FixedString>.Shared.Rent(frameCount);
            GCHandle[] methodHandles = ArrayPool<GCHandle>.Shared.Rent(frameCount);
            FixedString[] filenames = ArrayPool<FixedString>.Shared.Rent(frameCount);
            GCHandle[] filenameHandles = ArrayPool<GCHandle>.Shared.Rent(frameCount);
            int[] lineNumbers = ArrayPool<int>.Shared.Rent(frameCount);

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

                methods[i] = FixedString.Pin(methodName, out methodHandles[i]);
                filenames[i] = FixedString.Pin(filename, out filenameHandles[i]);
                lineNumbers[i] = frame.GetFileLineNumber();
            }

            fixed (FixedString* methodPtr = methods)
            fixed (FixedString* filenamesPtr = filenames)
            fixed (int* lineNumbersPtr = lineNumbers)
            {
                Debug_LogInfo(frameCount, methodPtr, filenamesPtr, lineNumbersPtr, string.Format(format, args));
            }

            for (int i = 0; i < frameCount; i++)
            {
                methodHandles[i].Free();
                filenameHandles[i].Free();
            }

            ArrayPool<FixedString>.Shared.Return(methods);
            ArrayPool<GCHandle>.Shared.Return(methodHandles);
            ArrayPool<FixedString>.Shared.Return(filenames);
            ArrayPool<GCHandle>.Shared.Return(filenameHandles);
            ArrayPool<int>.Shared.Return(lineNumbers);
        }
    }
}
