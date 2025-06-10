local m = marchmodule {
    name = "BindingGenerator",
    type = "Managed",
    kind = "SourceGenerator",
}

assemblyname "March.Binding"
namespace "March.Binding"

vsprops {
    EnforceExtendedAnalyzerRules = "true",
}

files {
    -- https://stackoverflow.com/questions/69655715/roslyn-how-to-fix-rs2008-warning
    "AnalyzerReleases.Shipped.md",
    "AnalyzerReleases.Unshipped.md",
}