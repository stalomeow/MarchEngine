using Microsoft.CodeAnalysis;
using System.Collections.Generic;
using System.Text;

namespace March.Binding
{
    internal class CodeBuilder
    {
        private readonly StringBuilder m_Builder;
        private readonly Dictionary<string, string> m_ParamWrapperTypes;
        private readonly Dictionary<string, int> m_NameCollisionCounts;

        public int IndentLevel { get; set; }

        public GeneratorExecutionContext Context { get; }

        public INamedTypeSymbol MethodAttributeSymbol { get; }

        public INamedTypeSymbol PropertyAttributeSymbol { get; }

        public INamedTypeSymbol TypeNameAttributeSymbol { get; }

        public INamedTypeSymbol ThisTypeSymbol { get; }

        public CodeBuilder(GeneratorExecutionContext context)
        {
            m_Builder = new StringBuilder();
            m_ParamWrapperTypes = new Dictionary<string, string>();
            m_NameCollisionCounts = new Dictionary<string, int>();

            IndentLevel = 0;

            Context = context;
            MethodAttributeSymbol = SymbolUtility.GetMethodAttributeSymbol(context.Compilation);
            PropertyAttributeSymbol = SymbolUtility.GetPropertyAttributeSymbol(context.Compilation);
            TypeNameAttributeSymbol = SymbolUtility.GetTypeNameAttributeSymbol(context.Compilation);
            ThisTypeSymbol = SymbolUtility.GetThisTypeSymbol(context.Compilation);
        }

        public override string ToString()
        {
            return m_Builder.ToString();
        }

        private void AppendIndent()
        {
            for (int i = 0; i < IndentLevel; i++)
            {
                m_Builder.Append("    ");
            }
        }

        public void AppendLine()
        {
            m_Builder.AppendLine();
        }

        public void AppendLine(string value)
        {
            AppendIndent();
            m_Builder.AppendLine(value);
        }

        public void AppendLineEditorBrowsableNever()
        {
            AppendLine("[global::System.ComponentModel.EditorBrowsable(global::System.ComponentModel.EditorBrowsableState.Never)]");
        }

        public void AppendLineStructLayoutSequential()
        {
            AppendLine("[global::System.Runtime.InteropServices.StructLayout(global::System.Runtime.InteropServices.LayoutKind.Sequential)]");
        }

        public void AppendLineDebuggerHidden()
        {
            AppendLine("[global::System.Diagnostics.DebuggerHidden]");
        }

        public string GetParameterWrapperTypeName(ITypeSymbol type)
        {
            string name = type.GetFullQualifiedNameIncludeGlobal();

            if (!SymbolUtility.ShouldWrapParameterType(type))
            {
                return name;
            }

            if (!m_ParamWrapperTypes.TryGetValue(name, out string wrapper))
            {
                wrapper = $"__ParameterType{m_ParamWrapperTypes.Count}_Wrapper";
                m_ParamWrapperTypes.Add(name, wrapper);
            }

            return wrapper;
        }

        public string GetThisParameterWrapperTypeName()
        {
            return GetParameterWrapperTypeName(ThisTypeSymbol);
        }

        public void AppendParameterWrapperTypes()
        {
            foreach (KeyValuePair<string, string> kv in m_ParamWrapperTypes)
            {
                AppendLineEditorBrowsableNever();
                AppendLineStructLayoutSequential();
                AppendLine($"private unsafe ref struct {kv.Value}");
                AppendLine("{");
                IndentLevel++;
                AppendLine($"public {kv.Key} Data;");
                AppendLine($"public static implicit operator {kv.Value}({kv.Key} value) => new {kv.Value} {{ Data = value }};");
                IndentLevel--;
                AppendLine("}");
                AppendLine();
            }
        }

        public string GetUniqueName(string name)
        {
            if (!m_NameCollisionCounts.TryGetValue(name, out int count))
            {
                m_NameCollisionCounts[name] = 1;
                return name;
            }

            m_NameCollisionCounts[name] = count + 1;
            return $"{name}{count}";
        }
    }
}
