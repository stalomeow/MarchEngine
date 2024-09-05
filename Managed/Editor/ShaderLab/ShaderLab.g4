grammar ShaderLab;

shader
    : 'Shader' StringLiteral LeftBrace shaderDeclaration* RightBrace
    ;

shaderDeclaration
    : propertiesBlock
    | passBlock
    ;

propertiesBlock
    : 'Properties' LeftBrace propertyDeclaration* RightBrace
    ;

passBlock
    : 'Pass' LeftBrace passDeclaration* RightBrace
    ;

passDeclaration
    : nameDeclaration
    | cullDeclaration
    | zTestDeclaration
    | zWriteDeclaration
    | blendDeclaration
    | blendOpDeclaration
    | colorMaskDeclaration
    | stencilBlock
    | hlslProgramDeclaration
    ;

attributeDeclaration
    : BracketLiteral
    ;

numberLiteralExpression
    : IntegerLiteral
    | FloatLiteral
    ;

vectorLiteralExpression
    : '(' numberLiteralExpression ',' numberLiteralExpression ',' numberLiteralExpression ',' numberLiteralExpression ')'
    ;

textureLiteralExpression
    : StringLiteral LeftBrace RightBrace
    ;

propertyDefaultValueExpression
    : numberLiteralExpression
    | vectorLiteralExpression
    | textureLiteralExpression
    ;

propertyTypeDeclaration
    : Float
    | Int
    | Color
    | Vector
    | Texture
    ;

propertyDeclaration
    : attributeDeclaration* Identifier '(' StringLiteral ',' propertyTypeDeclaration ')' Assign propertyDefaultValueExpression
    ;

nameDeclaration
    : 'Name' StringLiteral
    ;

cullDeclaration
    : 'Cull' cullModeValue
    ;

zTestDeclaration
    : 'ZTest' (Off | compareFuncValue)
    ;

zWriteDeclaration
    : 'ZWrite' (Off | On)
    ;

blendDeclaration
    : 'Blend' IntegerLiteral (Off | (blendFactorValue blendFactorValue ',' blendFactorValue blendFactorValue))
    ;

blendOpDeclaration
    : 'BlendOp' IntegerLiteral blendOpValue ',' blendOpValue
    ;

colorMaskDeclaration
    : 'ColorMask' IntegerLiteral (IntegerLiteral | Identifier)
    ;

stencilBlock
    : 'Stencil' LeftBrace stencilDeclaration* RightBrace
    ;

stencilDeclaration
    : stencilRefDeclaration
    | stencilReadMaskDeclaration
    | stencilWriteMaskDeclaration
    | stencilCompFrontDeclaration
    | stencilPassFrontDeclaration
    | stencilFailFrontDeclaration
    | stencilZFailFrontDeclaration
    | stencilCompBackDeclaration
    | stencilPassBackDeclaration
    | stencilFailBackDeclaration
    | stencilZFailBackDeclaration
    ;

stencilRefDeclaration
    : 'Ref' IntegerLiteral
    ;

stencilReadMaskDeclaration
    : 'ReadMask' IntegerLiteral
    ;

stencilWriteMaskDeclaration
    : 'WriteMask' IntegerLiteral
    ;

stencilCompFrontDeclaration
    : 'CompFront' compareFuncValue
    ;

stencilPassFrontDeclaration
    : 'PassFront' stencilOpValue
    ;

stencilFailFrontDeclaration
    : 'FailFront' stencilOpValue
    ;

stencilZFailFrontDeclaration
    : 'ZFailFront' stencilOpValue
    ;

stencilCompBackDeclaration
    : 'CompBack' compareFuncValue
    ;

stencilPassBackDeclaration
    : 'PassBack' stencilOpValue
    ;

stencilFailBackDeclaration
    : 'FailBack' stencilOpValue
    ;

stencilZFailBackDeclaration
    : 'ZFailBack' stencilOpValue
    ;

hlslProgramDeclaration
    : HlslProgram
    ;

HlslProgram
    : 'HLSLPROGRAM' .*? 'ENDHLSL'
    ;

cullModeValue
    : Off
    | Front
    | Back
    ;

blendFactorValue
    : Zero
    | One
    | SrcColor
    | InvSrcColor
    | SrcAlpha
    | InvSrcAlpha
    | DestAlpha
    | InvDestAlpha
    | DestColor
    | InvDestColor
    | SrcAlphaSat
    ;

blendOpValue
    : Add
    | Sub
    | RevSub
    | Min
    | Max
    ;

compareFuncValue
    : Never
    | Less
    | Equal
    | LessEqual
    | Greater
    | NotEqual
    | GreaterEqual
    | Always
    ;

stencilOpValue
    : Keep
    | Zero
    | Replace
    | IncrSat
    | DecrSat
    | Invert
    | Incr
    | Decr
    ;

Assign
    : '='
    ;

LeftBrace
    : '{'
    ;

RightBrace
    : '}'
    ;

StringLiteral
    : '"' (~["\r\n])* '"'
    ;

BracketLiteral
    : '[' (~[\]\r\n])* ']'
    ;

fragment Digit
    : [0-9]
    ;

fragment HexadecimalDigit
    : [0-9a-fA-F]
    ;

IntegerLiteral
    : Digit+
    | '0' [xX] HexadecimalDigit+
    ;

FloatLiteral
    : Digit+ '.' Digit*
    | '.' Digit+
    ;

Float
    : 'Float'
    ;

Int
    : 'Int'
    ;

Color
    : 'Color'
    ;

Vector
    : 'Vector'
    ;

Texture
    : '2D'
    ;

On
    : 'On'
    ;

Off
    : 'Off'
    ;

Front
    : 'Front'
    ;

Back
    : 'Back'
    ;

Zero
    : 'Zero'
    ;

One
    : 'One'
    ;

SrcColor
    : 'SrcColor'
    ;

InvSrcColor
    : 'InvSrcColor'
    ;

SrcAlpha
    : 'SrcAlpha'
    ;

InvSrcAlpha
    : 'InvSrcAlpha'
    ;

DestAlpha
    : 'DestAlpha'
    ;

InvDestAlpha
    : 'InvDestAlpha'
    ;

DestColor
    : 'DestColor'
    ;

InvDestColor
    : 'InvDestColor'
    ;

SrcAlphaSat
    : 'SrcAlphaSat'
    ;

Add
    : 'Add'
    ;

Sub
    : 'Sub'
    ;

RevSub
    : 'RevSub'
    ;

Min
    : 'Min'
    ;

Max
    : 'Max'
    ;

Never
    : 'Never'
    ;

Less
    : 'Less'
    ;

Equal
    : 'Equal'
    ;

LessEqual
    : 'LessEqual'
    ;

Greater
    : 'Greater'
    ;

NotEqual
    : 'NotEqual'
    ;

GreaterEqual
    : 'GreaterEqual'
    ;

Always
    : 'Always'
    ;

Keep
    : 'Keep'
    ;

Replace
    : 'Replace'
    ;

IncrSat
    : 'IncrSat'
    ;

DecrSat
    : 'DecrSat'
    ;

Invert
    : 'Invert'
    ;

Incr
    : 'Incr'
    ;

Decr
    : 'Decr'
    ;

Identifier
    : [a-zA-Z_][a-zA-Z0-9_]*
    ;

Whitespace
    : [ \t]+ -> channel(HIDDEN)
    ;

Newline
    : ('\r' '\n'? | '\n') -> skip
    ;

BlockComment
    : '/*' .*? '*/' -> channel(HIDDEN)
    ;

LineComment
    : '//' ~[\r\n]* -> channel(HIDDEN)
    ;
