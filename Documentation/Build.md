# Build

先劝退一下。项目现在处于玩具阶段，代码也比较复杂，还经常有 Breaking Change，而且我没空写非常详细的文档，请做好心理准备。

## 要求

- Windows x64
- Windows SDK 10.0
- [Visual Studio 2022](https://visualstudio.microsoft.com/zh-hans/vs/)
- [vcpkg](https://github.com/microsoft/vcpkg)
- [premake5](https://premake.github.io/)

## 生成项目

先下载项目

``` shell
git clone --recursive https://github.com/stalomeow/MarchEngine.git
```

然后进入项目目录

``` shell
cd MarchEngine
```

如果是第一次生成项目，或者修改了 vcpkg.json，需要重新下载和编译第三方库

``` shell
premake5 install
```

生成 Visual Studio 2022 Solution 到 Output 目录里

``` shell
premake5 vs2022
```

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
- `--nvaftermath`：启用 NVIDIA Nsight Aftermath SDK 的基本功能
- `--nvaftermath-full`：启用 NVIDIA Nsight Aftermath SDK 的所有功能

### 工作目录

`Working Directory` 必须设置为 `$(TargetDir)`。

### 调试器

推荐使用 `Mixed (.NET Core)`，它可以同时调试 C++ 和 C# 代码。这个调试器貌似是 Visual Studio 独占的，所以我才用它开发。

只有加载完 .NET Runtime，`Mixed (.NET Core)` 调试器才能正常工作，如果在这之前的 C++ 代码出错了需要调试，可以临时改用 `Native Only` 调试器。

## 清理项目

可以使用下面的命令清理生成的文件

``` shell
premake5 clean
```
