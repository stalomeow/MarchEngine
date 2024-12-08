#pragma once

namespace march
{
    class GfxDevice;

    class GfxDeviceChild
    {
    public:
        virtual ~GfxDeviceChild() = default;
        GfxDevice* GetDevice() const { return m_Device; }

    protected:
        GfxDeviceChild(GfxDevice* device) : m_Device(device) {}

    private:
        GfxDevice* m_Device;
    };
}
