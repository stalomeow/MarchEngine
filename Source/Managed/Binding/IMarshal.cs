using Microsoft.CodeAnalysis;

namespace March.Binding
{
    internal interface IMarshal
    {
        ITypeSymbol Symbol { get; set; }

        RefKind RefKind { get; set; }

        bool IsAllowed(GeneratorExecutionContext context, bool isReturnType, Location location);

        ITypeSymbol GetNativeType(GeneratorExecutionContext context);

        string BeginMarshalArgument(CodeBuilder builder, string argumentName);

        void EndMarshalArgument(CodeBuilder builder, string argumentName);

        string UnmarshalReturnValue(CodeBuilder builder, string returnValue);
    }
}
