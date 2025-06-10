@echo off

set files=ShaderLab.g4 HLSLPreprocessorLexer.g4 HLSLPreprocessorParser.g4
set args=-Dlanguage=CSharp -package "March.ShaderLab.Internal" -visitor -no-listener
cd Grammars && antlr4 %files% -o ../Internal %args%