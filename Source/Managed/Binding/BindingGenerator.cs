using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.Text;
using System.Collections.Generic;
using System.Text;

namespace March.Binding
{
    [Generator]
    public class BindingGenerator : ISourceGenerator
    {
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

            foreach (KeyValuePair<INamedTypeSymbol, MemberSymbolGroup> kv in receiver.Symbols)
            {
                string source = ProcessType(context, kv.Key, kv.Value);
                context.AddSource($"{kv.Key.Name}.binding.cs", SourceText.From(source, Encoding.UTF8));
            }
        }

        private string ProcessType(GeneratorExecutionContext context, INamedTypeSymbol typeSymbol, MemberSymbolGroup members)
        {
            CodeBuilder builder = new CodeBuilder(context.Compilation);

            // Begin Namespace
            if (typeSymbol.ContainingNamespace != null)
            {
                builder.AppendLine($"namespace {typeSymbol.ContainingNamespace.ToDisplayString()}");
                builder.AppendLine("{");
                builder.IndentLevel++;
            }

            // Begin Type
            builder.AppendLine($"unsafe partial {(typeSymbol.IsReferenceType ? "class" : "struct")} {typeSymbol.Name}");
            builder.AppendLine("{");
            builder.IndentLevel++;

            string nativeTypeName = GetNativeTypeName(typeSymbol, builder.TypeNameAttributeSymbol);

            foreach (IMethodSymbol methodSymbol in members.Methods)
            {
                ProcessMethod(builder, nativeTypeName, methodSymbol);
            }

            foreach (IPropertySymbol propSymbol in members.Properties)
            {
                ProcessProperty(builder, nativeTypeName, propSymbol);
            }

            builder.AppendParameterWrapperTypes();

            // End Type
            builder.IndentLevel--;
            builder.AppendLine("}");

            // End Namespace
            if (typeSymbol.ContainingNamespace != null)
            {
                builder.IndentLevel--;
                builder.AppendLine("}");
            }

            return builder.ToString();
        }

        private static string GetNativeTypeName(INamedTypeSymbol typeSymbol, INamedTypeSymbol attrSymbol)
        {
            AttributeData data = typeSymbol.GetAttributeData(attrSymbol);

            if (data != null && !data.ConstructorArguments.IsEmpty && !data.ConstructorArguments[0].IsNull)
            {
                return data.ConstructorArguments[0].Value.ToString();
            }

            return typeSymbol.Name;
        }

        private static string GetNativeMethodOrPropertyName(ISymbol symbol, INamedTypeSymbol attrSymbol, out string thisName)
        {
            thisName = "NativePtr";
            string memberName = symbol.Name;

            AttributeData data = symbol.GetAttributeData(attrSymbol);

            if (!data.ConstructorArguments.IsEmpty && !data.ConstructorArguments[0].IsNull)
            {
                memberName = data.ConstructorArguments[0].Value.ToString();
            }

            foreach (KeyValuePair<string, TypedConstant> arg in data.NamedArguments)
            {
                if (arg.Key == "This" && !arg.Value.IsNull)
                {
                    thisName = arg.Value.Value.ToString();
                }
            }

            return memberName;
        }

        private static void AppendEntryPointInitializer(CodeBuilder builder, string fieldType, string fieldName, string entryPointName)
        {
            builder.AppendLine($"if ({fieldName} == null)");
            builder.AppendLine("{");
            builder.IndentLevel++;

            builder.AppendLine("nint hModule = global::System.Runtime.InteropServices.NativeLibrary.GetMainProgramHandle();");

            builder.AppendLine($"if (!global::System.Runtime.InteropServices.NativeLibrary.TryGetExport(hModule, \"{entryPointName}\", out nint addr))");
            builder.AppendLine("{");
            builder.IndentLevel++;
            builder.AppendLine("throw new global::System.EntryPointNotFoundException();");
            builder.IndentLevel--;
            builder.AppendLine("}");

            builder.AppendLine($"{fieldName} = ({fieldType})addr;");

            builder.IndentLevel--;
            builder.AppendLine("}");
        }

