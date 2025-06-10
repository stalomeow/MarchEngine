#include "pch.h"
#include "Engine/Object.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO NativeMarchObject_Delete(cs<MarchObject*> ptr)
{
    delete ptr;
}
