#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include "PipelineState.h"

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
        virtual void AddFullScreenTriangle() = 0;

        virtual uint32_t GetSubMeshCount() const = 0;
        virtual D3D12_PRIMITIVE_TOPOLOGY GetTopology() const = 0;
        virtual D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType() const = 0;
        virtual D3D12_INPUT_LAYOUT_DESC GetVertexInputLayout() const = 0;

        MeshDesc GetDesc() const;

        GfxDevice* GetDevice() const { return m_Device; }

        static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(D3D12_PRIMITIVE_TOPOLOGY topology)
        {
            switch (topology)
            {
            case D3D_PRIMITIVE_TOPOLOGY_UNDEFINED:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

            case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

            case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
            case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
            case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
            case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
            case D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            default:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            }
        }

    private:
        GfxDevice* m_Device;
    };

    GfxMesh* CreateSimpleGfxMesh(GfxDevice* device);
    void ReleaseSimpleGfxMesh(GfxMesh* mesh);
}
