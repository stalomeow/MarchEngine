using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using March.Core.Pool;
using March.ShaderLab.Internal;

namespace March.ShaderLab
{
    internal sealed class HLSLPreprocessorVisitor : HLSLPreprocessorParserBaseVisitor<int>
    {
        public List<string> Includes { get; } = [];

        public List<string> Pragmas { get; } = [];

        public override int VisitPreprocessorInclude([NotNull] HLSLPreprocessorParser.PreprocessorIncludeContext context)
        {
            if (context.directiveText() != null)
            {
                string path = context.directiveText().GetText().Trim();

                if (path.Length >= 2)
                {
                    Includes.Add(path[1..^1]); // Remove the quotes
                }
            }

            return base.VisitPreprocessorInclude(context);
        }

        public override int VisitPreprocessorPragma([NotNull] HLSLPreprocessorParser.PreprocessorPragmaContext context)
        {
            if (context.directiveText() != null)
            {
                using var args = ListPool<string>.Get();

                foreach (IParseTree child in context.directiveText().children)
                {
                    args.Value.Add(child.GetText().Trim());
                }

                string pragma = string.Join(" ", args.Value);

                if (!string.IsNullOrWhiteSpace(pragma))
                {
                    Pragmas.Add(pragma);
                }
            }

            return base.VisitPreprocessorPragma(context);
        }
    }
}
