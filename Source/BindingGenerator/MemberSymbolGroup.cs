using Microsoft.CodeAnalysis;
using System.Collections.Generic;

namespace March.Binding
{
    internal class MemberSymbolGroup
    {
        public List<IMethodSymbol> Methods { get; } = new List<IMethodSymbol>();

        public List<IPropertySymbol> Properties { get; } = new List<IPropertySymbol>();
    }
}
