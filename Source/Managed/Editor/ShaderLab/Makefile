ANTLR = antlr4
ARGS = -Dlanguage=CSharp -package "March.Editor.ShaderLab.Internal" -visitor -no-listener -o ./Internal

csharp:
	$(ANTLR) ShaderLab.g4 $(ARGS)
.PHONY: csharp
