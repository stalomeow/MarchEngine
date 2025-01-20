using Microsoft.CodeAnalysis;

namespace March.Binding.Marshals
{
    internal class BoolMarshal : IMarshal
    {
        public ITypeSymbol Symbol { get; set; }

        public RefKind RefKind { get; set; }

        public bool IsAllowed(GeneratorExecutionContext context, bool isReturnType, Location location)
        {
            if (isReturnType && RefKind != RefKind.None)
            {
                DiagnosticUtility.ReportCannotBeReturnType(context, Symbol, location);
                return false;
            }

            return true;
        }

        public ITypeSymbol GetNativeType(GeneratorExecutionContext context)
        {
            ITypeSymbol type = context.Compilation.GetSpecialType(SpecialType.System_Int32);

            if (RefKind != RefKind.None)
            {
                type = context.Compilation.CreatePointerTypeSymbol(type);
            }

            return type;
        }

        public string BeginMarshalArgument(CodeBuilder builder, string argumentName)
        {
            string valueName = $"{argumentName}_Value";

            if (RefKind == RefKind.Out)
            {
                builder.AppendLine($"int {valueName} = 0;");
            }
            else
            {
                builder.AppendLine($"int {valueName} = {argumentName} ? 1 : 0;");
            }

            return RefKind == RefKind.None ? valueName : $"&{valueName}";
        }

        public void EndMarshalArgument(CodeBuilder builder, string argumentName)
        {
            if (RefKind == RefKind.Ref || RefKind == RefKind.Out)
            {
                builder.AppendLine($"{argumentName} = {argumentName}_Value != 0;");
            }
        }

        public string UnmarshalReturnValue(CodeBuilder builder, string returnValue)
        {
            return $"{returnValue} != 0";
        }
    }
}
