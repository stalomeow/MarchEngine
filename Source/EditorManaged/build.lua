local m = marchmodule {
    name = "EditorManaged",
    type = "Managed",
    kind = "SharedLib",
}

assemblyname "March.Editor"
namespace "March.Editor"

uses {
    "BindingGenerator",
}

nuget {
    "glTF2Loader:1.0.0",
    "Newtonsoft.Json:13.0.3",
    "System.Collections.Immutable:9.0.3",
    "System.Net.Http:4.3.4",
    "System.Text.RegularExpressions:4.3.1",
}

usage "PUBLIC"
    uses {
        "CoreManaged",
        "ShaderLabCompiler",
    }

    runtimedeps {
        ["glTFLoader.dll"] = path.join(m.binaryDir, "glTFLoader.dll"),
        ["Newtonsoft.Json.dll"] = path.join(m.binaryDir, "Newtonsoft.Json.dll"),
    }