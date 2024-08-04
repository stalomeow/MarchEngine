using System.Collections.Generic;
using System.Linq;
using System.Reflection.Metadata;
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
        public const string FixedStringText = @"
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DX12Demo.Binding
{
    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct FixedString
    {
        public char* Data;
        public int Length;

        public static FixedString Pin(string s, out GCHandle handle)
        {
            handle = GCHandle.Alloc(s, GCHandleType.Pinned);
            char* data = (char*)Unsafe.AsPointer(ref MemoryMarshal.GetReference(s.AsSpan()));
            return new FixedString { Data = data, Length = s.Length };
        }
    }
}
";

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
        public static unsafe delegate* unmanaged[Stdcall]<global::DX12Demo.Binding.FixedString, nint> LookUpFn;

        [global::System.Runtime.InteropServices.UnmanagedCallersOnly]
        [global::System.ComponentModel.EditorBrowsable(global::System.ComponentModel.EditorBrowsableState.Never)]
        public static unsafe void SetLookUpFn(nint fn)
        {
            LookUpFn = (delegate* unmanaged[Stdcall]<global::DX12Demo.Binding.FixedString, nint>)fn;
        }
    }
}

#nullable restore

";

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForPostInitialization(i =>
            {
                i.AddSource("FixedString.g.cs", SourceText.From(FixedStringText, Encoding.UTF8));
                i.AddSource("NativeFunctionAttribute.g.cs", SourceText.From(AttributeText, Encoding.UTF8));
            });
            context.RegisterForSyntaxNotifications(() => new SyntaxReceiver());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is SyntaxReceiver receiver))
            {
                return;
            }

            INamedTypeSymbol attrSymbol = context.Compilation.GetTypeByMetadataName("DX12Demo.Binding.NativeFunctionAttribute");
            INamedTypeSymbol stringSymbol = context.Compilation.GetTypeByMetadataName("System.String");

            foreach (var group in receiver.Methods.GroupBy<IMethodSymbol, INamedTypeSymbol>(f => f.ContainingType, SymbolEqualityComparer.Default))
            {
                string classSource = ProcessClass(context, attrSymbol, stringSymbol, group.Key, group);
                context.AddSource($"{group.Key.Name}_Binding.g.cs", SourceText.From(classSource, Encoding.UTF8));
            }
        }

        private string ProcessClass(GeneratorExecutionContext context, INamedTypeSymbol attrSymbol, INamedTypeSymbol stringSymbol, INamedTypeSymbol classSymbol, IEnumerable<IMethodSymbol> methods)
        {
            if (!classSymbol.ContainingSymbol.Equals(classSymbol.ContainingNamespace, SymbolEqualityComparer.Default))
            {
                return null; //TODO: issue a diagnostic that it must be top level
            }

            StringBuilder source = new StringBuilder($@"
namespace {classSymbol.ContainingNamespace.ToDisplayString()}
{{
    unsafe partial class {classSymbol.Name}
    {{
");

            foreach (IMethodSymbol methodSymbol in methods)
            {
                ProcessMethod(source, methodSymbol, attrSymbol, stringSymbol);
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

        private void ProcessMethod(StringBuilder source, IMethodSymbol methodSymbol, INamedTypeSymbol attrSymbol, INamedTypeSymbol stringSymbol)
        {
            if (!methodSymbol.IsPartialDefinition || !methodSymbol.IsStatic || methodSymbol.IsAsync || methodSymbol.IsGenericMethod)
            {
                return; // TODO: issue a diagnostic that it must be static
            }

            string fieldName = $"{methodSymbol.Name}_FunctionPointer";

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
            source.Append($"                char* ___keyData = stackalloc char[] {{ ");
            foreach(char c in methodSymbol.Name)
            {
                source.Append($"'\\u{(ushort)c:X4}', ");
            }
            source.AppendLine($"'\\0' }};"); // additional null terminator to be consistent with .NET's string implementation
            source.AppendLine($"                var ___key = new global::DX12Demo.Binding.FixedString {{ Data = ___keyData, Length = {methodSymbol.Name.Length} }};");
            source.AppendLine($"                {fieldName} = global::DX12Demo.Binding.NativeFunctionAttribute.LookUpFn(___key);");
            source.AppendLine($"                if ({fieldName} == nint.Zero)");
            source.AppendLine($"                {{");
            source.AppendLine($"                    throw new global::System.EntryPointNotFoundException();");
            source.AppendLine($"                }}");
            source.AppendLine($"            }}");

            foreach (IParameterSymbol parameter in methodSymbol.Parameters)
            {
                if (!parameter.Type.Equals(stringSymbol, SymbolEqualityComparer.Default))
                {
                    continue;
                }

                source.AppendLine($"            fixed (char* {parameter.Name}_ptr = {parameter.Name})");
                source.AppendLine($"            {{");
                source.AppendLine($"            var {parameter.Name}_fixed = new global::DX12Demo.Binding.FixedString {{ Data = {parameter.Name}_ptr, Length = {parameter.Name}.Length }};");
            }

            source.Append("            ");

            if (!methodSymbol.ReturnsVoid)
            {
                source.Append("return ");
            }

            source.Append($"((");

            source.Append("delegate* unmanaged[Stdcall]<");

            foreach (IParameterSymbol parameter in methodSymbol.Parameters)
            {
                string paramType;

                if (parameter.Type.Equals(stringSymbol, SymbolEqualityComparer.Default))
                {
                    paramType = "global::DX12Demo.Binding.FixedString";
                }
                else
                {
                    paramType = FullQualifiedNameIncludeGlobal(parameter.Type);
                }

                source.Append($"{paramType}, ");
            }

            source.Append($"{FullQualifiedNameIncludeGlobal(methodSymbol.ReturnType)}>){fieldName})(");

            for (int i = 0; i < methodSymbol.Parameters.Length; i++)
            {
                if (i > 0)
                {
                    source.Append(", ");
                }

                IParameterSymbol parameter = methodSymbol.Parameters[i];

                if (parameter.Type.Equals(stringSymbol, SymbolEqualityComparer.Default))
                {
                    source.Append($"{parameter.Name}_fixed");
                }
                else
                {
                    source.Append(parameter.Name);
                }
            }

            source.AppendLine(");");

            foreach (IParameterSymbol parameter in methodSymbol.Parameters)
            {
                if (!parameter.Type.Equals(stringSymbol, SymbolEqualityComparer.Default))
                {
                    continue;
                }

                source.AppendLine($"            }}");
            }

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
