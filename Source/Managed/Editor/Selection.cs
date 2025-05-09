using March.Core;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor
{
    public static class Selection
    {
        public static List<MarchObject> Objects { get; } = [];

        public static int Count => Objects.Count;

        public static bool IsEmpty => Objects.Count == 0;

        [return: NotNullIfNotNull(nameof(defaultObject))]
        public static MarchObject? FirstOrDefault(MarchObject? defaultObject = null)
        {
            return Objects.Count > 0 ? Objects[0] : defaultObject;
        }

        [return: NotNullIfNotNull(nameof(defaultObject))]
        public static T? FirstOrDefault<T>(T? defaultObject = null) where T : MarchObject
        {
            foreach (MarchObject obj in Objects)
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
            foreach (MarchObject obj in Objects)
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
            foreach (MarchObject obj in Objects)
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
            return Objects.Contains(obj);
        }

        public static void Clear()
        {
            Objects.Clear();
        }

        public static void Set(MarchObject obj)
        {
            Objects.Clear();
            Objects.Add(obj);
        }

        public static void Set(IEnumerable<MarchObject> objs)
        {
            Objects.Clear();
            Objects.AddRange(objs);
        }

        public static void Set(params ReadOnlySpan<MarchObject> objs)
        {
            Objects.Clear();
            Objects.AddRange(objs);
        }
    }
}
