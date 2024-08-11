using System.Diagnostics.CodeAnalysis;

namespace DX12Demo.Editor
{
    internal class ProjectFileTree
    {
        private static readonly char[] s_PathSeparators = { '/', '\\' };

        private sealed class Node
        {
            public SortedDictionary<string, Node> Folders { get; } = new();

            public SortedSet<string> Files { get; } = new();

            public void Draw()
            {
                // 没必要加 IDScope，因为每个结点名字都不一样

                foreach ((string name, Node node) in Folders)
                {
                    if (EditorGUI.BeginTreeNode(name, openOnArrow: true, openOnDoubleClick: true, spanWidth: true))
                    {
                        node.Draw();
                        EditorGUI.EndTreeNode();
                    }
                }

                foreach(string name in Files)
                {
                    if (EditorGUI.BeginTreeNode(name, isLeaf: true, spanWidth: true))
                    {
                        EditorGUI.EndTreeNode();
                    }
                }
            }

            public void Clear()
            {
                Folders.Clear();
                Files.Clear();
            }
        }

        private readonly Node m_Root = new();

        public void AddFile(string path) => Add(path, false);

        public void AddFolder(string path) => Add(path, true);

        public void Add(string path, bool isFolder)
        {
            if (FindParentNode(path, out string[] segments, out Node? node))
            {
                if (isFolder)
                {
                    node.Folders.Add(segments[^1], new Node());
                }
                else
                {
                    node.Files.Add(segments[^1]);
                }
            }
        }

        public void Remove(string path)
        {
            if (FindParentNode(path, out string[] segments, out Node? node))
            {
                string key = segments[^1];

                if (!node.Folders.Remove(key))
                {
                    node.Files.Remove(key);
                }
            }
        }

        private bool FindParentNode(string path, out string[] segments, [NotNullWhen(true)] out Node? node)
        {
            segments = path.Split(s_PathSeparators, StringSplitOptions.RemoveEmptyEntries);

            if (segments.Length == 0)
            {
                node = null;
                return false;
            }

            node = m_Root;

            for (int i = 0; i < segments.Length - 1; i++)
            {
                string seg = segments[i];

                if (!node.Folders.TryGetValue(seg, out Node? n))
                {
                    n = new Node();
                    node.Folders.Add(seg, n);
                }

                node = n;
            }

            return true;
        }

        public void Clear()
        {
            m_Root.Clear();
        }

        public void Draw()
        {
            m_Root.Draw();
        }
    }
}
