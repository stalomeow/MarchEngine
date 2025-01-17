#include "pch.h"
#include "Engine/JobManager.h"
#include "Engine/Debug.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Scripting/InteropServices.h"

namespace march
{
    void JobHandle::Complete()
    {
        DotNet::RuntimeInvoke<void, JobHandle>(ManagedMethod::JobManager_NativeComplete, *this);
    }

    JobHandle JobManager::Schedule(size_t totalSize, size_t batchSize, const std::function<void(size_t)>& func)
    {
        JobData* data = MARCH_NEW JobData{ func };

        cs_ulong arg0;
        cs_ulong arg1;
        cs_nint arg2;

        arg0.assign(totalSize);
        arg1.assign(batchSize);
        arg2.assign(data);

        return DotNet::RuntimeInvoke<JobHandle>(ManagedMethod::JobManager_NativeSchedule, arg0.data, arg1.data, arg2.data);
    }
}
