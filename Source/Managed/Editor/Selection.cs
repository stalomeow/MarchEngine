using March.Core;
using March.Core.Pool;
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

        public static bool All<T>(bool allowEmpty = false) where T : MarchObject
        {
            if (!allowEmpty && Objects.Count == 0)
            {
                return false;
            }

            foreach (MarchObject obj in Objects)
            {
                if (obj is not T)
                {
                    return false;
                }
            }

            return true;
        }

        public static void GetTopLevelGameObjects(List<GameObject> gameObjects)
        {
            using var transforms = HashSetPool<Transform>.Get();

            foreach (MarchObject obj in Objects)
            {
                if (obj is GameObject go)
                {
                    transforms.Value.Add(go.transform);
                }
            }

            foreach (Transform trans in transforms.Value)
            {
                bool isTopLevel = true;
                Transform? parent = trans.Parent;

                while (parent != null)
                {
                    if (transforms.Value.Contains(parent))
                    {
                        isTopLevel = false;
                        break;
                    }

                    parent = parent.Parent;
                }

                if (isTopLevel)
                {
                    gameObjects.Add(trans.gameObject);
                }
            }
        }

        public static bool Contains(MarchObject obj)
        {
            return Objects.Contains(obj);
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

        public static void Set(ReadOnlySpan<MarchObject> objs)
        {
            Objects.Clear();
            Objects.AddRange(objs);
        }
    }
}
