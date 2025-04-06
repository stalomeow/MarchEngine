using March.Core.Diagnostics;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.ShaderLab;
using Newtonsoft.Json;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Compute Shader Asset", ".compute", Version = 14)]
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

            shader.Name = Path.GetFileNameWithoutExtension(Location.AssetFullPath);

            AddDependency(ref context, content);
            shader.Compile(Location.AssetFullPath, content, m_Warnings, m_Errors);

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

        private void AddDependency(ref AssetImportContext context, string source)
        {
            using var includes = ListPool<AssetLocation>.Get();
            ShaderProgramUtility.GetIncludeFiles(Location.AssetFullPath, source, includes);

            foreach (AssetLocation location in includes.Value)
            {
                context.RequireOtherAsset<ShaderIncludeAsset>(location.AssetPath, dependsOn: true);
            }
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
