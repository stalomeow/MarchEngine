#include "pch.h"
#include "Engine/Object.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO NativeMarchObject_Delete(cs<MarchObject*> ptr)
{
    // C# 的 Finalize 不在主线程调用
    MarchObject::ThreadSafeDelete(ptr);
}
