using March.Core.Diagnostics;
using March.Core.Interop;
using March.Core.Pool;
using System.Runtime.InteropServices;

namespace March.Core
{
    internal static partial class NativeJobUtility
    {
        [NativeMethod]
        public static partial void Invoke(nint job, ulong index);

        [NativeMethod]
        public static partial void Release(nint job);
    }

    /// <summary>
    /// WaitGroup
    /// </summary>
    /// <param name="onCompletedCallback">参数是 id 和 state，这个事件会在线程池中被触发，注意线程安全</param>
    internal class JobWaitGroup(Action<ulong, object?>? onCompletedCallback)
    {
        private readonly ManualResetEventSlim m_Event = new(true);

        private ulong m_Id;
        private ulong m_Countdown = 0;
        private Action<int>? m_ManagedJob;
        private nint m_NativeJob;
        private object? m_State;

        public void Reset(ulong id, ulong count, Action<int>? managedJob, nint nativeJob, object? state)
        {
            if (count == 0)
            {
                throw new ArgumentException("Count must be greater than 0", nameof(count));
            }

            Interlocked.Exchange(ref m_Id, id);
            Interlocked.Exchange(ref m_Countdown, count);
            Interlocked.Exchange(ref m_ManagedJob, managedJob);
            Interlocked.Exchange(ref m_NativeJob, nativeJob);
            Interlocked.Exchange(ref m_State, state);
            m_Event.Reset();
        }

        public ulong Id => m_Id;

        public Action<int>? ManagedJob => m_ManagedJob;

        public nint NativeJob => m_NativeJob;

        public void Done()
        {
            if (Interlocked.Decrement(ref m_Countdown) == 0)
            {
                m_Event.Set();

                onCompletedCallback?.Invoke(m_Id, m_State);
                NativeJobUtility.Release(NativeJob);

                Interlocked.Exchange(ref m_ManagedJob, null);
                Interlocked.Exchange(ref m_NativeJob, nint.Zero);
                Interlocked.Exchange(ref m_State, null);
            }
        }

        public void Wait()
        {
            m_Event.Wait();
        }
    }

    internal class JobItem : IThreadPoolWorkItem
    {
        private ExecutionContext? m_Context;
        private JobWaitGroup? m_Group;

        private ulong m_StartIndex;
        private ulong m_EndIndex;

        public void Reset(ExecutionContext? context, JobWaitGroup group, ulong startIndex, ulong endIndex)
        {
            m_Context = context;
            m_Group = group;
            m_StartIndex = startIndex;
            m_EndIndex = endIndex;
        }

        private ContextCallback? m_ContextCallbackCache;

        void IThreadPoolWorkItem.Execute()
        {
            try
            {
                if (m_Context != null)
                {
                    ExecutionContext.Run(m_Context, m_ContextCallbackCache ??= ExecuteBatch, null);
                }
                else
                {
                    ExecuteBatch(null);
                }
            }
            finally
            {
                JobWaitGroup wg = m_Group!;

                // 清理引用
                m_Context = null;
                m_Group = null;

                wg.Done();
            }
        }

        private void ExecuteBatch(object? state)
        {
            JobWaitGroup wg = m_Group!;

            if (wg.ManagedJob != null)
            {
                int startIndex = (int)m_StartIndex;
                int endIndex = (int)m_EndIndex;

                for (int i = startIndex; i < endIndex; i++)
                {
                    wg.ManagedJob(i);
                }
            }
            else
            {
                for (ulong i = m_StartIndex; i < m_EndIndex; i++)
                {
                    NativeJobUtility.Invoke(wg.NativeJob, i);
                }
            }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct JobHandle
    {
        internal ulong GroupId;
    }

    public static class JobManager
    {
        private static readonly Queue<JobItem> s_ItemPool = new();
        private static readonly Queue<JobWaitGroup> s_WaitGroupPool = new();
        private static readonly Dictionary<ulong, JobWaitGroup> s_RunningJobs = new();
        private static readonly Lock s_Lock = new();

        private static ulong s_LastGroupId = 0; // 0 保留为无效值

        public static JobHandle Schedule(int totalSize, int batchSize, Action<int> func)
        {
            return ScheduleInternal((ulong)totalSize, (ulong)batchSize, func, nint.Zero);
        }

        private static JobHandle ScheduleInternal(ulong totalSize, ulong batchSize, Action<int>? managedJob, nint nativeJob)
        {
            if (totalSize == 0)
            {
                throw new ArgumentException("Total size must be greater than 0", nameof(totalSize));
            }

            if (batchSize == 0)
            {
                throw new ArgumentException("Batch size must be greater than 0", nameof(batchSize));
            }

            ExecutionContext? context = ExecutionContext.Capture();

            ulong groupId = Interlocked.Increment(ref s_LastGroupId);
            ulong itemCount = (totalSize + batchSize - 1) / batchSize;
            List<JobItem> items = ListPool<JobItem>.Shared.Rent();

            using (s_Lock.EnterScope())
            {
                if (!s_WaitGroupPool.TryDequeue(out JobWaitGroup? wg))
                {
                    wg = new JobWaitGroup(OnGroupCompleted);
                }

                wg.Reset(groupId, itemCount, managedJob, nativeJob, items);

                for (ulong i = 0; i < itemCount; i++)
                {
                    if (!s_ItemPool.TryDequeue(out JobItem? item))
                    {
                        item = new JobItem();
                    }

                    ulong startIndex = i * batchSize;
                    ulong endIndex = Math.Min(startIndex + batchSize, totalSize);

                    item.Reset(context, wg, startIndex, endIndex);
                    items.Add(item);

                    ThreadPool.UnsafeQueueUserWorkItem(item, preferLocal: false);
                }

                s_RunningJobs.Add(groupId, wg);
            }

            return new JobHandle { GroupId = groupId };
        }

        private static void OnGroupCompleted(ulong groupId, object? state)
        {
            var items = (List<JobItem>)state!;

            try
            {
                using (s_Lock.EnterScope())
                {
                    if (s_RunningJobs.Remove(groupId, out JobWaitGroup? wg))
                    {
                        s_WaitGroupPool.Enqueue(wg);
                    }
                    else
                    {
                        Log.Message(LogLevel.Warning, "Job group is not running", $"{groupId}");
                    }

                    foreach (JobItem item in items)
                    {
                        s_ItemPool.Enqueue(item);
                    }
                }
            }
            finally
            {
                ListPool<JobItem>.Shared.Return(items);
            }
        }

        public static void Complete(this JobHandle handle)
        {
            bool running;
            JobWaitGroup? wg;

            using (s_Lock.EnterScope())
            {
                running = s_RunningJobs.TryGetValue(handle.GroupId, out wg);
            }

            if (running && handle.GroupId == wg!.Id)
            {
                wg!.Wait();
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe void NativeSchedule(JobHandle* handle, ulong totalSize, ulong batchSize, nint job)
        {
            *handle = ScheduleInternal(totalSize, batchSize, null, job);
        }

        [UnmanagedCallersOnly]
        private static void NativeComplete(JobHandle handle)
        {
            Complete(handle);
        }
    }
}
