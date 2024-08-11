using DX12Demo.Core.Binding;

namespace DX12Demo.Core
{
    public static partial class Application
    {
        private static string? s_DataPath;

        public static string DataPath
        {
            get
            {
                if (s_DataPath == null)
                {
                    nint path = Application_GetDataPath();
                    s_DataPath = NativeString.Get(path);
                    NativeString.Free(path);
                }

                return s_DataPath;
            }
        }

        [NativeFunction]
        private static partial nint Application_GetDataPath();
    }
}
