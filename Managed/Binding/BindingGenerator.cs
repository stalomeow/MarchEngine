using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace DX12Demo.BindingSourceGen
{
    [Generator]
    public class BindingGenerator : ISourceGenerator
    {
        public const string AttributeText = @"
using System;

#nullable enable

namespace DX12Demo.Binding
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    internal sealed class NativeFunctionAttribute : Attribute
    {
        public string? Name { get; set; }

        [global::System.ComponentModel.EditorBrowsable(global::System.ComponentModel.EditorBrowsableState.Never)]
        public static unsafe delegate* unmanaged[Stdcall]<char*, int, nint> LookUpFn;

        [global::System.Runtime.InteropServices.UnmanagedCallersOnly]
        [global::System.ComponentModel.EditorBrowsable(global::System.ComponentModel.EditorBrowsableState.Never)]
        public static unsafe void SetLookUpFn(nint fn)
        {
            LookUpFn = (delegate* unmanaged[Stdcall]<char*, int, nint>)fn;
        }
    }
}

#nullable restore

";

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForPostInitialization(i => i.AddSource("NativeFunctionAttribute.g.cs", SourceText.From(AttributeText, Encoding.UTF8)));
            context.RegisterForSyntaxNotifications(() => new SyntaxReceiver());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is SyntaxReceiver receiver))
            {
                return;
            }

            INamedTypeSymbol attrSymbol = context.Compilation.GetTypeByMetadataName("DX12Demo.Binding.NativeFunctionAttribute");

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

            StringBuilder source = new StringBuilder($@"
namespace {classSymbol.ContainingNamespace.ToDisplayString()}
{{
    unsafe partial {(classSymbol.IsReferenceType ? "class" : "struct")} {classSymbol.Name}
    {{
");

            foreach (IMethodSymbol methodSymbol in methods)
            {
                ProcessMethod(source, methodSymbol, attrSymbol);
            }

            source.Append("} }");
            return source.ToString();
        }

        private static SymbolDisplayFormat FullyQualifiedFormatIncludeGlobal { get; } =
            SymbolDisplayFormat.FullyQualifiedFormat.WithGlobalNamespaceStyle(SymbolDisplayGlobalNamespaceStyle.Included);

        private static string FullQualifiedNameIncludeGlobal(ITypeSymbol symbol)
        {
            return symbol.ToDisplayString(NullableFlowState.NotNull, FullyQualifiedFormatIncludeGlobal);
        }

        private void ProcessMethod(StringBuilder source, IMethodSymbol methodSymbol, INamedTypeSymbol attrSymbol)
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
            source.Append($"                char* __pKey = stackalloc char[] {{ ");
            foreach(char c in entryPointName)
            {
                source.Append($"'\\u{(ushort)c:X4}', ");
            }
            source.AppendLine("'\\0' };"); // additional null terminator to be consistent with .NET's string implementation
            source.AppendLine($"                {fieldName} = global::DX12Demo.Binding.NativeFunctionAttribute.LookUpFn(__pKey, {entryPointName.Length});");
            source.AppendLine($"                if ({fieldName} == nint.Zero)");
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
                source.Append($"{paramType}, ");
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
                if (methodSymbol.GetAttributes().Any(a => a.AttributeClass.ToDisplayString() == "DX12Demo.Binding.NativeFunctionAttribute"))
                {
                    Methods.Add(methodSymbol);
                }
            }
        }
    }
}
