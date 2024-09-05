ANTLR = "C:\Users\10247\Downloads\antlr-4.13.2-complete.jar"
ARGS = -Dlanguage=CSharp -package "DX12Demo.Editor.ShaderLab.Internal" -visitor -no-listener -o ./Internal

csharp:
	java -jar $(ANTLR) ShaderLab.g4 $(ARGS)
.PHONY: csharp
