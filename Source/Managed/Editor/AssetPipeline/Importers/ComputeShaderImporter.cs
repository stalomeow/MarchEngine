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
    [CustomAssetImporter("Compute Shader Asset", ".compute", Version = 10)]
    internal class ComputeShaderImporter : AssetImporter
    {
        [JsonProperty]
        [HideInInspector]
        private bool m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;

        [JsonProperty]
        [HideInInspector]
        private GraphicsColorSpace m_ColorSpace = GraphicsSettings.ColorSpace;

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
            shader.ClearWarningsAndErrors();
            shader.Name = Path.GetFileNameWithoutExtension(Location.AssetFullPath);

            if (shader.Compile(Location.AssetFullPath, content))
            {
                AddDependency(ref context, content);
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
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = base.DrawProperties(out showApplyRevertButtons);

            ComputeShader shader = (ComputeShader)Target.MainAsset;

            EditorGUI.LabelField("Warnings", string.Empty, shader.Warnings.Length.ToString());
            EditorGUI.LabelField("Errors", string.Empty, shader.Errors.Length.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                foreach (string warning in shader.Warnings)
                {
                    Log.Message(LogLevel.Warning, warning);
                }

                foreach (string error in shader.Errors)
                {
                    Log.Message(LogLevel.Error, error);
                }
            }

            return isChanged;
        }
    }
}
