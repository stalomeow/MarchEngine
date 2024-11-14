using March.Binding.Marshals;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Immutable;
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

        public static bool HasMethodAttribute(ISymbol symbol)
        {
            return symbol.GetAttributes().Any(a => a.AttributeClass.ToDisplayString() == k_MethodAttributeName);
        }

        public static bool HasPropertyAttribute(ISymbol symbol)
        {
            return symbol.GetAttributes().Any(a => a.AttributeClass.ToDisplayString() == k_PropertyAttributeName);
        }

        public static bool IsPartialType(INamedTypeSymbol symbol)
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

        private static readonly SymbolDisplayFormat s_TypeFormat = SymbolDisplayFormat.FullyQualifiedFormat.WithGlobalNamespaceStyle(SymbolDisplayGlobalNamespaceStyle.Included).WithMiscellaneousOptions(SymbolDisplayMiscellaneousOptions.UseSpecialTypes | SymbolDisplayMiscellaneousOptions.IncludeNullableReferenceTypeModifier);
        private static readonly SymbolDisplayFormat s_ParamFormat = s_TypeFormat.WithParameterOptions(SymbolDisplayParameterOptions.IncludeModifiers | SymbolDisplayParameterOptions.IncludeType | SymbolDisplayParameterOptions.IncludeName); // partial 的部分可以不要默认参数值
        private static readonly SymbolDisplayFormat s_MethodFormat = s_ParamFormat.WithMemberOptions(SymbolDisplayMemberOptions.IncludeAccessibility | SymbolDisplayMemberOptions.IncludeModifiers | SymbolDisplayMemberOptions.IncludeType | SymbolDisplayMemberOptions.IncludeParameters | SymbolDisplayMemberOptions.IncludeRef);
        private static readonly SymbolDisplayFormat s_PropertyFormat = s_TypeFormat.WithMemberOptions(SymbolDisplayMemberOptions.IncludeAccessibility | SymbolDisplayMemberOptions.IncludeModifiers | SymbolDisplayMemberOptions.IncludeType | SymbolDisplayMemberOptions.IncludeRef);

        public static string GetFullQualifiedNameIncludeGlobal(this ITypeSymbol symbol)
        {
            return symbol.ToDisplayString(NullableFlowState.NotNull, s_TypeFormat);
        }

        public static string GetAccessibilityAsText(this ISymbol symbol)
        {
            return SyntaxFacts.GetText(symbol.DeclaredAccessibility);
        }

        public static string GetFullQualifiedNameIncludeGlobal(this IParameterSymbol symbol)
        {
            return symbol.ToDisplayString(s_ParamFormat);
        }

        public static string GetFullQualifiedNameIncludeGlobal(this IMethodSymbol symbol)
        {
            ImmutableArray<SymbolDisplayPart> parts = symbol.ToDisplayParts(s_MethodFormat);
            parts = InsertPartialKeywordBeforeType(parts);
            return parts.ToDisplayString();
        }

        public static string GetFullQualifiedNameIncludeGlobal(this IPropertySymbol symbol)
        {
            ImmutableArray<SymbolDisplayPart> parts = symbol.ToDisplayParts(s_PropertyFormat);
            parts = InsertPartialKeywordBeforeType(parts);
            return parts.ToDisplayString();
        }

        private static ImmutableArray<SymbolDisplayPart> InsertPartialKeywordBeforeType(ImmutableArray<SymbolDisplayPart> parts)
        {
            for (int i = 0; i < parts.Length; i++)
            {
                // keyword 和 space 都没有 symbol，类型名（比如 nint）有 symbol
                if (parts[i].Symbol == null)
                {
                    if (parts[i].Kind == SymbolDisplayPartKind.Space)
                    {
                        continue;
                    }

                    if (parts[i].Kind == SymbolDisplayPartKind.Keyword)
                    {
                        SyntaxKind kind = SyntaxFacts.GetKeywordKind(parts[i].ToString());

                        if (!SyntaxFacts.IsPredefinedType(kind) && kind != SyntaxKind.GlobalKeyword)
                        {
                            continue;
                        }
                    }
                }

                // 跳过 modifier keyword 和 space 后第一个就是 type，partial 必须插入在 type 前面
                return parts.InsertRange(i, new[]
                {
                    new SymbolDisplayPart(SymbolDisplayPartKind.Keyword, null, "partial"),
                    new SymbolDisplayPart(SymbolDisplayPartKind.Space, null, " ")
                });
            }

            return parts;
        }

        public static bool ShouldWrapParameterType(ITypeSymbol type)
        {
            // 参考 https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
            // 在 x64 上 floating-point 的寄存器和 struct 不一样，所以需要用 struct 包装
            // 保险起见，把所有内置的类型都包装一下

            switch (type.SpecialType)
            {
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
            }

            switch (type.TypeKind)
            {
                case TypeKind.Enum:
                case TypeKind.Pointer:
                case TypeKind.FunctionPointer:
                    return true;
            }

            return false;
        }

        public static bool IsNativeMarchObject(ITypeSymbol symbol)
        {
            while (symbol != null)
            {
                if (symbol.ToDisplayString() == "March.Core.NativeMarchObject")
                {
                    return true;
                }

                symbol = symbol.BaseType;
            }

            return false;
        }

        public static IMarshal GetMarshal(ITypeSymbol symbol, RefKind refKind)
        {
            IMarshal marshal;

            if (IsNativeMarchObject(symbol))
            {
                marshal = new NativeMarchObjectMarshal();
            }
            else if (symbol.SpecialType == SpecialType.System_String)
            {
                marshal = new StringMarshal();
            }
            else
            {
                switch (symbol.ToDisplayString())
                {
                    case "March.Core.Interop.StringLike":
                    case "System.Span<char>":
                    case "System.ReadOnlySpan<char>":
                    case "System.Text.StringBuilder":
                        marshal = new StringMarshal();
                        break;

                    default:
                        marshal = new DefaultMarshal();
                        break;
                }
            }

            marshal.Symbol = symbol;
            marshal.RefKind = refKind;
            return marshal;
        }
    }
}
