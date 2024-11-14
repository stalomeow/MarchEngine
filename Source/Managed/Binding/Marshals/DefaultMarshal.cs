using Microsoft.CodeAnalysis;

namespace March.Binding.Marshals
{
    internal class DefaultMarshal : IMarshal
    {
        public ITypeSymbol Symbol { get; set; }

        public RefKind RefKind { get; set; }

        public bool IsAllowed(GeneratorExecutionContext context, bool isReturnType, Location location)
        {
            return true;
        }

        public ITypeSymbol GetNativeType(GeneratorExecutionContext context)
        {
            if (RefKind == RefKind.None)
            {
                return Symbol;
            }

            return context.Compilation.CreatePointerTypeSymbol(Symbol);
        }

        public string BeginMarshalArgument(CodeBuilder builder, string argumentName)
        {
            if (RefKind == RefKind.None)
            {
                return argumentName;
            }

            string nativePointerName = $"{argumentName}_Pointer";
            builder.AppendLine($"fixed ({Symbol.GetFullQualifiedNameIncludeGlobal()}* {nativePointerName} = &{argumentName})");
            builder.AppendLine("{");
            builder.IndentLevel++;

            return nativePointerName;
        }

        public void EndMarshalArgument(CodeBuilder builder, string argumentName)
        {
            if (RefKind != RefKind.None)
            {
                builder.IndentLevel--;
                builder.AppendLine("}");
            }
        }

        public string UnmarshalReturnValue(CodeBuilder builder, string returnValue)
        {
            if (RefKind == RefKind.None)
            {
                return returnValue;
            }

            return $"global::System.Runtime.CompilerServices.Unsafe.AsRef<{Symbol.GetFullQualifiedNameIncludeGlobal()}>((void*){returnValue})";
        }
    }
}
