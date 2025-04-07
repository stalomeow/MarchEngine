lexer grammar HLSLPreprocessorLexer;

Sharp
    : '#' -> mode(DIRECTIVE_MODE)
    ;

BlockComment
    : '/*' .*? '*/' -> skip
    ;

LineComment
    : '//' ~[\r\n]* -> skip
    ;

Slash
    : '/' -> type(Code)
    ;

Code
    : ~[#/]+
    ;

mode DIRECTIVE_MODE;

Include
    : 'include' -> mode(DIRECTIVE_TEXT)
    ;

Pragma
    : 'pragma' -> mode(DIRECTIVE_TEXT)
    ;

DirectiveWhitespace
    : [ \t]+ -> skip
    ;

DirectiveBlockComment
    : '/*' .*? '*/' -> skip
    ;

DirectiveLineComment
    : '//' ~[\r\n]* -> skip
    ;

DirectiveSlash
    : '/' -> type(Code), mode(DEFAULT_MODE)
    ;

// 反斜杠换行
DirectiveNewline
    : '\\' [ \t]* ('\r' '\n'? | '\n') -> skip
    ;

DirectiveBackSlash
    : '\\' -> type(Code), mode(DEFAULT_MODE)
    ;

Newline
    : ('\r' '\n'? | '\n') -> mode(DEFAULT_MODE)
    ;

DirectiveUnknown
    : ~[ \t\r\n\\/]+ -> type(Code), mode(DEFAULT_MODE)
    ;

mode DIRECTIVE_TEXT;

// 反斜杠换行
DirectiveTextNewline
    : '\\' [ \t]* ('\r' '\n'? | '\n') -> skip
    ;

BackSlashEscape
    : '\\' . -> type(Text)
    ;

TextNewline
    : ('\r' '\n'? | '\n') -> type(Newline), mode(DEFAULT_MODE)
    ;

DirectiveTextBlockComment
    : '/*' .*? '*/' -> skip
    ;

DirectiveTextLineComment
    : '//' ~[\r\n]* -> skip
    ;

DirectiveTextSlash
    : '/' -> type(Text)
    ;

Text
    : ~[\r\n\\/]+
    ;
