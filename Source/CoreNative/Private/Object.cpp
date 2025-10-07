#include "pch.h"
#include "Engine/Object.h"
#include "Engine/Application.h"
#include "Engine/Memory/MemoryManager.h"
#include <vector>
#include <mutex>

namespace march
{
    static std::vector<MarchObject*> g_DeleteQueue{};
    static std::mutex g_DeleteQueueMutex{};

    void MarchObject::ThreadSafeDelete(MarchObject* obj)
    {
        if (GetApp()->IsOnMainThread())
        {
            MARCH_DELETE(obj, MemoryLabel::Default);
        }
        else
        {
            std::lock_guard lock(g_DeleteQueueMutex);
            g_DeleteQueue.push_back(obj);
        }
    }

    void MarchObject::ProcessDeleteQueueOnMainThread()
    {
        std::lock_guard lock(g_DeleteQueueMutex);

        for (MarchObject* obj : g_DeleteQueue)
        {
            MARCH_DELETE(obj, MemoryLabel::Default);
        }

        g_DeleteQueue.clear();
    }
}
