using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DX12Demo.Core
{
    public static class SceneManager
    {
        public static Scene CurrentScene { get; set; } = new();
    }
}
