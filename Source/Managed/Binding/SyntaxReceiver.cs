using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;

namespace March.Binding
{
    internal sealed class SyntaxReceiver : ISyntaxContextReceiver
    {
        public Dictionary<INamedTypeSymbol, MemberSymbolGroup> Symbols { get; }

        public SyntaxReceiver()
        {
            Symbols = new Dictionary<INamedTypeSymbol, MemberSymbolGroup>(SymbolEqualityComparer.Default);
        }

        private MemberSymbolGroup GetMemberSymbolGroup(ISymbol symbol)
        {
            INamedTypeSymbol type = symbol.ContainingType;

            if (!Symbols.TryGetValue(type, out MemberSymbolGroup group))
            {
                group = new MemberSymbolGroup();
                Symbols.Add(type, group);
            }

            return group;
        }

        public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
        {
            if (context.Node is MethodDeclarationSyntax mds)
            {
                IMethodSymbol methodSymbol = context.SemanticModel.GetDeclaredSymbol(mds);

                if (SymbolUtility.HasMethodAttribute(methodSymbol))
                {
                    GetMemberSymbolGroup(methodSymbol).Methods.Add(methodSymbol);
                }
            }
            else if (context.Node is PropertyDeclarationSyntax pds)
            {
                IPropertySymbol propertySymbol = context.SemanticModel.GetDeclaredSymbol(pds);

                if (SymbolUtility.HasPropertyAttribute(propertySymbol))
                {
                    GetMemberSymbolGroup(propertySymbol).Properties.Add(propertySymbol);
                }
            }
        }
    }
}
