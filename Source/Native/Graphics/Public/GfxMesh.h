#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>

namespace march
{
    class GfxDevice;

    struct GfxSubMesh
    {
        int32_t BaseVertexLocation;
        uint32_t StartIndexLocation;
        uint32_t IndexCount;
    };

    class GfxMesh
    {
    protected:
        GfxMesh(GfxDevice* device);

    public:
        GfxMesh(const GfxMesh&) = delete;
        GfxMesh& operator=(const GfxMesh&) = delete;

        virtual ~GfxMesh() = default;

        virtual void Draw() = 0;
        virtual void Draw(uint32_t subMeshIndex) = 0;
        virtual void RecalculateNormals() = 0;
        virtual void ClearSubMeshes() = 0;
        virtual void AddSubMeshCube(float width, float height, float depth) = 0;
        virtual void AddSubMeshSphere(float radius, uint32_t sliceCount, uint32_t stackCount) = 0;

        virtual uint32_t GetSubMeshCount() const = 0;
        virtual D3D12_PRIMITIVE_TOPOLOGY GetTopology() const = 0;
        virtual D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType() const = 0;
        virtual D3D12_INPUT_LAYOUT_DESC GetVertexInputLayout() const = 0;

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        GfxDevice* m_Device;
    };

    GfxMesh* CreateSimpleGfxMesh(GfxDevice* device);
    void ReleaseSimpleGfxMesh(GfxMesh* mesh);
}
