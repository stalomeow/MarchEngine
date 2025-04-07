parser grammar HLSLPreprocessorParser;

// 参考 Objective-C 2.0 grammars 示例中的 Two-step processing
// https://github.com/antlr/grammars-v4/tree/master/objc#two-step-processing

options {
    tokenVocab = HLSLPreprocessorLexer;
}

program
    : text* EOF
    ;

text
    : Sharp directive? Newline
    | Sharp directive? EOF
    | Sharp code
    | code
    ;

code
    : Code+
    ;

directive
    : Include directiveText? # preprocessorInclude
    | Pragma directiveText?  # preprocessorPragma
    ;

directiveText
    : DirectiveText+
    ;
