local m = marchmodule {
    name = "CoreManaged",
    type = "Managed",
    kind = "SharedLib",
}

assemblyname "March.Core"
namespace "March.Core"

vsprops {
    EnableDynamicLoading = "true",
    ServerGarbageCollection = "true",
    ConcurrentGarbageCollection = "true",
    GarbageCollectionAdaptationMode = "1",
}

uses {
    "BindingGenerator",
}

nuget {
    "Newtonsoft.Json:13.0.3",
    "System.Text.RegularExpressions:4.3.1",
}

usage "PUBLIC"
    runtimedeps {
        ["March.Core.runtimeconfig.json"] = path.join(m.binaryDir, "March.Core.runtimeconfig.json"),
        ["Newtonsoft.Json.dll"] = path.join(m.binaryDir, "Newtonsoft.Json.dll"),
    }