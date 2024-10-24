using Antlr4.Runtime;
using System.Collections.Immutable;

namespace March.Editor.ShaderLab
{
    internal sealed class ShaderLabErrorListener(string file) : IAntlrErrorListener<int>, IAntlrErrorListener<IToken>
    {
        public ImmutableArray<string> Errors { get; private set; } = [];

        public bool HasError => !Errors.IsEmpty;

        void IAntlrErrorListener<int>.SyntaxError(TextWriter output, IRecognizer recognizer, int offendingSymbol, int line, int charPositionInLine, string msg, RecognitionException e)
        {
            // 和 hlsl 编译器保持一致
            Errors = Errors.Add($"{file}:{line}:{charPositionInLine}: {msg}");
        }

        void IAntlrErrorListener<IToken>.SyntaxError(TextWriter output, IRecognizer recognizer, IToken offendingSymbol, int line, int charPositionInLine, string msg, RecognitionException e)
        {
            // 和 hlsl 编译器保持一致
            Errors = Errors.Add($"{file}:{line}:{charPositionInLine}: {msg}");
        }

        public void AddException(Exception e)
        {
            Errors = Errors.Add($"{file}: compiler error: {e}");
        }
    }
}
