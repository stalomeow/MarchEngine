using March.Core.Interop;
using March.Core.Pool;
using System.ComponentModel;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace March.Core.Diagnostics
{
    public static unsafe partial class Log
    {
        public static LogLevel MinimumLevel
        {
            get => Log_GetMinimumLevel();
            set => Log_SetMinimumLevel(value);
        }

        public static bool EnableFullStackTrace { get; set; } = true;

        public static void Message(LogLevel level, StringLike message,
            [CallerMemberName] string memberName = "", [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            var context = new InterpolatedStringContext(0, 0, level, message, out _, memberName, filePath, lineNumber);
            context.SendMessage(skipFrames: 2); // 跳过 SendMessage 和当前方法的栈帧
        }

        public static void Message(LogLevel level, StringLike message,
            [InterpolatedStringHandlerArgument(nameof(level), nameof(message))] ref InterpolatedStringContext context)
        {
            context.SendMessage(skipFrames: 2); // 跳过 SendMessage 和当前方法的栈帧
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct LogStackFrame
        {
            public nint MethodName;
            public nint Filename;
            public int Line;
        }

        private static PooledObject<StringBuilder> BuildMethodName(StackFrame frame)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();

            MethodBase? method = frame.GetMethod();
            string methodName = method?.ToString() ?? "<unknown method>";

            if (method?.DeclaringType != null)
            {
                builder.Value.Append(method.DeclaringType.FullName).Append(": ");
            }

            builder.Value.Append(methodName);
            return builder;
        }

        [InterpolatedStringHandler]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public ref struct InterpolatedStringContext
        {
            public LogLevel Level { get; }

            public string CallerMemberName { get; }

            public string CallerFilePath { get; }

            public int CallerLineNumber { get; }

            public StringBuilder? Builder { get; private set; }

            public InterpolatedStringContext(int literalLength, int formattedCount, LogLevel level, StringLike message, out bool isEnabled,
                [CallerMemberName] string memberName = "", [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
            {
                Level = level;
                CallerMemberName = memberName;
                CallerFilePath = filePath;
                CallerLineNumber = lineNumber;

                if (level >= MinimumLevel)
                {
                    isEnabled = true;
                    Builder = StringBuilderPool.Shared.Rent();
                    Builder.Append(message);
                }
                else
                {
                    isEnabled = false;
                    Builder = null;
                }
            }

            public void SendMessage(int skipFrames)
            {
                if (Builder == null)
                {
                    return;
                }

                using NativeString message = Builder;
                using var frames = ListPool<LogStackFrame>.Get();

                try
                {
                    if (EnableFullStackTrace)
                    {
                        var stackTrace = new StackTrace(skipFrames, fNeedFileInfo: true);

                        for (int i = 0; i < stackTrace.FrameCount; i++)
                        {
                            StackFrame frame = stackTrace.GetFrame(i)!;
                            using var methodName = BuildMethodName(frame);

                            frames.Value.Add(new LogStackFrame
                            {
                                MethodName = NativeString.New(methodName),
                                Filename = NativeString.New(frame.GetFileName() ?? "<unknown file>"),
                                Line = frame.GetFileLineNumber()
                            });
                        }
                    }
                    else
                    {
                        frames.Value.Add(new LogStackFrame
                        {
                            MethodName = NativeString.New(CallerMemberName),
                            Filename = NativeString.New(CallerFilePath),
                            Line = CallerLineNumber
                        });
                    }

                    fixed (LogStackFrame* pFrames = CollectionsMarshal.AsSpan(frames.Value))
                    {
                        Log_Message(Level, message.Data, pFrames, frames.Value.Count);
                    }
                }
                finally
                {
                    for (int i = 0; i < frames.Value.Count; i++)
                    {
                        NativeString.Free(frames.Value[i].MethodName);
                        NativeString.Free(frames.Value[i].Filename);
                    }

                    StringBuilderPool.Shared.Return(Builder);
                    Builder = null;
                }
            }

            #region Append

            public readonly void AppendLiteral(string literal) { }

            public readonly void AppendFormatted(bool value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(sbyte value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(byte value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(short value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(ushort value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(int value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(uint value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(long value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(ulong value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(float value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(double value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(decimal value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(char value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(ReadOnlyMemory<char> value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(ReadOnlySpan<char> value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(char[]? value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(string? value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(StringBuilder? value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted(StringLike value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value);
            }

            public readonly void AppendFormatted<T>(T value, [CallerArgumentExpression(nameof(value))] string expr = "")
            {
                Builder?.Append(' ').Append(expr).Append('=').Append(value?.ToString());
            }

            #endregion
        }

        #region Bindings

        [NativeFunction]
        private static partial LogLevel Log_GetMinimumLevel();

        [NativeFunction]
        private static partial void Log_SetMinimumLevel(LogLevel value);

        [NativeFunction(Name = "Log_GetCount")]
        public static partial int GetCount(LogLevel level);

        [NativeFunction(Name = "Log_Clear")]
        public static partial void Clear();

        [NativeFunction]
        private static partial void Log_Message(LogLevel level, nint message, LogStackFrame* pFrames, int frameCount);

        #endregion
    }
}
