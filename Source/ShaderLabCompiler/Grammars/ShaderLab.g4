grammar ShaderLab;

shader
    : 'Shader' StringLiteral LeftBrace shaderDeclaration* RightBrace EOF
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
    | TwoD
    | ThreeD
    | Cube
    | TwoDArray
    | CubeArray
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
    : 'Cull' (cullModeValue | BracketLiteral)
    ;

zTestDeclaration
    : 'ZTest' (Disabled | compareFuncValue | BracketLiteral)
    ;

zWriteDeclaration
    : 'ZWrite' (Off | On | BracketLiteral)
    ;

blendDeclaration
    : 'Blend' IntegerLiteral? (Off | (blendFactorValueOrBracketLiteral blendFactorValueOrBracketLiteral (',' blendFactorValueOrBracketLiteral blendFactorValueOrBracketLiteral)?))
    ;

blendOpDeclaration
    : 'BlendOp' IntegerLiteral? blendOpValueOrBracketLiteral (',' blendOpValueOrBracketLiteral)?
    ;

colorMaskDeclaration
    : 'ColorMask' IntegerLiteral                 # colorMaskInt1Declaration
    | 'ColorMask' IntegerLiteral IntegerLiteral  # colorMaskInt2Declaration
    | 'ColorMask' IntegerLiteral? Identifier     # colorMaskIdentifierDeclaration
    | 'ColorMask' IntegerLiteral? BracketLiteral # colorMaskBracketLiteralDeclaration
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
    : 'Ref' (IntegerLiteral | BracketLiteral)
    ;

stencilReadMaskDeclaration
    : 'ReadMask' (IntegerLiteral | BracketLiteral)
    ;

stencilWriteMaskDeclaration
    : 'WriteMask' (IntegerLiteral | BracketLiteral)
    ;

stencilCompDeclaration
    : 'Comp' (compareFuncValue | BracketLiteral)
    ;

stencilPassDeclaration
    : 'Pass' (stencilOpValue | BracketLiteral)
    ;

stencilFailDeclaration
    : 'Fail' (stencilOpValue | BracketLiteral)
    ;

stencilZFailDeclaration
    : 'ZFail' (stencilOpValue | BracketLiteral)
    ;

stencilCompFrontDeclaration
    : 'CompFront' (compareFuncValue | BracketLiteral)
    ;

stencilPassFrontDeclaration
    : 'PassFront' (stencilOpValue | BracketLiteral)
    ;

stencilFailFrontDeclaration
    : 'FailFront' (stencilOpValue | BracketLiteral)
    ;

stencilZFailFrontDeclaration
    : 'ZFailFront' (stencilOpValue | BracketLiteral)
    ;

stencilCompBackDeclaration
    : 'CompBack' (compareFuncValue | BracketLiteral)
    ;

stencilPassBackDeclaration
    : 'PassBack' (stencilOpValue | BracketLiteral)
    ;

stencilFailBackDeclaration
    : 'FailBack' (stencilOpValue | BracketLiteral)
    ;

stencilZFailBackDeclaration
    : 'ZFailBack' (stencilOpValue | BracketLiteral)
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

blendFactorValueOrBracketLiteral
    : blendFactorValue
    | BracketLiteral
    ;

blendOpValue
    : Add
    | Sub
    | RevSub
    | Min
    | Max
    ;

blendOpValueOrBracketLiteral
    : blendOpValue
    | BracketLiteral
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

TwoD
    : '2D'
    ;

ThreeD
    : '3D'
    ;

Cube
    : 'Cube'
    ;

TwoDArray
    : '2DArray'
    ;

CubeArray
    : 'CubeArray'
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
    : [ \t]+ -> skip
    ;

Newline
    : ('\r' '\n'? | '\n') -> skip
    ;

BlockComment
    : '/*' .*? '*/' -> skip
    ;

LineComment
    : '//' ~[\r\n]* -> skip
    ;
