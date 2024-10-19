#include "AssetManger.h"
#include "DotNetRuntime.h"
#include "DotNetMarshal.h"

namespace march
{
    void* AssetManager::LoadAsset(const std::string& path)
    {
        cs_string csPath{};
        csPath.assign(path);
        void* asset = DotNet::RuntimeInvoke<void*, void*>(ManagedMethod::AssetManager_NativeLoadAsset, csPath.data);
        cs_string::destroy(csPath);
        return asset;
    }

    void AssetManager::UnloadAsset(void* asset)
    {
        DotNet::RuntimeInvoke<void, void*>(ManagedMethod::AssetManager_NativeUnloadAsset, asset);
    }
}
