using March.Core;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor
{
    public static class Selection
    {
        private static readonly List<MarchObject> s_Objects = [];
        private static object? s_Context = null;

        public static int Count => s_Objects.Count;

        public static bool IsEmpty => s_Objects.Count == 0;

        public static object? Context => s_Context;

        public static MarchObject Get(int index)
        {
            return s_Objects[index];
        }

        [return: NotNullIfNotNull(nameof(defaultObject))]
        public static MarchObject? FirstOrDefault(MarchObject? defaultObject = null)
        {
            return s_Objects.Count > 0 ? s_Objects[0] : defaultObject;
        }

        [return: NotNullIfNotNull(nameof(defaultObject))]
        public static T? FirstOrDefault<T>(T? defaultObject = null) where T : MarchObject
        {
            foreach (MarchObject obj in s_Objects)
            {
                if (obj is T t)
                {
                    return t;
                }
            }

            return defaultObject;
        }

        public static bool Any<T>() where T : MarchObject
        {
            foreach (MarchObject obj in s_Objects)
            {
                if (obj is T)
                {
                    return true;
                }
            }

            return false;
        }

        public static bool All<T>() where T : MarchObject
        {
            foreach (MarchObject obj in s_Objects)
            {
                if (obj is not T)
                {
                    return false;
                }
            }

            return true;
        }

        public static bool Contains(MarchObject obj)
        {
            return s_Objects.Contains(obj);
        }

        public static void Clear(object? context = null)
        {
            s_Objects.Clear();
            s_Context = context;
        }

        public static void Set(MarchObject obj, object? context = null)
        {
            s_Objects.Clear();
            s_Objects.Add(obj);
            s_Context = context;
        }

        public static void Set(IEnumerable<MarchObject> objs, object? context = null)
        {
            s_Objects.Clear();
            s_Objects.AddRange(objs);
            s_Context = context;
        }

        public static void Set(ReadOnlySpan<MarchObject> objs, object? context = null)
        {
            s_Objects.Clear();
            s_Objects.AddRange(objs);
            s_Context = context;
        }

        public static void Add(MarchObject obj, object? context = null)
        {
            if (s_Context != context)
            {
                s_Objects.Clear();
                s_Context = context;
            }

            if (!s_Objects.Contains(obj))
            {
                s_Objects.Add(obj);
            }
        }

        public static void AddRange(IEnumerable<MarchObject> objs, object? context = null)
        {
            if (s_Context != context)
            {
                s_Objects.Clear();
                s_Context = context;
            }

            s_Objects.AddRange(objs);
        }

        public static void AddRange(ReadOnlySpan<MarchObject> objs, object? context = null)
        {
            if (s_Context != context)
            {
                s_Objects.Clear();
                s_Context = context;
            }

            s_Objects.AddRange(objs);
        }

        public static void Remove(MarchObject obj, object? context = null)
        {
            if (s_Context != context)
            {
                s_Objects.Clear();
                s_Context = context;
                return;
            }

            s_Objects.Remove(obj);
        }

        public static void RemoveAt(int index, object? context = null)
        {
            if (s_Context != context)
            {
                s_Objects.Clear();
                s_Context = context;
                return;
            }

            s_Objects.RemoveAt(index);
        }
    }
}
