# Build

先劝退一下。项目现在处于玩具阶段，代码也比较复杂，还经常有 Breaking Change，而且我没空写非常详细的文档，请做好心理准备。

## 要求

- Windows x64
- Windows SDK 10.0
- [Visual Studio 2022](https://visualstudio.microsoft.com/zh-hans/vs/)
- vcpkg，参考微软文档中 [Set up vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell#1---set-up-vcpkg) 部分来配置，里面几个步骤都要做

## 项目结构

- Binaries
  - NsightAftermath：[NVIDIA Nsight Aftermath SDK](https://developer.nvidia.com/nsight-aftermath)
  - Runtime：[.NET Runtime](https://dotnet.microsoft.com/en-us/download)
- Documentation：文档
- Resources：内置资源
- Source
  - Managed
    - Binding：C# Binding 生成器
    - Core：引擎核心的 C# 代码
    - Editor：编辑器的 C# 代码
    - ShaderLab：ShaderLab 编译器
  - Native
    - EditorApp：编辑器 App
    - Engine：引擎核心的 C++ 代码
  - Shaders：引擎内置的 Shader
- Tools：工具

## 项目配置

打开 EditorApp 的属性页面，配置调试参数

<p align="center"><img src="Attachments/debugging.png"></p>

### 命令行参数

必须提供的参数

- `--project`：指定项目的路径，只要给一个合法的路径就行，文件夹不存在则会自动创建

图形调试参数，这几个是可选且互斥的，最多只能给定一个

- `--renderdoc`：启动时加载 RenderDoc，之后点击编辑器上方的相机按钮就能截帧
- `--pix`：启动时加载 PIX，之后点击编辑器上方的相机按钮就能截帧
- `--d3d12-debug-layer`：启用 D3D12 的调试层，能提供详细的错误信息，但运行会变卡

### 工作目录

`Working Directory` 必须设置为 `$(TargetDir)`。

### 调试器

推荐使用 `Mixed (.NET Core)`，它可以同时调试 C++ 和 C# 代码。这个调试器貌似是 Visual Studio 独占的，所以我才用它开发。

只有加载完 .NET Runtime，`Mixed (.NET Core)` 调试器才能正常工作，如果在这之前的 C++ 代码出错了需要调试，可以临时改用 `Native Only` 调试器。

## 构建结果

构建结果都在 Build 文件夹中。
