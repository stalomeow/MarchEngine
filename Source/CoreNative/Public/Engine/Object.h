#pragma once

namespace march
{
    class MarchObject
    {
    public:
        virtual ~MarchObject() = default;

        static void ThreadSafeDelete(MarchObject* obj);
        static void ProcessDeleteQueueOnMainThread();
    };
}
