using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Linq;

namespace March.Binding
{
    internal static class SymbolUtility
    {
        private const string k_MethodAttributeName = "March.Core.Interop.NativeMethodAttribute";
        private const string k_PropertyAttributeName = "March.Core.Interop.NativePropertyAttribute";
        private const string k_TypeNameAttributeName = "March.Core.Interop.NativeTypeNameAttribute";

        public static INamedTypeSymbol GetMethodAttributeSymbol(Compilation compilation)
        {
            return compilation.GetTypeByMetadataName(k_MethodAttributeName);
        }

        public static INamedTypeSymbol GetPropertyAttributeSymbol(Compilation compilation)
        {
            return compilation.GetTypeByMetadataName(k_PropertyAttributeName);
        }

        public static INamedTypeSymbol GetTypeNameAttributeSymbol(Compilation compilation)
        {
            return compilation.GetTypeByMetadataName(k_TypeNameAttributeName);
        }

        public static INamedTypeSymbol GetThisTypeSymbol(Compilation compilation)
        {
            return compilation.GetSpecialType(SpecialType.System_IntPtr);
        }

        public static AttributeData GetAttributeData(this ISymbol symbol, INamedTypeSymbol attr)
        {
            return symbol.GetAttributes().FirstOrDefault(a => a.AttributeClass.Equals(attr, SymbolEqualityComparer.Default));
        }

        public static bool ShouldProcess(IMethodSymbol symbol)
        {
            return symbol != null && HasMethodAttribute(symbol) && symbol.IsPartialDefinition && !symbol.IsGenericMethod;
        }

        public static bool ShouldProcess(IPropertySymbol symbol)
        {
            return symbol != null && HasPropertyAttribute(symbol) && symbol.IsPartialDefinition;
        }

        public static bool ShouldProcess(INamedTypeSymbol symbol)
        {
            return symbol != null && IsPartialType(symbol) && !symbol.IsGenericType && symbol.ContainingType == null; // 必须是 top-level type
        }

        private static bool HasMethodAttribute(ISymbol symbol)
        {
            return symbol.GetAttributes().Any(a => a.AttributeClass.ToDisplayString() == k_MethodAttributeName);
        }

        private static bool HasPropertyAttribute(ISymbol symbol)
        {
            return symbol.GetAttributes().Any(a => a.AttributeClass.ToDisplayString() == k_PropertyAttributeName);
        }

        private static bool IsPartialType(INamedTypeSymbol symbol)
        {
            // Ref: https://stackoverflow.com/questions/68906372/roslyn-analyzer-is-class-marked-as-partial

            return symbol.DeclaringSyntaxReferences.Any(syntax =>
            {
                if (syntax.GetSyntax() is BaseTypeDeclarationSyntax declaration)
                {
                    return declaration.Modifiers.Any(modifier => modifier.IsKind(SyntaxKind.PartialKeyword));
                }

                return false;
            });
        }

        private static readonly SymbolDisplayFormat s_TypeFormat = SymbolDisplayFormat.FullyQualifiedFormat.WithGlobalNamespaceStyle(SymbolDisplayGlobalNamespaceStyle.Included);

        public static string GetFullQualifiedNameIncludeGlobal(this ITypeSymbol symbol)
        {
            return symbol.ToDisplayString(NullableFlowState.NotNull, s_TypeFormat);
        }

        public static string GetAccessibilityAsText(this ISymbol symbol)
        {
            return SyntaxFacts.GetText(symbol.DeclaredAccessibility);
        }

        public static bool ShouldWrapParameterType(ITypeSymbol type)
        {
            // 参考 https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
            // 在 x64 上 floating-point 的寄存器和 struct 不一样，所以需要包装
            // 保险起见，把所有内置的类型都包装一下

            switch (type.SpecialType)
            {
                case SpecialType.System_Enum:
                case SpecialType.System_Boolean:
                case SpecialType.System_Char:
                case SpecialType.System_SByte:
                case SpecialType.System_Byte:
                case SpecialType.System_Int16:
                case SpecialType.System_UInt16:
                case SpecialType.System_Int32:
                case SpecialType.System_UInt32:
                case SpecialType.System_Int64:
                case SpecialType.System_UInt64:
                case SpecialType.System_Decimal:
                case SpecialType.System_Single:
                case SpecialType.System_Double:
                case SpecialType.System_IntPtr:
                case SpecialType.System_UIntPtr:
                    return true;

                default:
                    return false;
            }
        }
    }
}
