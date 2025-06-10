using Microsoft.CodeAnalysis;

namespace March.Binding.Marshals
{
    internal class StringMarshal : IMarshal
    {
        public ITypeSymbol Symbol { get; set; }

        public RefKind RefKind { get; set; }

        public bool IsAllowed(GeneratorExecutionContext context, bool isReturnType, Location location)
        {
            if (RefKind != RefKind.None)
            {
                DiagnosticUtility.ReportCannotPassByRef(context, Symbol, location);
                return false;
            }

            return true;
        }

        public ITypeSymbol GetNativeType(GeneratorExecutionContext context)
        {
            return context.Compilation.GetSpecialType(SpecialType.System_IntPtr);
        }

        public string BeginMarshalArgument(CodeBuilder builder, string argumentName)
        {
            string nativeName = $"{argumentName}_Native";
            builder.AppendLine($"using global::March.Core.Interop.NativeString {nativeName} = {argumentName};");
            return $"{nativeName}.Data";
        }

        public void EndMarshalArgument(CodeBuilder builder, string argumentName) { }

        public string UnmarshalReturnValue(CodeBuilder builder, string returnValue)
        {
            return $"global::March.Core.Interop.NativeString.GetAndFree({returnValue})";
        }
    }
}
