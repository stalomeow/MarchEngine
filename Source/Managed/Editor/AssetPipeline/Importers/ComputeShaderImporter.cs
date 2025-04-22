using March.Core;
using March.Core.Diagnostics;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using March.ShaderLab;
using Newtonsoft.Json;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Compute Shader Asset", ".compute", Version = ShaderCompiler.Version + 26)]
    public class ComputeShaderImporter : AssetImporter
    {
        [JsonProperty]
        [HideInInspector]
        private bool m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;

        [JsonProperty]
        [HideInInspector]
        private GraphicsColorSpace m_ColorSpace = GraphicsSettings.ColorSpace;

        [JsonProperty]
        [HideInInspector]
        private List<string> m_Warnings = [];

        [JsonProperty]
        [HideInInspector]
        private List<string> m_Errors = [];

        [JsonProperty]
        [HideInInspector]
        private List<byte[]> m_ProgramHashes = [];

        protected override bool CheckNeedReimport(bool fullCheck)
        {
            return m_UseReversedZBuffer != GraphicsSettings.UseReversedZBuffer
                || m_ColorSpace != GraphicsSettings.ColorSpace
                || base.CheckNeedReimport(fullCheck);
        }

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;
            m_ColorSpace = GraphicsSettings.ColorSpace;

            ComputeShader shader = context.AddMainAsset<ComputeShader>(normalIcon: FontAwesome6Brands.StripeS);
            CompileShader(ref context, shader, File.ReadAllText(Location.AssetFullPath, Encoding.UTF8));
        }

        private void CompileShader(ref AssetImportContext context, ComputeShader shader, string content)
        {
            m_Warnings.Clear();
            m_Errors.Clear();
            m_ProgramHashes.Clear();

            shader.Name = Path.GetFileNameWithoutExtension(Location.AssetFullPath);

            ShaderCompiler.GetHLSLIncludesAndPragmas(content, out List<string> includes, out List<string> pragmas);
            AddDependency(ref context, includes);
            shader.Compile(Location.AssetFullPath, content, pragmas, m_Warnings, m_Errors);

            m_ProgramHashes.AddRange(ShaderProgramUtility.GetDistinctProgramHashes(
                from pass in shader.Kernels
                from program in pass.Programs
                select program.Hash));

            if (m_Warnings.Count > 0)
            {
                m_Warnings = m_Warnings.Distinct().ToList();
            }

            if (m_Errors.Count > 0)
            {
                m_Errors = m_Errors.Distinct().ToList();
            }
        }

        public int WarningCount => m_Warnings.Count;

        public int ErrorCount => m_Errors.Count;

        public override void LogImportMessages()
        {
            base.LogImportMessages();

            foreach (string warning in m_Warnings)
            {
                Log.Message(LogLevel.Warning, warning);
            }

            foreach (string error in m_Errors)
            {
                Log.Message(LogLevel.Error, error);
            }
        }

        private void AddDependency(ref AssetImportContext context, List<string> includes)
        {
            using var locations = ListPool<AssetLocation>.Get();
            ShaderProgramUtility.GetHLSLIncludeFileLocations(Location.AssetFullPath, includes, locations);

            foreach (AssetLocation location in locations.Value)
            {
                context.RequireOtherAsset<ShaderIncludeAsset>(location.AssetPath, dependsOn: true);
            }
        }

        protected override MarchObject? TryLoadAssetFromCache(string guid)
        {
            // 保证所有的 shader program 都有对应的缓存
            foreach (byte[] hash in m_ProgramHashes)
            {
                if (!ShaderUtility.HasCachedShaderProgram(hash))
                {
                    return null;
                }
            }

            return base.TryLoadAssetFromCache(guid);
        }
    }

    internal class ComputeShaderImporterDrawer : AssetImporterDrawerFor<ComputeShaderImporter>
    {
        protected override bool RequireAssets => true;

        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = base.DrawProperties(out showApplyRevertButtons);

            EditorGUI.LabelField("Warnings", string.Empty, Target.WarningCount.ToString());
            EditorGUI.LabelField("Errors", string.Empty, Target.ErrorCount.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                Target.LogImportMessages();
            }

            return isChanged;
        }

        protected override void DrawAdditional()
        {
            base.DrawAdditional();

            if (EditorGUI.Foldout("Kernels", string.Empty, defaultOpen: true))
            {
                ComputeShader shader = (ComputeShader)Target.MainAsset;

                using (new EditorGUI.IndentedScope())
                {
                    foreach (ComputeShaderKernel kernel in shader.Kernels)
                    {
                        EditorGUI.BulletLabel(kernel.Name, "");
                    }
                }
            }
        }
    }
}
