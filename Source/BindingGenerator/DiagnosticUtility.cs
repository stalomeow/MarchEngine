using Microsoft.CodeAnalysis;
using System.Linq;

namespace March.Binding
{
    internal static class DiagnosticUtility
    {
        private static readonly DiagnosticDescriptor s_TypeMustBeTopLevelDesc = new DiagnosticDescriptor(
                id: "MA0001",
                title: "Type must be top-level",
                messageFormat: "The type '{0}' must be top-level because it contains native members",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_TypeMustNotBeGenericDesc = new DiagnosticDescriptor(
                id: "MA0002",
                title: "Type must not be generic",
                messageFormat: "The type '{0}' must not be generic because it contains native members",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_TypeMustBePartialDesc = new DiagnosticDescriptor(
                id: "MA0003",
                title: "Type must be partial",
                messageFormat: "The type '{0}' must have the 'partial' modifier because it contains native members",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_MethodMustNotBeGenericDesc = new DiagnosticDescriptor(
                id: "MA0004",
                title: "Method must not be generic",
                messageFormat: "The native method '{0}' must not be generic",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_MethodMustBePartialDesc = new DiagnosticDescriptor(
                id: "MA0005",
                title: "Method must be partial",
                messageFormat: "The native method '{0}' must have the 'partial' modifier",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_PropertyMustBePartialDesc = new DiagnosticDescriptor(
                id: "MA0006",
                title: "Property must be partial",
                messageFormat: "The native property '{0}' must have the 'partial' modifier",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_CannotPassByRefDesc = new DiagnosticDescriptor(
                id: "MA0007",
                title: "Type cannot be passed by reference",
                messageFormat: "The parameter or return value cannot have the 'in', 'ref readonly', 'ref', or 'out' modifiers because it is of type '{0}'",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        private static readonly DiagnosticDescriptor s_CannotBeReturnTypeDesc = new DiagnosticDescriptor(
                id: "MA0008",
                title: "Type cannot be returned",
                messageFormat: "The return value cannot be of type '{0}'",
                category: "Interop",
                defaultSeverity: DiagnosticSeverity.Error,
                isEnabledByDefault: true);

        public static bool CheckType(GeneratorExecutionContext context, INamedTypeSymbol symbol)
        {
            if (symbol.ContainingType != null)
            {
                context.ReportDiagnostic(Diagnostic.Create(s_TypeMustBeTopLevelDesc, symbol.Locations.FirstOrDefault(), symbol.Name));
                return false;
            }

            if (symbol.IsGenericType)
            {
                context.ReportDiagnostic(Diagnostic.Create(s_TypeMustNotBeGenericDesc, symbol.Locations.FirstOrDefault(), symbol.Name));
                return false;
            }

            if (!SymbolUtility.IsPartialType(symbol))
            {
                context.ReportDiagnostic(Diagnostic.Create(s_TypeMustBePartialDesc, symbol.Locations.FirstOrDefault(), symbol.Name));
                return false;
            }

            return true;
        }

        public static bool CheckMethod(GeneratorExecutionContext context, IMethodSymbol symbol)
        {
            if (symbol.IsGenericMethod)
            {
                context.ReportDiagnostic(Diagnostic.Create(s_MethodMustNotBeGenericDesc, symbol.Locations.FirstOrDefault(), symbol.Name));
                return false;
            }

            if (!symbol.IsPartialDefinition)
            {
                context.ReportDiagnostic(Diagnostic.Create(s_MethodMustBePartialDesc, symbol.Locations.FirstOrDefault(), symbol.Name));
                return false;
            }

            return true;
        }

        public static bool CheckProperty(GeneratorExecutionContext context, IPropertySymbol symbol)
        {
            if (!symbol.IsPartialDefinition)
            {
                context.ReportDiagnostic(Diagnostic.Create(s_PropertyMustBePartialDesc, symbol.Locations.FirstOrDefault(), symbol.Name));
                return false;
            }

            return true;
        }

        public static void ReportCannotPassByRef(GeneratorExecutionContext context, ITypeSymbol symbol, Location location)
        {
            context.ReportDiagnostic(Diagnostic.Create(s_CannotPassByRefDesc, location, symbol));
        }

        public static void ReportCannotBeReturnType(GeneratorExecutionContext context, ITypeSymbol symbol, Location location)
        {
            context.ReportDiagnostic(Diagnostic.Create(s_CannotBeReturnTypeDesc, location, symbol));
        }
    }
}
