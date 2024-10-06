using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace March.Binding
{
    [Generator]
    public class BindingGenerator : ISourceGenerator
    {
        private const string NativeFuncAttrName = "March.Core.Binding.NativeFunctionAttribute";

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForSyntaxNotifications(() => new SyntaxReceiver());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is SyntaxReceiver receiver))
            {
                return;
            }

            INamedTypeSymbol attrSymbol = context.Compilation.GetTypeByMetadataName(NativeFuncAttrName);

            if (attrSymbol == null)
            {
                return;
            }

            foreach (var group in receiver.Methods.GroupBy<IMethodSymbol, INamedTypeSymbol>(f => f.ContainingType, SymbolEqualityComparer.Default))
            {
                string classSource = ProcessClass(context, attrSymbol, group.Key, group);
                context.AddSource($"{group.Key.Name}_Binding.g.cs", SourceText.From(classSource, Encoding.UTF8));
            }
        }

        private string ProcessClass(GeneratorExecutionContext context, INamedTypeSymbol attrSymbol, INamedTypeSymbol classSymbol, IEnumerable<IMethodSymbol> methods)
        {
            if (!classSymbol.ContainingSymbol.Equals(classSymbol.ContainingNamespace, SymbolEqualityComparer.Default))
            {
                return null; //TODO: issue a diagnostic that it must be top level
            }

            StringBuilder source = new StringBuilder();
            source.Append($@"
namespace {classSymbol.ContainingNamespace.ToDisplayString()}
{{
    unsafe partial {(classSymbol.IsReferenceType ? "class" : "struct")} {classSymbol.Name}");

            if (classSymbol.TypeParameters.Length > 0)
            {
                source.Append("<");
                for (int i = 0; i < classSymbol.TypeParameters.Length; i++)
                {
                    if (i > 0)
                    {
                        source.Append(", ");
                    }

                    source.Append(classSymbol.TypeParameters[i].Name);
                }
                source.Append(">");
            }

            source.AppendLine("\n    {");

            var paramWrapperTypes = new Dictionary<string, string>();

            foreach (IMethodSymbol methodSymbol in methods)
            {
                ProcessMethod(source, paramWrapperTypes, methodSymbol, attrSymbol);
            }

            GenerateParameterWrappers(source, paramWrapperTypes);
            source.Append("} }");
            return source.ToString();
        }

        private static SymbolDisplayFormat FullyQualifiedFormatIncludeGlobal { get; } =
            SymbolDisplayFormat.FullyQualifiedFormat.WithGlobalNamespaceStyle(SymbolDisplayGlobalNamespaceStyle.Included);

        private static string FullQualifiedNameIncludeGlobal(ITypeSymbol symbol)
        {
            return symbol.ToDisplayString(NullableFlowState.NotNull, FullyQualifiedFormatIncludeGlobal);
        }

        private void ProcessMethod(StringBuilder source, Dictionary<string, string> paramWrapperTypes, IMethodSymbol methodSymbol, INamedTypeSymbol attrSymbol)
        {
            if (!methodSymbol.IsPartialDefinition || !methodSymbol.IsStatic || methodSymbol.IsAsync || methodSymbol.IsGenericMethod)
            {
                return; // TODO: issue a diagnostic that it must be static
            }

            string fieldName = $"__{methodSymbol.Name}_FunctionPointer";
            string entryPointName = methodSymbol.Name;

            AttributeData attrData = methodSymbol.GetAttributes().First(a => a.AttributeClass.Equals(attrSymbol, SymbolEqualityComparer.Default));
            foreach (KeyValuePair<string, TypedConstant> arg in attrData.NamedArguments)
            {
                if (arg.Key == "Name" && !arg.Value.IsNull)
                {
                    entryPointName = arg.Value.Value.ToString();
                }
            }

            source.AppendLine();
            source.AppendLine("        [global::System.ComponentModel.EditorBrowsable(global::System.ComponentModel.EditorBrowsableState.Never)]");
            source.AppendLine($"        private static nint {fieldName};");

            source.AppendLine();
            source.Append($"        {SyntaxFacts.GetText(methodSymbol.DeclaredAccessibility)} static partial {FullQualifiedNameIncludeGlobal(methodSymbol.ReturnType)} {methodSymbol.Name}(");

            for (int i = 0; i < methodSymbol.Parameters.Length; i++)
            {
                if (i > 0)
                {
                    source.Append(", ");
                }

                IParameterSymbol parameter = methodSymbol.Parameters[i];
                source.Append($"{FullQualifiedNameIncludeGlobal(parameter.Type)} {parameter.Name}");
            }

            source.AppendLine(")");
            source.AppendLine("        {");

            source.AppendLine($"            if ({fieldName} == nint.Zero)");
            source.AppendLine($"            {{");
            source.AppendLine($"                nint hModule = global::System.Runtime.InteropServices.NativeLibrary.GetMainProgramHandle();");
            source.AppendLine($"                if (!global::System.Runtime.InteropServices.NativeLibrary.TryGetExport(hModule, \"{entryPointName}\", out {fieldName}))");
            source.AppendLine($"                {{");
            source.AppendLine($"                    throw new global::System.EntryPointNotFoundException();");
            source.AppendLine($"                }}");
            source.AppendLine($"            }}");

            source.Append("            ");

            if (!methodSymbol.ReturnsVoid)
            {
                source.Append("return ");
            }

            source.Append($"((");

            source.Append("delegate* unmanaged[Stdcall]<");

            foreach (IParameterSymbol parameter in methodSymbol.Parameters)
            {
                string paramType = FullQualifiedNameIncludeGlobal(parameter.Type);

                if (!paramWrapperTypes.TryGetValue(paramType, out string wrapperType))
                {
                    wrapperType = $"__CS_Param_Wrapper{paramWrapperTypes.Count}";
                    paramWrapperTypes.Add(paramType, wrapperType);
                }

                source.Append($"{wrapperType}, ");
            }

            source.Append($"{FullQualifiedNameIncludeGlobal(methodSymbol.ReturnType)}>){fieldName})(");

            for (int i = 0; i < methodSymbol.Parameters.Length; i++)
            {
                if (i > 0)
                {
                    source.Append(", ");
                }

                source.Append(methodSymbol.Parameters[i].Name);
            }

            source.AppendLine(");");
            source.AppendLine("        }");
        }

        private void GenerateParameterWrappers(StringBuilder source, Dictionary<string, string> types)
        {
            foreach (KeyValuePair<string, string> type in types)
            {
                source.AppendLine($"private unsafe struct {type.Value}");
                source.AppendLine($"{{");
                source.AppendLine($"    public {type.Key} data;");
                source.AppendLine($"    public static implicit operator {type.Value}({type.Key} value) => new {type.Value} {{ data = value }};");
                source.AppendLine($"}}");
                source.AppendLine();
            }
        }

        internal class SyntaxReceiver : ISyntaxContextReceiver
        {
            public List<IMethodSymbol> Methods { get; } = new List<IMethodSymbol>();

            public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
            {
                if (!(context.Node is MethodDeclarationSyntax methodDeclarationSyntax))
                {
                    return;
                }

                var methodSymbol = context.SemanticModel.GetDeclaredSymbol(methodDeclarationSyntax) as IMethodSymbol;
                if (methodSymbol.GetAttributes().Any(a => a.AttributeClass.ToDisplayString() == NativeFuncAttrName))
                {
                    Methods.Add(methodSymbol);
                }
            }
        }
    }
}
