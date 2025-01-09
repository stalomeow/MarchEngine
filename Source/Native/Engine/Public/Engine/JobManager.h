#pragma once

#include "Engine/Ints.h"
#include <functional>

namespace march
{
    class JobHandle
    {
    public:
        void Complete();

    private:
        uint64 m_GroupId;
    };

    struct JobData final
    {
        std::function<void(size_t)> Func;
    };

    struct JobManager final
    {
        static JobHandle Schedule(size_t totalSize, size_t batchSize, const std::function<void(size_t)>& func);
    };
}
