# Asset Pipeline

在引擎中，不使用操作系统的文件路径，而是使用类似 Unity 的虚拟路径。

## 项目资产

和 Unity 一样，项目的资产都在 `Assets/` 目录下，但是提供路径时必须加上文件扩展名。

## 内置资产

引擎的内置资产，即引擎根目录中 `Resources/Assets/` 下的资产，在构建后会被拷贝到可执行文件所在的目录。在代码中，会被映射到 `Engine/Resources/` 路径。

例如，如果要加载 `Resources/Assets/Materials/DefaultLit.mat`，在代码中应该指定 `Engine/Resources/Materials/DefaultLit.mat` 路径。

引擎的内置 Shader 也是类似，构建后会被拷贝到可执行文件所在的目录。在代码中使用 `Engine/Shaders/` 路径加载 Shader，例如 `Engine/Shaders/TemporalAntialiasing.compute`。

为了实现 Shader 的热重载，在 Debug 模式下，实际会将 `Source/Shaders/Public/` 映射到 `Engine/Shaders/`。在 Release 模式下，才会映射拷贝后的目录。

## 加载方法

在 C# 中，可以使用 `AssetManager`。如果是 Editor 代码，还可以使用 `AssetDatabase`。

在 C++ 中，使用 `asset_ptr`，使用方法和 `std::unique_ptr` 差不多，可以看 `RenderPipeline` 里是怎么使用的。
