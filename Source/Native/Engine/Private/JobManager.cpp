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
        JobHandle handle{};

        cs<JobHandle*> arg0;
        cs_ulong arg1;
        cs_ulong arg2;
        cs_nint arg3;

        arg0.assign(&handle); // 如果把这个参数写成返回值的话，C++ 编译器会做优化，导致函数指针和 C# 那边不一致
        arg1.assign(totalSize);
        arg2.assign(batchSize);
        arg3.assign(data);

        DotNet::RuntimeInvoke<void>(ManagedMethod::JobManager_NativeSchedule, arg0.data, arg1.data, arg2.data, arg3.data);
        return handle;
    }
}
