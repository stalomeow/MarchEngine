using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.Text;
using System.Collections.Generic;
using System.Linq;
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
                if (!DiagnosticUtility.CheckType(context, kv.Key))
                {
                    continue;
                }

                string source = ProcessType(context, kv.Key, kv.Value);
                context.AddSource($"{kv.Key.Name}.binding.cs", SourceText.From(source, Encoding.UTF8));
            }
        }

        private string ProcessType(GeneratorExecutionContext context, INamedTypeSymbol typeSymbol, MemberSymbolGroup members)
        {
            CodeBuilder builder = new CodeBuilder(context);

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
                if (!DiagnosticUtility.CheckMethod(context, methodSymbol))
                {
                    continue;
                }

                ProcessMethod(builder, nativeTypeName, methodSymbol);
            }

            foreach (IPropertySymbol propSymbol in members.Properties)
            {
                if (!DiagnosticUtility.CheckProperty(context, propSymbol))
                {
                    continue;
                }

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

        private static bool AppendFunctionPointerField(CodeBuilder builder, IMethodSymbol methodSymbol, List<string> paramNames, List<IMarshal> marshals, string thisName, string fieldName, out string fieldType)
        {
            if (!methodSymbol.IsStatic)
            {
                // Add this pointer
                paramNames.Add(thisName);
                marshals.Add(SymbolUtility.GetMarshal(builder.ThisTypeSymbol, RefKind.None));
            }

            for (int i = 0; i < methodSymbol.Parameters.Length; i++)
            {
                IParameterSymbol parameter = methodSymbol.Parameters[i];
                paramNames.Add(parameter.Name);
                marshals.Add(SymbolUtility.GetMarshal(parameter.Type, parameter.RefKind));
            }

            // Add return value
            marshals.Add(SymbolUtility.GetMarshal(methodSymbol.ReturnType, methodSymbol.RefKind));

            for (int i = 0; i < marshals.Count; i++)
            {
                Location location;
                bool isReturnType = (i == marshals.Count - 1);

                if (isReturnType || (!methodSymbol.IsStatic && i == 0))
                {
                    location = methodSymbol.Locations.FirstOrDefault();
                }
                else
                {
                    int paramIndex = methodSymbol.IsStatic ? i : i - 1;
                    location = methodSymbol.Parameters[paramIndex].Locations.FirstOrDefault();
                }

                if (!marshals[i].IsAllowed(builder.Context, isReturnType, location))
                {
                    fieldType = null;
                    return false;
                }
            }

            string funcPtrTypeArgs = string.Join(", ", marshals.Select((m, i) =>
            {
                ITypeSymbol s = m.GetNativeType(builder.Context);

                // 最后一个是返回值，不用 wrap
                return i == marshals.Count - 1 ? s.GetFullQualifiedNameIncludeGlobal() : builder.GetParameterWrapperTypeName(s);
            }));
            fieldType = $"delegate* unmanaged[Stdcall]<{funcPtrTypeArgs}>";

            builder.AppendLineEditorBrowsableNever();
            builder.AppendLine($"private static {fieldType} {fieldName};");

            return true;
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
            builder.AppendLine($"throw new global::System.EntryPointNotFoundException(\"Can not find native entry point: {entryPointName}\");");
            builder.IndentLevel--;
            builder.AppendLine("}");

            builder.AppendLine($"{fieldName} = ({fieldType})addr;");

            builder.IndentLevel--;
            builder.AppendLine("}");
        }

        private static void AppendMethodInvocation(CodeBuilder builder, IMethodSymbol methodSymbol, List<string> paramNames, List<IMarshal> marshals, string fieldType, string fieldName, string entryPointName)
        {
            AppendEntryPointInitializer(builder, fieldType, fieldName, entryPointName);

            builder.AppendLine();

            if (!methodSymbol.ReturnsVoid)
            {
                ITypeSymbol returnType = marshals[marshals.Count - 1].GetNativeType(builder.Context);
                builder.AppendLine($"{returnType.GetFullQualifiedNameIncludeGlobal()} __ret;");
            }

            var argNames = new List<string>();

            for (int i = 0; i < marshals.Count - 1; i++)
            {
                argNames.Add(marshals[i].BeginMarshalArgument(builder, paramNames[i]));
            }

            string invoke = $"{fieldName}({string.Join(", ", argNames)})";

            if (methodSymbol.ReturnsVoid)
            {
                builder.AppendLine($"{invoke};");
            }
            else
            {
                builder.AppendLine($"__ret = {invoke};");
            }

            for (int i = 0; i < marshals.Count - 1; i++)
            {
                marshals[i].EndMarshalArgument(builder, paramNames[i]);
            }

            if (!methodSymbol.ReturnsVoid)
            {
                builder.AppendLine($"return {marshals[marshals.Count - 1].UnmarshalReturnValue(builder, "__ret")};");
            }
        }

        private bool ProcessMethod(CodeBuilder builder, string nativeTypeName, IMethodSymbol methodSymbol)
        {
            string nativeName = GetNativeMethodOrPropertyName(methodSymbol, builder.MethodAttributeSymbol, out string thisName);
            string fieldName = builder.GetUniqueName($"__{methodSymbol.Name}_FunctionPointer");
            string entryPointName = $"{nativeTypeName}_{nativeName}";

            var paramNames = new List<string>();
            var marshals = new List<IMarshal>();

            if (!AppendFunctionPointerField(builder, methodSymbol, paramNames, marshals, thisName, fieldName, out string fieldType))
            {
                return false;
            }

            builder.AppendLine();

            builder.AppendLineDebuggerHidden();
            builder.AppendLine(methodSymbol.GetFullQualifiedNameIncludeGlobal());
            builder.AppendLine("{");
            builder.IndentLevel++;

            AppendMethodInvocation(builder, methodSymbol, paramNames, marshals, fieldType, fieldName, entryPointName);

            builder.IndentLevel--;
            builder.AppendLine("}");
            builder.AppendLine();

            return true;
        }

        private bool ProcessProperty(CodeBuilder builder, string nativeTypeName, IPropertySymbol propSymbol)
        {
            string nativeName = GetNativeMethodOrPropertyName(propSymbol, builder.PropertyAttributeSymbol, out string thisName);

            string fieldNameGet = builder.GetUniqueName($"__{propSymbol.Name}_Getter_FunctionPointer");
            string entryPointNameGet = $"{nativeTypeName}_Get{nativeName}";
            string fieldTypeGet = null;
            var paramNamesGet = new List<string>();
            var marshalsGet = new List<IMarshal>();

            string fieldNameSet = builder.GetUniqueName($"__{propSymbol.Name}_Setter_FunctionPointer");
            string entryPointNameSet = $"{nativeTypeName}_Set{nativeName}";
            string fieldTypeSet = null;
            var paramNamesSet = new List<string>();
            var marshalsSet = new List<IMarshal>();

            if (propSymbol.GetMethod != null)
            {
                if (!AppendFunctionPointerField(builder, propSymbol.GetMethod, paramNamesGet, marshalsGet, thisName, fieldNameGet, out fieldTypeGet))
                {
                    return false;
                }
            }

            if (propSymbol.SetMethod != null)
            {
                if (!AppendFunctionPointerField(builder, propSymbol.SetMethod, paramNamesSet, marshalsSet, thisName, fieldNameSet, out fieldTypeSet))
                {
                    return false;
                }
            }

            builder.AppendLine();

            builder.AppendLineDebuggerHidden();
            builder.AppendLine(propSymbol.GetFullQualifiedNameIncludeGlobal());
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

                AppendMethodInvocation(builder, propSymbol.GetMethod, paramNamesGet, marshalsGet, fieldTypeGet, fieldNameGet, entryPointNameGet);

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

                AppendMethodInvocation(builder, propSymbol.SetMethod, paramNamesSet, marshalsSet, fieldTypeSet, fieldNameSet, entryPointNameSet);

                builder.IndentLevel--;
                builder.AppendLine("}");
            }

            builder.IndentLevel--;
            builder.AppendLine("}");
            builder.AppendLine();

            return true;
        }
    }
}
