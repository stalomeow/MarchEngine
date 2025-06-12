local m = marchmodule {
    name = "CoreManaged",
    type = "Managed",
    kind = "SharedLib",
}

assemblyname "March.Core"
namespace "March.Core"

uses {
    "BindingGenerator",
}

nuget {
    "Newtonsoft.Json:13.0.3",
    "System.Text.RegularExpressions:4.3.1",
}

usage "PUBLIC"
    runtimedeps {
        ["Newtonsoft.Json.dll"] = path.join(m.binaryDir, "Newtonsoft.Json.dll"),
    }