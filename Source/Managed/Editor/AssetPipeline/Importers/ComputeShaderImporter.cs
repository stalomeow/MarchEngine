using March.Core.Diagnostics;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.IconFont;
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
        private List<string> m_Warnings = [];

        [JsonProperty]
        [HideInInspector]
        private List<string> m_Errors = [];

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            ComputeShader shader = context.AddMainAsset<ComputeShader>(normalIcon: FontAwesome6Brands.StripeS);
            CompileShader(ref context, shader, File.ReadAllText(Location.AssetFullPath, Encoding.UTF8));
            context.SetUserData(ShaderProgramUtility.CreateManifest(shader));
        }

        private void CompileShader(ref AssetImportContext context, ComputeShader shader, string content)
        {
            m_Warnings.Clear();
            m_Errors.Clear();

            shader.Name = Path.GetFileNameWithoutExtension(Location.AssetFullPath);

            ShaderCompiler.GetHLSLIncludesAndPragmas(content, out List<string> includes, out List<string> pragmas);
            AddDependency(ref context, includes);
            shader.Compile(Location.AssetFullPath, content, pragmas, m_Warnings, m_Errors);

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

        protected override bool HasValidAssetCache(string guid)
        {
            if (UserData is not ShaderProgramManifest manifest)
            {
                return false;
            }

            if (!ShaderProgramUtility.HasValidCache(manifest))
            {
                return false;
            }

            return base.HasValidAssetCache(guid);
        }

        protected override void DeleteAssetCache(string guid)
        {
            if (UserData is ShaderProgramManifest manifest)
            {
                ShaderProgramUtility.DeleteCache(manifest);
            }

            base.DeleteAssetCache(guid);
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
