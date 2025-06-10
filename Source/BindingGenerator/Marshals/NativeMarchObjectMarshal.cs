using Microsoft.CodeAnalysis;

namespace March.Binding.Marshals
{
    internal class NativeMarchObjectMarshal : IMarshal
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

            if (isReturnType)
            {
                DiagnosticUtility.ReportCannotBeReturnType(context, Symbol, location);
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
            if (Symbol.NullableAnnotation == NullableAnnotation.Annotated)
            {
                return $"{argumentName}?.NativePtr ?? nint.Zero";
            }
            else
            {
                return $"{argumentName}.NativePtr";
            }
        }

        public void EndMarshalArgument(CodeBuilder builder, string argumentName) { }

        public string UnmarshalReturnValue(CodeBuilder builder, string returnValue)
        {
            return returnValue;
        }
    }
}
