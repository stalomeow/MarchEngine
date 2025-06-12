local m = marchmodule {
    name = "ShaderLabCompiler",
    type = "Managed",
    kind = "SharedLib",
}

assemblyname "March.ShaderLab"
namespace "March.ShaderLab"

disablewarnings {
    "1701",
    "1702",
    "3021",
}

nuget {
    "Antlr4.Runtime.Standard:4.13.1",
    "Newtonsoft.Json:13.0.3",
    "System.Collections.Immutable:9.0.3",
    "System.Text.RegularExpressions:4.3.1",
}

usage "PUBLIC"
    uses {
        "CoreManaged",
    }

    runtimedeps {
        ["Antlr4.Runtime.Standard.dll"] = path.join(m.binaryDir, "Antlr4.Runtime.Standard.dll"),
        ["Newtonsoft.Json.dll"] = path.join(m.binaryDir, "Newtonsoft.Json.dll"),
    }
