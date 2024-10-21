#include "AssetManger.h"
#include "DotNetRuntime.h"
#include "DotNetMarshal.h"

namespace march
{
    void* AssetManager::LoadAsset(const std::string& path)
    {
        cs_defer_destroy<cs_string> arg{};

        arg.v.assign(path);
        return DotNet::RuntimeInvoke<void*, void*>(ManagedMethod::AssetManager_NativeLoadAsset, arg.v.data);
    }

    void AssetManager::UnloadAsset(void* asset)
    {
        DotNet::RuntimeInvoke<void, void*>(ManagedMethod::AssetManager_NativeUnloadAsset, asset);
    }
}
