parser grammar HLSLPreprocessorParser;

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
    : Text+
    ;