        private void ProcessMethod(CodeBuilder builder, string nativeTypeName, IMethodSymbol methodSymbol)
        {
            string nativeName = GetNativeMethodOrPropertyName(methodSymbol, builder.MethodAttributeSymbol, out string thisName);
            string fieldName = $"__{methodSymbol.Name}_FunctionPointer";
            string entryPointName = $"{nativeTypeName}_{nativeName}";

            string retType = methodSymbol.ReturnType.GetFullQualifiedNameIncludeGlobal();
            var paramTypeSpaceNames = new List<string>();
            var argNames = new List<string>();
            var fieldTypeArgs = new List<string>();

            if (!methodSymbol.IsStatic)
            {
                // Add this pointer
                argNames.Add(thisName);
                fieldTypeArgs.Add(builder.GetThisParameterWrapperTypeName());
            }

            for (int i = 0; i < methodSymbol.Parameters.Length; i++)
            {
                IParameterSymbol parameter = methodSymbol.Parameters[i];

                paramTypeSpaceNames.Add($"{parameter.Type.GetFullQualifiedNameIncludeGlobal()} {parameter.Name}");
                argNames.Add(parameter.Name);
                fieldTypeArgs.Add(builder.GetParameterWrapperTypeName(parameter.Type));
            }

            fieldTypeArgs.Add(retType);
            string fieldType = $"delegate* unmanaged[Stdcall]<{string.Join(", ", fieldTypeArgs)}>";

            builder.AppendLineEditorBrowsableNever();
            builder.AppendLine($"private static {fieldType} {fieldName};");

            builder.AppendLine();
            builder.AppendLine($"{methodSymbol.GetAccessibilityAsText()}{(methodSymbol.IsStatic ? " static" : string.Empty)} partial {retType} {methodSymbol.Name}({string.Join(", ", paramTypeSpaceNames)})");
            builder.AppendLine("{");
            builder.IndentLevel++;

            AppendEntryPointInitializer(builder, fieldType, fieldName, entryPointName);

            if (methodSymbol.ReturnsVoid)
            {
                builder.AppendLine($"{fieldName}({string.Join(", ", argNames)});");
            }
            else
            {
                builder.AppendLine($"return {fieldName}({string.Join(", ", argNames)});");
            }

            builder.IndentLevel--;
            builder.AppendLine("}");
            builder.AppendLine();
        }

        private void ProcessProperty(CodeBuilder builder, string nativeTypeName, IPropertySymbol propSymbol)
        {
            string nativeName = GetNativeMethodOrPropertyName(propSymbol, builder.PropertyAttributeSymbol, out string thisName);
            string fieldNameGet = $"__{propSymbol.Name}_Getter_FunctionPointer";
            string fieldNameSet = $"__{propSymbol.Name}_Setter_FunctionPointer";
            string entryPointNameGet = $"{nativeTypeName}_Get{nativeName}";
            string entryPointNameSet = $"{nativeTypeName}_Set{nativeName}";
            string fieldTypeGet = null;
            string fieldTypeSet = null;
            string propType = propSymbol.Type.GetFullQualifiedNameIncludeGlobal();

            if (propSymbol.GetMethod != null)
            {
                if (propSymbol.IsStatic)
                {
                    fieldTypeGet = $"delegate* unmanaged[Stdcall]<{propType}>";
                }
                else
                {
                    fieldTypeGet = $"delegate* unmanaged[Stdcall]<{builder.GetThisParameterWrapperTypeName()}, {propType}>";
                }

                builder.AppendLineEditorBrowsableNever();
                builder.AppendLine($"private static {fieldTypeGet} {fieldNameGet};");
            }

            if (propSymbol.SetMethod != null)
            {
                if (propSymbol.IsStatic)
                {
                    fieldTypeSet = $"delegate* unmanaged[Stdcall]<{builder.GetParameterWrapperTypeName(propSymbol.Type)}, void>";
                }
                else
                {
                    fieldTypeSet = $"delegate* unmanaged[Stdcall]<{builder.GetThisParameterWrapperTypeName()}, {builder.GetParameterWrapperTypeName(propSymbol.Type)}, void>";
                }

                builder.AppendLineEditorBrowsableNever();
                builder.AppendLine($"private static {fieldTypeSet} {fieldNameSet};");
            }

            builder.AppendLine();

            builder.AppendLine($"{propSymbol.GetAccessibilityAsText()}{(propSymbol.IsStatic ? " static" : string.Empty)} partial {propType} {propSymbol.Name}");
            builder.AppendLine("{");
            builder.IndentLevel++;

            if (propSymbol.GetMethod != null)
            {
                if (propSymbol.GetMethod.DeclaredAccessibility != propSymbol.DeclaredAccessibility)
                {
                    builder.AppendLine($"{propSymbol.GetMethod.GetAccessibilityAsText()} get");
                }
                else
                {
                    builder.AppendLine("get");
                }

                builder.AppendLine("{");
                builder.IndentLevel++;

                AppendEntryPointInitializer(builder, fieldTypeGet, fieldNameGet, entryPointNameGet);

                if (propSymbol.IsStatic)
                {
                    builder.AppendLine($"return {fieldNameGet}();");
                }
                else
                {
                    builder.AppendLine($"return {fieldNameGet}({thisName});");
                }

                builder.IndentLevel--;
                builder.AppendLine("}");
            }

            if (propSymbol.SetMethod != null)
            {
                if (propSymbol.SetMethod.DeclaredAccessibility != propSymbol.DeclaredAccessibility)
                {
                    builder.AppendLine($"{propSymbol.SetMethod.GetAccessibilityAsText()} set");
                }
                else
                {
                    builder.AppendLine("set");
                }

                builder.AppendLine("{");
                builder.IndentLevel++;

                AppendEntryPointInitializer(builder, fieldTypeSet, fieldNameSet, entryPointNameSet);

                if (propSymbol.IsStatic)
                {
                    builder.AppendLine($"{fieldNameSet}(value);");
                }
                else
                {
                    builder.AppendLine($"{fieldNameSet}({thisName}, value);");
                }

                builder.IndentLevel--;
                builder.AppendLine("}");
            }

            builder.IndentLevel--;
            builder.AppendLine("}");
            builder.AppendLine();
        }
    }
}
