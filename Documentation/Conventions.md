# Conventions

- 由于 C# 数学库和 [DirectXMath](https://learn.microsoft.com/en-us/windows/win32/dxmath/directxmath-portal)，C# 和 C++ 中矩阵为行主序，而 HLSL 中矩阵为列主序，请务必牢记这点
- 坐标系都是左手坐标系，X 轴朝右，Y 轴朝上，Z 轴朝前，View Space 也是这样，这点和 Unity 不同
- NDC 的 XY 范围是 $[-1,1]$，Y 轴朝上，Z 范围是 $[0,1]$，默认开启 Reversed-Z Buffer
- UV 的原点在左上角，Y 轴朝下
- 顺时针旋转是正向，包括 Mesh 中三角形的顶点顺序
