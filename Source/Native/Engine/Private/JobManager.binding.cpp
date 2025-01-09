#include "pch.h"
#include "Engine/JobManager.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO NativeJobUtility_Invoke(cs<JobData*> job, cs_ulong index)
{
    job->Func(index);
}

NATIVE_EXPORT_AUTO NativeJobUtility_Release(cs<JobData*> job)
{
    delete job;
}
