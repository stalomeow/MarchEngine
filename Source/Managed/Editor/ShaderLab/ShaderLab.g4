grammar ShaderLab;

shader
    : 'Shader' StringLiteral LeftBrace shaderDeclaration* RightBrace
    ;

shaderDeclaration
    : propertiesBlock
    | tagsBlock
    | renderStateDeclaration
    | hlslIncludeDeclaration
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
    | tagsBlock
    | renderStateDeclaration
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

renderStateDeclaration
    : cullDeclaration
    | zTestDeclaration
    | zWriteDeclaration
    | blendDeclaration
    | blendOpDeclaration
    | colorMaskDeclaration
    | stencilBlock
    ;

tagsBlock
    : 'Tags' LeftBrace tagDeclaration* RightBrace
    ;

tagDeclaration
    : StringLiteral Assign StringLiteral
    ;

cullDeclaration
    : 'Cull' cullModeValue
    ;

zTestDeclaration
    : 'ZTest' (Disabled | compareFuncValue)
    ;

zWriteDeclaration
    : 'ZWrite' (Off | On)
    ;

blendDeclaration
    : 'Blend' IntegerLiteral? (Off | (blendFactorValue blendFactorValue (',' blendFactorValue blendFactorValue)?))
    ;

blendOpDeclaration
    : 'BlendOp' IntegerLiteral? blendOpValue (',' blendOpValue)?
    ;

colorMaskDeclaration
    : 'ColorMask' IntegerLiteral                # colorMaskInt1Declaration
    | 'ColorMask' IntegerLiteral IntegerLiteral # colorMaskInt2Declaration
    | 'ColorMask' IntegerLiteral? Identifier    # colorMaskIdentifierDeclaration
    ;

stencilBlock
    : 'Stencil' LeftBrace stencilDeclaration* RightBrace
    ;

stencilDeclaration
    : stencilRefDeclaration
    | stencilReadMaskDeclaration
    | stencilWriteMaskDeclaration
    | stencilCompDeclaration
    | stencilPassDeclaration
    | stencilFailDeclaration
    | stencilZFailDeclaration
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

stencilCompDeclaration
    : 'Comp' compareFuncValue
    ;

stencilPassDeclaration
    : 'Pass' stencilOpValue
    ;

stencilFailDeclaration
    : 'Fail' stencilOpValue
    ;

stencilZFailDeclaration
    : 'ZFail' stencilOpValue
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

hlslIncludeDeclaration
    : HlslInclude
    ;

hlslProgramDeclaration
    : HlslProgram
    ;

HlslInclude
    : 'HLSLINCLUDE' .*? 'ENDHLSL'
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
    | OneMinusSrcColor
    | SrcAlpha
    | OneMinusSrcAlpha
    | DstAlpha
    | OneMinusDstAlpha
    | DstColor
    | OneMinusDstColor
    | SrcAlphaSaturate
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
    | LEqual
    | Greater
    | NotEqual
    | GEqual
    | Always
    ;

stencilOpValue
    : Keep
    | Zero
    | Replace
    | IncrSat
    | DecrSat
    | Invert
    | IncrWrap
    | DecrWrap
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

Disabled
    : 'Disabled'
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

OneMinusSrcColor
    : 'OneMinusSrcColor'
    ;

SrcAlpha
    : 'SrcAlpha'
    ;

OneMinusSrcAlpha
    : 'OneMinusSrcAlpha'
    ;

DstAlpha
    : 'DstAlpha'
    ;

OneMinusDstAlpha
    : 'OneMinusDstAlpha'
    ;

DstColor
    : 'DstColor'
    ;

OneMinusDstColor
    : 'OneMinusDstColor'
    ;

SrcAlphaSaturate
    : 'SrcAlphaSaturate'
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

LEqual
    : 'LEqual'
    ;

Greater
    : 'Greater'
    ;

NotEqual
    : 'NotEqual'
    ;

GEqual
    : 'GEqual'
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

IncrWrap
    : 'IncrWrap'
    ;

DecrWrap
    : 'DecrWrap'
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
